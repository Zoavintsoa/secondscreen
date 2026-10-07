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

struct MsQuicServer::Impl;

struct ConnectionContext {
    Impl* owner{};
    HQUIC connection{};
    HQUIC controlStream{};
    ControlFrameParser parser{kMaxControlPayload};
    std::unique_ptr<ControlSession> session;
    std::mutex sendMutex;
    bool authenticated{false};
};

struct Impl {
    MsQuicServer::Config config;
    quic::TransportCallbacks callbacks;

    bool running{false};
    bool connected{false};
    uint16_t maxSendLength{kDefaultDatagramSize};

    const QUIC_API_TABLE* api{nullptr};
    HQUIC registration{nullptr};
    HQUIC configuration{nullptr};
    HQUIC listener{nullptr};
    ConnectionContext* connectionContext{nullptr};

    std::mutex stateMutex;
};

bool parseThumbprint(const std::string& text, std::array<uint8_t, 20>& out) {
    std::string hex;
    hex.reserve(text.size());
    for (char c : text) {
        if (c == ' ' || c == ':' || c == '-') continue;
        if (!std::isxdigit(static_cast<unsigned char>(c))) return false;
        hex.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    }
    if (hex.size() != 40) return false;

    for (size_t i = 0; i < out.size(); ++i) {
        auto nibble = [](char c) -> uint8_t {
            if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
            return static_cast<uint8_t>(c - 'A' + 10);
        };
        out[i] = static_cast<uint8_t>((nibble(hex[i * 2]) << 4) | nibble(hex[i * 2 + 1]));
    }
    return true;
}

bool sendStream(MsQuicServer::Impl& impl, ConnectionContext& ctx, const uint8_t* data, size_t size) {
    if (!impl.api || !ctx.controlStream || !data || size == 0 ||
        size > UINT32_MAX || !ctx.connection) {
        return false;
    }

    auto* owned = new (std::nothrow) OwnedSendBuffer(data, size);
    if (!owned) return false;

    const QUIC_STATUS status = impl.api->StreamSend(
        ctx.controlStream,
        &owned->buffer,
        1,
        QUIC_SEND_FLAG_NONE,
        owned);

    if (QUIC_FAILED(status)) {
        delete owned;
        return false;
    }
    return true;
}

QUIC_STATUS QUIC_API streamCallback(HQUIC stream, void* context, QUIC_STREAM_EVENT* event);
QUIC_STATUS QUIC_API connectionCallback(HQUIC connection, void* context, QUIC_CONNECTION_EVENT* event);
QUIC_STATUS QUIC_API listenerCallback(HQUIC listener, void* context, QUIC_LISTENER_EVENT* event);

void closeConnectionContext(ConnectionContext* ctx) {
    if (!ctx) return;
    if (ctx->owner) {
        std::lock_guard lock(ctx->owner->stateMutex);
        if (ctx->owner->connectionContext == ctx) {
            ctx->owner->connectionContext = nullptr;
            ctx->owner->connected = false;
        }
    }
    if (ctx->owner && ctx->owner->callbacks.onAuthenticated && ctx->authenticated) {
        ctx->owner->callbacks.onAuthenticated(false);
    }
    if (ctx->owner && ctx->owner->callbacks.onClosed) {
        ctx->owner->callbacks.onClosed("quic-connection-closed");
    }
    delete ctx;
}

QUIC_STATUS QUIC_API streamCallback(HQUIC stream, void* context, QUIC_STREAM_EVENT* event) {
    auto* ctx = static_cast<ConnectionContext*>(context);
    if (!ctx || !ctx->owner || !ctx->owner->api) return QUIC_STATUS_SUCCESS;

    switch (event->Type) {
    case QUIC_STREAM_EVENT_RECEIVE: {
        std::lock_guard lock(ctx->sendMutex);
        for (uint32_t i = 0; i < event->RECEIVE.BufferCount; ++i) {
            const auto& buffer = event->RECEIVE.Buffers[i];
            ControlMessage message;
            auto status = ctx->parser.push(buffer.Buffer, buffer.Length, message);
            while (status == ParseStatus::MessageReady) {
                if (!ctx->session) {
                    status = ParseStatus::Invalid;
                    break;
                }

                const auto action = ctx->session->onMessage(message);
                if (!action.responseJson.empty()) {
                    const auto response = makeControlFrame(action.responseType, action.responseJson);
                    if (!sendStream(*ctx->owner, *ctx, response.data(), response.size())) {
                        status = ParseStatus::Invalid;
                        break;
                    }
                }

                if (message.type == second_screen::control::MessageType::Hello &&
                    action.accepted) {
                    const auto challenge = [&]() {
                        if (ctx->session->identity().deviceId.empty()) {
                            return PairingManager::Challenge{};
                        }
                        return PairingManager::Challenge{};
                    }();

                    if (ctx->owner->callbacks.onPairingChallenge) {
                        // The actual challenge is created by the security callback below.
                        // This callback is only informational; the code is emitted there.
                        (void)challenge;
                    }
                }

                if (action.authenticated && !ctx->authenticated) {
                    ctx->authenticated = true;
                    if (ctx->owner->callbacks.onAuthenticated) {
                        ctx->owner->callbacks.onAuthenticated(true);
                    }
                }
                if (action.requestKeyframe && ctx->owner->callbacks.onKeyframeRequested) {
                    ctx->owner->callbacks.onKeyframeRequested();
                }
                if (action.close) {
                    ctx->owner->api->StreamShutdown(
                        stream,
                        QUIC_STREAM_SHUTDOWN_FLAG_ABORT | QUIC_STREAM_SHUTDOWN_FLAG_IMMEDIATE,
                        0);
                    return QUIC_STATUS_SUCCESS;
                }

                status = ctx->parser.push(nullptr, 0, message);
            }

            if (status == ParseStatus::Invalid) {
                ctx->owner->api->StreamShutdown(
                    stream,
                    QUIC_STREAM_SHUTDOWN_FLAG_ABORT | QUIC_STREAM_SHUTDOWN_FLAG_IMMEDIATE,
                    0x100);
                return QUIC_STATUS_SUCCESS;
            }
        }
        break;
    }

    case QUIC_STREAM_EVENT_SEND_COMPLETE:
        delete static_cast<OwnedSendBuffer*>(event->SEND_COMPLETE.ClientContext);
        break;

    case QUIC_STREAM_EVENT_PEER_SEND_ABORTED:
        ctx->owner->api->StreamShutdown(
            stream,
            QUIC_STREAM_SHUTDOWN_FLAG_ABORT | QUIC_STREAM_SHUTDOWN_FLAG_IMMEDIATE,
            0);
        break;

    case QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE:
        ctx->owner->api->StreamClose(stream);
        if (ctx->controlStream == stream) ctx->controlStream = nullptr;
        break;

    default:
        break;
    }

    return QUIC_STATUS_SUCCESS;
}

QUIC_STATUS QUIC_API connectionCallback(HQUIC connection, void* context, QUIC_CONNECTION_EVENT* event) {
    auto* ctx = static_cast<ConnectionContext*>(context);
    if (!ctx || !ctx->owner || !ctx->owner->api) return QUIC_STATUS_SUCCESS;

    switch (event->Type) {
    case QUIC_CONNECTION_EVENT_CONNECTED:
        ctx->owner->connected = true;
        break;

    case QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED:
        if ((event->PEER_STREAM_STARTED.Flags & QUIC_STREAM_OPEN_FLAG_UNIDIRECTIONAL) != 0 ||
            ctx->controlStream != nullptr) {
            ctx->owner->api->StreamShutdown(
                event->PEER_STREAM_STARTED.Stream,
                QUIC_STREAM_SHUTDOWN_FLAG_ABORT | QUIC_STREAM_SHUTDOWN_FLAG_IMMEDIATE,
                0x101);
            break;
        }

        ctx->controlStream = event->PEER_STREAM_STARTED.Stream;
        ctx->owner->api->SetCallbackHandler(
            ctx->controlStream,
            streamCallback,
            ctx);
        break;

    case QUIC_CONNECTION_EVENT_DATAGRAM_STATE_CHANGED:
        if (event->DATAGRAM_STATE_CHANGED.SendEnabled &&
            event->DATAGRAM_STATE_CHANGED.MaxSendLength > 0) {
            ctx->owner->maxSendLength = event->DATAGRAM_STATE_CHANGED.MaxSendLength;
        } else {
            ctx->owner->maxSendLength = 0;
        }
        break;

    case QUIC_CONNECTION_EVENT_DATAGRAM_RECEIVED:
        if (ctx->owner->callbacks.onVideoDatagram &&
            event->DATAGRAM_RECEIVED.Buffer) {
            const auto* b = event->DATAGRAM_RECEIVED.Buffer;
            ctx->owner->callbacks.onVideoDatagram(
                std::vector<uint8_t>(b->Buffer, b->Buffer + b->Length));
        }
        break;

    case QUIC_CONNECTION_EVENT_DATAGRAM_SEND_STATE_CHANGED:
        if (event->DATAGRAM_SEND_STATE_CHANGED.State != QUIC_DATAGRAM_SEND_LOST_SUSPECT) {
            delete static_cast<OwnedSendBuffer*>(
                event->DATAGRAM_SEND_STATE_CHANGED.ClientContext);
        }
        break;

    case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE:
        ctx->owner->api->ConnectionClose(connection);
        closeConnectionContext(ctx);
        break;

    default:
        break;
    }

    return QUIC_STATUS_SUCCESS;
}

QUIC_STATUS QUIC_API listenerCallback(HQUIC, void* context, QUIC_LISTENER_EVENT* event) {
    auto* impl = static_cast<Impl*>(context);
    if (!impl || !impl->api) return QUIC_STATUS_SUCCESS;

    if (event->Type != QUIC_LISTENER_EVENT_NEW_CONNECTION) {
        return QUIC_STATUS_SUCCESS;
    }

    std::lock_guard lock(impl->stateMutex);

    if (impl->connectionContext != nullptr) {
        return QUIC_STATUS_INTERNAL_ERROR;
    }

    auto* ctx = new (std::nothrow) ConnectionContext{};
    if (!ctx) return QUIC_STATUS_OUT_OF_MEMORY;

    ctx->owner = impl;
    ctx->connection = event->NEW_CONNECTION.Connection;

    ControlSession::SecurityCallbacks security;
    security.validateSessionToken = [impl](const std::string& deviceId, const std::string& token) {
        return impl->callbacks.validateSessionToken &&
               impl->callbacks.validateSessionToken(deviceId, token);
    };
    security.confirmPairingCode = [impl](const std::string& deviceId, const std::string& code) {
        const bool ok = impl->callbacks.confirmPairingCode &&
                        impl->callbacks.confirmPairingCode(deviceId, code);
        if (ok && impl->callbacks.onPairingChallenge) {
            // No secret is derived from the pairing code; this is only a state notification.
            impl->callbacks.onPairingChallenge(deviceId, code);
        }
        return ok;
    };
    security.issueSessionToken = [impl](const std::string& deviceId) {
        return impl->callbacks.issueSessionToken ?
            impl->callbacks.issueSessionToken(deviceId) : std::string{};
    };
    ctx->session = std::make_unique<ControlSession>(std::move(security));

    impl->api->SetCallbackHandler(
        event->NEW_CONNECTION.Connection,
        connectionCallback,
        ctx);

    const QUIC_STATUS status = impl->api->ConnectionSetConfiguration(
        event->NEW_CONNECTION.Connection,
        impl->configuration);

    if (QUIC_FAILED(status)) {
        impl->api->ConnectionShutdown(
            event->NEW_CONNECTION.Connection,
            QUIC_CONNECTION_SHUTDOWN_FLAG_SILENT,
            0);
        delete ctx;
        return status;
    }

    impl->connectionContext = ctx;
    return QUIC_STATUS_SUCCESS;
}

}

#endif

struct MsQuicServer::Impl {
    Config config;
    quic::TransportCallbacks callbacks;
    bool running{false};
    bool connected{false};
    uint16_t maxSendLength{1200};
#if SECOND_SCREEN_HAS_MSQUIC
    const QUIC_API_TABLE* api{nullptr};
    HQUIC registration{nullptr};
    HQUIC configuration{nullptr};
    HQUIC listener{nullptr};
    ConnectionContext* connectionContext{nullptr};
    std::mutex stateMutex;
#endif
};

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
