#include "MsQuicServer.h"

#include "../../../shared-protocol/CONTROL_PROTOCOL.h"
#include "../../../shared-protocol/CONTROL_SESSION.h"

#include <array>
#include <cctype>
#include <condition_variable>
#include <cstring>
#include <memory>
#include <mutex>
#include <new>
#include <utility>
#include <vector>

#if __has_include(<msquic.h>)
#define SECOND_SCREEN_HAS_MSQUIC 1
#include <msquic.h>
#else
#define SECOND_SCREEN_HAS_MSQUIC 0
#endif

namespace second_screen {

struct ConnectionContext;

struct MsQuicServer::Impl {
    Config config;
    quic::TransportCallbacks callbacks;
    bool running{false};
    std::atomic_bool connected{false};
    std::atomic_uint16_t maxSendLength{1200};

#if SECOND_SCREEN_HAS_MSQUIC
    const QUIC_API_TABLE* api{nullptr};
    HQUIC registration{nullptr};
    HQUIC configuration{nullptr};
    HQUIC listener{nullptr};
    ConnectionContext* connectionContext{nullptr};
    std::mutex stateMutex;
    std::condition_variable stateCv;
#endif
};

#if SECOND_SCREEN_HAS_MSQUIC
namespace {

using second_screen::control::ControlFrameParser;
using second_screen::control::ControlMessage;
using second_screen::control::ControlSession;
using second_screen::control::ParseStatus;
using second_screen::control::makeControlFrame;

constexpr uint16_t kDefaultDatagramSize = 1200;
constexpr uint32_t kMaxControlPayload = 64 * 1024;

struct OwnedSendBuffer {
    QUIC_BUFFER buffer{};
    std::vector<uint8_t> bytes;

    OwnedSendBuffer(const uint8_t* data, size_t size)
        : bytes(data, data + size) {
        buffer.Buffer = bytes.data();
        buffer.Length = static_cast<uint32_t>(bytes.size());
    }
};

struct ConnectionContext {
    MsQuicServer::Impl* owner{};
    HQUIC connection{};
    HQUIC controlStream{};
    ControlFrameParser parser{kMaxControlPayload};
    std::unique_ptr<ControlSession> session;
    std::mutex sendMutex;
    bool authenticated{false};
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

    auto nibble = [](char c) -> uint8_t {
        return (c >= '0' && c <= '9')
            ? static_cast<uint8_t>(c - '0')
            : static_cast<uint8_t>(c - 'A' + 10);
    };

    for (size_t i = 0; i < out.size(); ++i) {
        out[i] = static_cast<uint8_t>(
            (nibble(hex[i * 2]) << 4) | nibble(hex[i * 2 + 1]));
    }
    return true;
}

bool sendStream(MsQuicServer::Impl& impl, ConnectionContext& ctx,
                const uint8_t* data, size_t size) {
    if (!impl.api || !ctx.controlStream || !data ||
        size == 0 || size > UINT32_MAX) {
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

QUIC_STATUS QUIC_API streamCallback(
    HQUIC stream, void* context, QUIC_STREAM_EVENT* event);

QUIC_STATUS QUIC_API connectionCallback(
    HQUIC connection, void* context, QUIC_CONNECTION_EVENT* event);

QUIC_STATUS QUIC_API listenerCallback(
    HQUIC listener, void* context, QUIC_LISTENER_EVENT* event);

void finishConnection(ConnectionContext* ctx) {
    if (!ctx || !ctx->owner) return;

    auto* impl = ctx->owner;
    bool wasAuthenticated = false;

    {
        std::lock_guard lock(impl->stateMutex);
        wasAuthenticated = ctx->authenticated;
        if (impl->connectionContext == ctx) {
            impl->connectionContext = nullptr;
            impl->connected = false;
        }
    }

    if (wasAuthenticated && impl->callbacks.onAuthenticated) {
        impl->callbacks.onAuthenticated(false);
    }
    if (impl->callbacks.onClosed) {
        impl->callbacks.onClosed("quic-connection-closed");
    }

    delete ctx;
    impl->stateCv.notify_all();
}

QUIC_STATUS QUIC_API streamCallback(
    HQUIC stream, void* context, QUIC_STREAM_EVENT* event) {
    auto* ctx = static_cast<ConnectionContext*>(context);
    if (!ctx || !ctx->owner || !ctx->owner->api) {
        return QUIC_STATUS_SUCCESS;
    }

    auto& impl = *ctx->owner;

    switch (event->Type) {
    case QUIC_STREAM_EVENT_RECEIVE: {
        std::lock_guard lock(ctx->sendMutex);

        for (uint32_t i = 0; i < event->RECEIVE.BufferCount; ++i) {
            const auto& buffer = event->RECEIVE.Buffers[i];
            ControlMessage message;

            auto status = ctx->parser.push(
                buffer.Buffer,
                buffer.Length,
                message);

            while (status == ParseStatus::MessageReady) {
                if (!ctx->session) {
                    return QUIC_STATUS_INVALID_STATE;
                }

                const auto action = ctx->session->onMessage(message);

                if (!action.responseJson.empty()) {
                    const auto response =
                        makeControlFrame(action.responseType, action.responseJson);
                    if (response.empty() ||
                        !sendStream(impl, *ctx, response.data(), response.size())) {
                        impl.api->StreamShutdown(
                            stream,
                            QUIC_STREAM_SHUTDOWN_FLAG_ABORT |
                            QUIC_STREAM_SHUTDOWN_FLAG_IMMEDIATE,
                            0x100);
                        return QUIC_STATUS_SUCCESS;
                    }
                }

                if (message.type == second_screen::control::MessageType::Hello &&
                    action.accepted &&
                    impl.callbacks.createPairingChallenge) {
                    const std::string code =
                        impl.callbacks.createPairingChallenge(
                            ctx->session->identity().deviceId);
                    if (!code.empty() && impl.callbacks.onPairingChallenge) {
                        impl.callbacks.onPairingChallenge(
                            ctx->session->identity().deviceId,
                            code);
                    }
                }

                if (action.authenticated && !ctx->authenticated) {
                    ctx->authenticated = true;
                    if (impl.callbacks.onAuthenticated) {
                        impl.callbacks.onAuthenticated(true);
                    }
                }

                if (action.requestKeyframe && impl.callbacks.onKeyframeRequested) {
                    impl.callbacks.onKeyframeRequested();
                }

                if (action.close) {
                    impl.api->StreamShutdown(
                        stream,
                        QUIC_STREAM_SHUTDOWN_FLAG_ABORT |
                        QUIC_STREAM_SHUTDOWN_FLAG_IMMEDIATE,
                        0);
                    return QUIC_STATUS_SUCCESS;
                }

                status = ctx->parser.push(nullptr, 0, message);
            }

            if (status == ParseStatus::Invalid) {
                impl.api->StreamShutdown(
                    stream,
                    QUIC_STREAM_SHUTDOWN_FLAG_ABORT |
                    QUIC_STREAM_SHUTDOWN_FLAG_IMMEDIATE,
                    0x101);
                return QUIC_STATUS_SUCCESS;
            }
        }
        break;
    }

    case QUIC_STREAM_EVENT_SEND_COMPLETE:
        delete static_cast<OwnedSendBuffer*>(
            event->SEND_COMPLETE.ClientContext);
        break;

    case QUIC_STREAM_EVENT_PEER_SEND_ABORTED:
        impl.api->StreamShutdown(
            stream,
            QUIC_STREAM_SHUTDOWN_FLAG_ABORT |
            QUIC_STREAM_SHUTDOWN_FLAG_IMMEDIATE,
            0);
        break;

    case QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE:
        if (ctx->controlStream == stream) {
            ctx->controlStream = nullptr;
        }
        impl.api->StreamClose(stream);
        break;

    default:
        break;
    }

    return QUIC_STATUS_SUCCESS;
}

QUIC_STATUS QUIC_API connectionCallback(
    HQUIC connection, void* context, QUIC_CONNECTION_EVENT* event) {
    auto* ctx = static_cast<ConnectionContext*>(context);
    if (!ctx || !ctx->owner || !ctx->owner->api) {
        return QUIC_STATUS_SUCCESS;
    }

    auto& impl = *ctx->owner;

    switch (event->Type) {
    case QUIC_CONNECTION_EVENT_CONNECTED:
        impl.connected = true;
        break;

    case QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED:
        if ((event->PEER_STREAM_STARTED.Flags &
             QUIC_STREAM_OPEN_FLAG_UNIDIRECTIONAL) != 0 ||
            ctx->controlStream != nullptr) {
            impl.api->StreamShutdown(
                event->PEER_STREAM_STARTED.Stream,
                QUIC_STREAM_SHUTDOWN_FLAG_ABORT |
                QUIC_STREAM_SHUTDOWN_FLAG_IMMEDIATE,
                0x102);
            break;
        }

        ctx->controlStream = event->PEER_STREAM_STARTED.Stream;
        impl.api->SetCallbackHandler(
            ctx->controlStream,
            streamCallback,
            ctx);
        break;

    case QUIC_CONNECTION_EVENT_DATAGRAM_STATE_CHANGED:
        if (event->DATAGRAM_STATE_CHANGED.SendEnabled &&
            event->DATAGRAM_STATE_CHANGED.MaxSendLength > 0) {
            impl.maxSendLength =
                event->DATAGRAM_STATE_CHANGED.MaxSendLength;
        } else {
            impl.maxSendLength = 0;
        }
        break;

    case QUIC_CONNECTION_EVENT_DATAGRAM_RECEIVED:
        if (impl.callbacks.onVideoDatagram &&
            event->DATAGRAM_RECEIVED.Buffer) {
            const auto* b = event->DATAGRAM_RECEIVED.Buffer;
            impl.callbacks.onVideoDatagram(
                std::vector<uint8_t>(
                    b->Buffer,
                    b->Buffer + b->Length));
        }
        break;

    case QUIC_CONNECTION_EVENT_DATAGRAM_SEND_STATE_CHANGED:
        const auto state = event->DATAGRAM_SEND_STATE_CHANGED.State;
        if (QUIC_DATAGRAM_SEND_STATE_IS_FINAL(state)) {
            delete static_cast<OwnedSendBuffer*>(
                event->DATAGRAM_SEND_STATE_CHANGED.ClientContext);
        }
        break;

    case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE:
        impl.api->ConnectionClose(connection);
        finishConnection(ctx);
        break;

    default:
        break;
    }

    return QUIC_STATUS_SUCCESS;
}

QUIC_STATUS QUIC_API listenerCallback(
    HQUIC, void* context, QUIC_LISTENER_EVENT* event) {
    auto* impl = static_cast<MsQuicServer::Impl*>(context);
    if (!impl || !impl->api) {
        return QUIC_STATUS_SUCCESS;
    }

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
    security.validateSessionToken =
        [impl](const std::string& deviceId, const std::string& token) {
            return impl->callbacks.validateSessionToken &&
                   impl->callbacks.validateSessionToken(deviceId, token);
        };
    security.confirmPairingCode =
        [impl](const std::string& deviceId, const std::string& code) {
            return impl->callbacks.confirmPairingCode &&
                   impl->callbacks.confirmPairingCode(deviceId, code);
        };
    security.issueSessionToken =
        [impl](const std::string& deviceId) {
            return impl->callbacks.issueSessionToken
                ? impl->callbacks.issueSessionToken(deviceId)
                : std::string{};
        };

    ctx->session =
        std::make_unique<ControlSession>(std::move(security));

    impl->api->SetCallbackHandler(
        event->NEW_CONNECTION.Connection,
        connectionCallback,
        ctx);

    const QUIC_STATUS status =
        impl->api->ConnectionSetConfiguration(
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

} // namespace

#endif

MsQuicServer::MsQuicServer(
    Config config,
    quic::TransportCallbacks callbacks)
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
    if (!parseThumbprint(
            impl_->config.certificateThumbprint,
            thumbprint)) {
        return false;
    }

    QUIC_STATUS status = MsQuicOpen2(&impl_->api);
    if (QUIC_FAILED(status) || !impl_->api) {
        impl_->api = nullptr;
        return false;
    }

    QUIC_REGISTRATION_CONFIG registrationConfig{
        "SecondScreen",
        QUIC_EXECUTION_PROFILE_LOW_LATENCY
    };

    status = impl_->api->RegistrationOpen(
        &registrationConfig,
        &impl_->registration);
    if (QUIC_FAILED(status)) {
        MsQuicClose(impl_->api);
        impl_->api = nullptr;
        return false;
    }

    const uint8_t alpnBytes[] = {
        's','e','c','o','n','d','s','c','r','e','e','n','/','1'
    };
    QUIC_BUFFER alpn{
        static_cast<uint32_t>(sizeof(alpnBytes)),
        const_cast<uint8_t*>(alpnBytes)
    };

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
        impl_->registration = nullptr;
        MsQuicClose(impl_->api);
        impl_->api = nullptr;
        return false;
    }

    QUIC_CERTIFICATE_HASH certificateHash{};
    std::memcpy(
        certificateHash.ShaHash,
        thumbprint.data(),
        thumbprint.size());

    QUIC_CREDENTIAL_CONFIG credential{};
    credential.Type = QUIC_CREDENTIAL_TYPE_CERTIFICATE_HASH;
    credential.CertificateHash = &certificateHash;

    status = impl_->api->ConfigurationLoadCredential(
        impl_->configuration,
        &credential);
    if (QUIC_FAILED(status)) {
        impl_->api->ConfigurationClose(impl_->configuration);
        impl_->configuration = nullptr;
        impl_->api->RegistrationClose(impl_->registration);
        impl_->registration = nullptr;
        MsQuicClose(impl_->api);
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
        impl_->configuration = nullptr;
        impl_->api->RegistrationClose(impl_->registration);
        impl_->registration = nullptr;
        MsQuicClose(impl_->api);
        impl_->api = nullptr;
        return false;
    }

    QUIC_ADDR address{};
    QuicAddrSetFamily(
        &address,
        QUIC_ADDRESS_FAMILY_UNSPEC);
    QuicAddrSetPort(
        &address,
        impl_->config.port);

    status = impl_->api->ListenerStart(
        impl_->listener,
        &alpn,
        1,
        &address);
    if (QUIC_FAILED(status)) {
        impl_->api->ListenerClose(impl_->listener);
        impl_->listener = nullptr;
        impl_->api->ConfigurationClose(impl_->configuration);
        impl_->configuration = nullptr;
        impl_->api->RegistrationClose(impl_->registration);
        impl_->registration = nullptr;
        MsQuicClose(impl_->api);
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

    if (impl_->connectionContext) {
        impl_->api->ConnectionShutdown(
            impl_->connectionContext->connection,
            QUIC_CONNECTION_SHUTDOWN_FLAG_SILENT,
            0);
        std::unique_lock lock(impl_->stateMutex);
        impl_->stateCv.wait(lock, [this] {
            return impl_->connectionContext == nullptr;
        });
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

bool MsQuicServer::sendControl(
    const uint8_t* data, size_t size) {
#if !SECOND_SCREEN_HAS_MSQUIC
    (void)data;
    (void)size;
    return false;
#else
    if (!impl_->api || !data || size == 0) return false;

    std::lock_guard stateLock(impl_->stateMutex);
    auto* ctx = impl_->connectionContext;
    if (!ctx || !ctx->controlStream) return false;

    std::lock_guard sendLock(ctx->sendMutex);
    return sendStream(*impl_, *ctx, data, size);
#endif
}

bool MsQuicServer::sendVideoDatagram(
    const uint8_t* data, size_t size) {
#if !SECOND_SCREEN_HAS_MSQUIC
    (void)data;
    (void)size;
    return false;
#else
    if (!impl_->api || !data ||
        size == 0 ||
        size > impl_->maxSendLength.load()) {
        return false;
    }

    std::lock_guard stateLock(impl_->stateMutex);
    auto* connectionContext = impl_->connectionContext;
    if (!connectionContext) return false;

    auto* owned =
        new (std::nothrow) OwnedSendBuffer(data, size);
    if (!owned) return false;

    const QUIC_STATUS status =
        impl_->api->DatagramSend(
            connectionContext->connection,
            &owned->buffer,
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
    return impl_->connected.load();
}

quic::DatagramLimits MsQuicServer::datagramLimits() const {
    return {impl_->maxSendLength.load()};
}

}
