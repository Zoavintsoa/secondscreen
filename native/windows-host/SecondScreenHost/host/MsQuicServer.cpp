#include "MsQuicServer.h"

#include "../../../shared-protocol/CONTROL_PROTOCOL.h"
#include "../../../shared-protocol/CONTROL_SESSION.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <iostream>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#if __has_include(<msquic.h>)
#define SECOND_SCREEN_HAS_MSQUIC 1
#include <msquic.h>
#else
#define SECOND_SCREEN_HAS_MSQUIC 0
#endif

namespace second_screen {

#if SECOND_SCREEN_HAS_MSQUIC

namespace {

using second_screen::control::ControlFrameParser;
using second_screen::control::ControlMessage;
using second_screen::control::ControlSession;
using second_screen::control::ParseStatus;
using second_screen::control::makeControlFrame;

constexpr char kAlpnText[] = "secondscreen/1";
constexpr uint16_t kDefaultDatagramSize = 1200;
constexpr uint32_t kMaxControlPayload = 64 * 1024;

struct OwnedSendBuffer {
    QUIC_BUFFER buffer{};
    std::vector<uint8_t> bytes;

    explicit OwnedSendBuffer(const uint8_t* data, size_t size) : bytes(data, data + size) {
        buffer.Buffer = bytes.data();
        buffer.Length = static_cast<uint32_t>(bytes.size());
    }
};

struct ConnectionContext;

MsQuicServer::MsQuicServer(Config config, quic::TransportCallbacks callbacks)
    : impl_(std::make_unique<Impl>()) {
    impl_->config = std::move(config);
    impl_->callbacks = std::move(callbacks);
}

MsQuicServer::~MsQuicServer() {
    stop();
}

bool MsQuicServer::start() {
#if !SECOND_SCREEN_HAS_MSQUIC
    return false;
#else
    if (impl_->running) return true;
    if (impl_->config.certificateThumbprint.empty()) return false;

    std::array<uint8_t, 20> thumbprint{};
    if (!parseThumbprint(impl_->config.certificateThumbprint, thumbprint)) return false;

    QUIC_STATUS status = MsQuicOpen2(&impl_->api);
    if (QUIC_FAILED(status) || !impl_->api) {
        impl_->api = nullptr;
        return false;
    }

    QUIC_REGISTRATION_CONFIG registrationConfig{
        "SecondScreen",
        QUIC_EXECUTION_PROFILE_LOW_LATENCY
    };

    status = impl_->api->RegistrationOpen(&registrationConfig, &impl_->registration);
    if (QUIC_FAILED(status)) {
        MsQuicClose(impl_->api);
        impl_->api = nullptr;
        return false;
    }

    const uint8_t alpnBytes[] = {'s','e','c','o','n','d','s','c','r','e','e','n','/','1'};
    QUIC_BUFFER alpn{sizeof(alpnBytes), const_cast<uint8_t*>(alpnBytes)};

    QUIC_SETTINGS settings{};
    settings.IsSet.IdleTimeoutMs = TRUE;
    settings.IdleTimeoutMs = 30000;
    settings.IsSet.HandshakeIdleTimeoutMs = TRUE;
    settings.HandshakeIdleTimeoutMs = 10000;
    settings.IsSet.PeerBidiStreamCount = TRUE;
    settings.PeerBidiStreamCount = 1;
    settings.IsSet.DatagramReceiveEnabled = TRUE;
    settings.DatagramReceiveEnabled = TRUE;
    settings.IsSet.PacingEnabled = TRUE;
    settings.PacingEnabled = TRUE;
    settings.IsSet.SendBufferingEnabled = TRUE;
    settings.SendBufferingEnabled = TRUE;

    status = impl_->api->ConfigurationOpen(
        impl_->registration,
        &alpn,
        1,
        &settings,
        sizeof(settings),
        nullptr,
        &impl_->configuration);
    if (QUIC_FAILED(status)) {
        impl_->api->RegistrationClose(impl_->registration);
        MsQuicClose(impl_->api);
        impl_->registration = nullptr;
        impl_->api = nullptr;
        return false;
    }

    QUIC_CERTIFICATE_HASH certificateHash{};
    std::memcpy(certificateHash.ShaHash, thumbprint.data(), thumbprint.size());

    QUIC_CREDENTIAL_CONFIG credential{};
    credential.Type = QUIC_CREDENTIAL_TYPE_CERTIFICATE_HASH;
    credential.CertificateHash = &certificateHash;

    status = impl_->api->ConfigurationLoadCredential(
        impl_->configuration,
        &credential);
    if (QUIC_FAILED(status)) {
        impl_->api->ConfigurationClose(impl_->configuration);
        impl_->api->RegistrationClose(impl_->registration);
        MsQuicClose(impl_->api);
        impl_->configuration = nullptr;
        impl_->registration = nullptr;
        impl_->api = nullptr;
        return false;
    }

    status = impl_->api->ListenerOpen(
        impl_->registration,
        listenerCallback,
        impl_.get(),
        &impl_->listener);
    if (QUIC_FAILED(status)) {
        impl_->api->ConfigurationClose(impl_->configuration);
        impl_->api->RegistrationClose(impl_->registration);
        MsQuicClose(impl_->api);
        impl_->listener = nullptr;
        impl_->configuration = nullptr;
        impl_->registration = nullptr;
        impl_->api = nullptr;
        return false;
    }

    QUIC_ADDR address{};
    QuicAddrSetFamily(&address, QUIC_ADDRESS_FAMILY_UNSPEC);
    QuicAddrSetPort(&address, impl_->config.port);

    status = impl_->api->ListenerStart(impl_->listener, &alpn, 1, &address);
    if (QUIC_FAILED(status)) {
        impl_->api->ListenerClose(impl_->listener);
        impl_->api->ConfigurationClose(impl_->configuration);
        impl_->api->RegistrationClose(impl_->registration);
        MsQuicClose(impl_->api);
        impl_->listener = nullptr;
        impl_->configuration = nullptr;
        impl_->registration = nullptr;
        impl_->api = nullptr;
        return false;
    }

    impl_->running = true;
    return true;
#endif
}

void MsQuicServer::stop() {
#if !SECOND_SCREEN_HAS_MSQUIC
    impl_->running = false;
    impl_->connected = false;
#else
    if (!impl_->api) {
        impl_->running = false;
        impl_->connected = false;
        return;
    }

    impl_->running = false;

    if (impl_->listener) {
        impl_->api->ListenerStop(impl_->listener);
        impl_->api->ListenerClose(impl_->listener);
        impl_->listener = nullptr;
    }

    if (impl_->registration) {
        impl_->api->RegistrationShutdown(
            impl_->registration,
            QUIC_CONNECTION_SHUTDOWN_FLAG_SILENT,
            0);
    }

    // RegistrationShutdown causes connection shutdown callbacks. We do not
    // destroy the API table here; MsQuic must finish those callbacks first.
    // The current host uses one transport lifetime, so the next start will
    // only be allowed after a clean shutdown.
    if (impl_->connectionContext) {
        // Best effort: force the connection to shutdown. The callback owns
        // final ConnectionClose/context cleanup.
        impl_->api->ConnectionShutdown(
            impl_->connectionContext->connection,
            QUIC_CONNECTION_SHUTDOWN_FLAG_SILENT,
            0);
    }

    if (impl_->configuration) {
        impl_->api->ConfigurationClose(impl_->configuration);
        impl_->configuration = nullptr;
    }

    if (impl_->registration) {
        impl_->api->RegistrationClose(impl_->registration);
        impl_->registration = nullptr;
    }

    MsQuicClose(impl_->api);
    impl_->api = nullptr;
    impl_->connectionContext = nullptr;
    impl_->connected = false;
    impl_->maxSendLength = kDefaultDatagramSize;
#endif
}

bool MsQuicServer::sendControl(const uint8_t* data, size_t size) {
#if !SECOND_SCREEN_HAS_MSQUIC
    (void)data; (void)size;
    return false;
#else
    if (!impl_->api || !impl_->connectionContext) return false;
    auto* ctx = impl_->connectionContext;
    std::lock_guard lock(ctx->sendMutex);
    return sendStream(*impl_, *ctx, data, size);
#endif
}

bool MsQuicServer::sendVideoDatagram(const uint8_t* data, size_t size) {
#if !SECOND_SCREEN_HAS_MSQUIC
    (void)data; (void)size;
    return false;
#else
    if (!impl_->api || !impl_->connectionContext || !data ||
        size == 0 || size > impl_->maxSendLength) {
        return false;
    }

    auto* owned = new (std::nothrow) OwnedSendBuffer(data, size);
    if (!owned) return false;

    QUIC_BUFFER buffer = owned->buffer;
    const QUIC_STATUS status = impl_->api->DatagramSend(
        impl_->connectionContext->connection,
        &buffer,
        1,
        QUIC_SEND_FLAG_NONE,
        owned);

    if (QUIC_FAILED(status)) {
        delete owned;
        return false;
    }
    return true;
#endif
}

bool MsQuicServer::connected() const {
    return impl_->connected;
}

quic::DatagramLimits MsQuicServer::datagramLimits() const {
    return {impl_->maxSendLength};
}

}
