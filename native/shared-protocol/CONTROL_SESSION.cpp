#include "CONTROL_SESSION.h"
#include <cctype>
#include <string>

namespace second_screen::control {
namespace {
bool findValue(std::string_view json, std::string_view key, size_t& valueStart) {
    const std::string needle = std::string("\"") + std::string(key) + "\"";
    const size_t keyPos = json.find(needle);
    if (keyPos == std::string_view::npos) return false;
    const size_t colon = json.find(':', keyPos + needle.size());
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
        if (escaped) { value.push_back(c); escaped = false; continue; }
        if (c == '\\') { escaped = true; continue; }
        if (c == '"') { out = std::move(value); return true; }
        value.push_back(c);
    }
    return false;
}

bool ControlSession::extractBool(std::string_view json, std::string_view key, bool& out) {
    size_t pos = 0;
    if (!findValue(json, key, pos)) return false;
    if (json.compare(pos, 4, "true") == 0) { out = true; return true; }
    if (json.compare(pos, 5, "false") == 0) { out = false; return true; }
    return false;
}

SessionAction ControlSession::onMessage(const ControlMessage& message) {
    SessionAction action;
    switch (message.type) {
    case MessageType::Hello: {
        if (state_ != SessionState::Disconnected) { action.close = true; return action; }
        std::string deviceId;
        if (!extractString(message.json, "deviceId", deviceId) || deviceId.empty() || deviceId.size() > 128) {
            action.close = true; return action;
        }
        std::string deviceName;
        extractString(message.json, "deviceName", deviceName);
        identity_ = {std::move(deviceId), std::move(deviceName)};
        state_ = SessionState::HelloReceived;
        action.accepted = true;
        action.responseType = MessageType::Capabilities;
        action.responseJson = "{\"protocolMajor\":1,\"protocolMinor\":1,\"pairingRequired\":true,\"transport\":\"tcp-bringup\"}";
        return action;
    }
    case MessageType::PairRequest: {
        if (state_ != SessionState::HelloReceived) { action.close = true; return action; }
        std::string code;
        if (!extractString(message.json, "code", code) || code.size() != 6 ||
            !security_.confirmPairingCode || !security_.confirmPairingCode(identity_.deviceId, code)) {
            action.close = true; return action;
        }
        if (!security_.issueSessionToken) { action.close = true; return action; }
        const std::string token = security_.issueSessionToken(identity_.deviceId);
        if (token.size() != 64) { action.close = true; return action; }
        state_ = SessionState::Authenticated;
        action.accepted = true; action.authenticated = true; action.requestKeyframe = true;
        action.responseType = MessageType::PairResponse;
        action.responseJson = std::string("{\"status\":\"paired\",\"sessionToken\":\"") + token + "\"}";
        return action;
    }
    case MessageType::Auth: {
        if (state_ != SessionState::HelloReceived) { action.close = true; return action; }
        std::string token;
        if (!extractString(message.json, "sessionToken", token) || token.size() != 64 ||
            !security_.validateSessionToken || !security_.validateSessionToken(identity_.deviceId, token)) {
            action.close = true; return action;
        }
        state_ = SessionState::Authenticated;
        action.accepted = true; action.authenticated = true; action.requestKeyframe = true;
        action.responseType = MessageType::StreamConfig;
        action.responseJson = "{\"codec\":\"h264\",\"width\":1920,\"height\":1080,\"fps\":60,\"bitrateKbps\":8000,\"maxLatencyMs\":50,\"profileId\":\"balanced\"}";
        return action;
    }
    case MessageType::KeyframeRequest:
        if (!authenticated()) { action.close = true; return action; }
        action.accepted = true; action.requestKeyframe = true; return action;
    case MessageType::Ping:
        if (!authenticated()) { action.close = true; return action; }
        action.accepted = true; action.responseType = MessageType::Pong; action.responseJson = message.json; return action;
    case MessageType::Close:
        state_ = SessionState::Closed; action.accepted = true; action.close = true; return action;
    default:
        if (!authenticated()) { action.close = true; return action; }
        action.accepted = true; return action;
    }
}
} // namespace second_screen::control
