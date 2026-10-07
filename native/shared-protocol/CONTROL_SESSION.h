#pragma once

#include "CONTROL_PROTOCOL.h"
#include <cstdint>
#include <string>
#include <functional>
#include <utility>

namespace second_screen::control {

enum class SessionState {
    Disconnected,
    HelloReceived,
    PairingPending,
    Authenticated,
    Streaming,
    Closed
};

struct SessionIdentity {
    std::string deviceId;
    std::string deviceName;
};

struct SessionAction {
    bool accepted{false};
    bool close{false};
    bool requestKeyframe{false};
    bool authenticated{false};
    std::string responseJson;
    MessageType responseType{MessageType::Close};
};

class ControlSession {
public:
    struct SecurityCallbacks {
        std::function<bool(const std::string&, const std::string&)> validateSessionToken;
        std::function<bool(const std::string&, const std::string&)> confirmPairingCode;
        std::function<std::string(const std::string&)> issueSessionToken;
    };

    explicit ControlSession(
        SecurityCallbacks security = {},
        uint32_t maxPayloadBytes = kDefaultMaxPayloadBytes);

    void reset();
    SessionAction onMessage(const ControlMessage& message);

    SessionState state() const noexcept { return state_; }
    const SessionIdentity& identity() const noexcept { return identity_; }
    bool authenticated() const noexcept { return state_ == SessionState::Authenticated ||
                                                  state_ == SessionState::Streaming; }

private:
    static bool extractString(std::string_view json, std::string_view key, std::string& out);
    static bool extractBool(std::string_view json, std::string_view key, bool& out);

    SessionState state_{SessionState::Disconnected};
    SessionIdentity identity_;
    bool awaitingPairConfirmation_{false};
    ControlFrameParser parser_;
};

} // namespace second_screen::control
