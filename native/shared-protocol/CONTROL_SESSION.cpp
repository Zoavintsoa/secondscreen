#include "CONTROL_SESSION.h"

#include <algorithm>
#include <cctype>

namespace second_screen::control {

namespace {
std::string trim(std::string_view value) {
    size_t begin = 0;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin]))) ++begin;
    size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1]))) --end;
    return std::string(value.substr(begin, end - begin));
}

bool findValue(std::string_view json, std::string_view key, size_t& valueStart) {
    const std::string needle = """ + std::string(key) + """;
    const size_t keyPos = json.find(needle);
    if (keyPos == std::string_view::npos) return false;
    size_t colon = json.find(':', keyPos + needle.size());
    if (colon == std::string_view::npos) return false;
    valueStart = colon + 1;
    while (valueStart < json.size() && std::isspace(static_cast<unsigned char>(json[valueStart]))) ++valueStart;
    return valueStart < json.size();
}
}

ControlSession::ControlSession(SecurityCallbacks security, uint32_t maxPayloadBytes)
    : security_(std::move(security)), parser_(maxPayloadBytes) {}

void ControlSession::reset() {
    state_ = SessionState::Disconnected;
    identity_ = {};
    awaitingPairConfirmation_ = false;
    parser_.reset();
}

bool ControlSession::extractString(std::string_view json, std::string_view key, std::string& out) {
    size_t pos = 0;
    if (!findValue(json, key, pos) || json[pos] != '"') return false;
    ++pos;
    std::string value;
    bool escaped = false;
    for (; pos < json.size(); ++pos) {
        const char c = json[pos];
        if (escaped) {
            value.push_back(c);
            escaped = false;
            continue;
        }
        if (c == '\\') {
            escaped = true;
            continue;
        }
        if (c == '"') {
            out = std::move(value);
            return true;
        }
        value.push_back(c);
    }
    return false;
}

bool ControlSession::extractBool(std::string_view json, std::string_view key, bool& out) {
    size_t pos = 0;
    if (!findValue(json, key, pos)) return false;
    if (json.compare(pos, 4, "true") == 0) {
        out = true;
        return true;
    }
    if (json.compare(pos, 5, "false") == 0) {
        out = false;
        return true;
    }
    return false;
}

SessionAction ControlSession::onMessage(const ControlMessage& message) {
    SessionAction action;

    switch (message.type) {
    case MessageType::Hello: {
        if (state_ != SessionState::Disconnected) {
            action.close = true;
            return action;
        }

        std::string deviceId;
        if (!extractString(message.json, "deviceId", deviceId) || deviceId.empty() || deviceId.size() > 128) {
            action.close = true;
            return action;
        }

        std::string deviceName;
        extractString(message.json, "deviceName", deviceName);
        identity_ = {std::move(deviceId), std::move(deviceName)};
        state_ = SessionState::HelloReceived;
        action.accepted = true;
        action.responseType = MessageType::Capabilities;
        action.responseJson = "{"protocolMajor":1,"protocolMinor":1,"pairingRequired":true}";
        return action;
    }

    case MessageType::Auth: {
        if (state_ != SessionState::HelloReceived) {
            action.close = true;
            return action;
        }
        std::string token;
        if (!extractString(message.json, "sessionToken", token) || token.size() != 64) {
            action.close = true;
            return action;
        }
        if (!security_.validateSessionToken ||
            !security_.validateSessionToken(identity_.deviceId, token)) {
            action.close = true;
            return action;
        }
        state_ = SessionState::Authenticated;
        action.accepted = true;
        action.authenticated = true;
        action.requestKeyframe = true;
        action.responseType = MessageType::StreamConfig;
        action.responseJson =
            "{"codec":"h264","width":1920,"height":1080,"fps":60,"
            ""bitrateKbps":8000,"maxLatencyMs":50,"profileId":"balanced"}";
        return action;
    }

    case MessageType::PairRequest: {
        if (state_ != SessionState::HelloReceived) {
            action.close = true;
            return action;
        }
        bool confirmed = false;
        if (!extractBool(message.json, "confirmed", confirmed) || !confirmed) {
            action.close = true;
            return action;
        }
        awaitingPairConfirmation_ = true;
        state_ = SessionState::PairingPending;
        action.accepted = true;
        action.responseType = MessageType::PairResponse;
        action.responseJson = "{"status":"awaiting_host_confirmation"}";
        return action;
    }

    case MessageType::KeyframeRequest:
        if (!authenticated()) {
            action.close = true;
            return action;
        }
        action.accepted = true;
        action.requestKeyframe = true;
        return action;

    case MessageType::Ping:
        if (!authenticated()) {
            action.close = true;
            return action;
        }
        action.accepted = true;
        action.responseType = MessageType::Pong;
        action.responseJson = message.json;
        return action;

    case MessageType::Close:
        state_ = SessionState::Closed;
        action.accepted = true;
        action.close = true;
        return action;

    default:
        if (!authenticated()) {
            action.close = true;
            return action;
        }
        action.accepted = true;
        return action;
    }
}

} // namespace second_screen::control
