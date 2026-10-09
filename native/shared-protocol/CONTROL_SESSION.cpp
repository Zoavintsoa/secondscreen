#include "CONTROL_SESSION.h"

#include <cctype>
#include <cstdint>
#include <string>

namespace second_screen::control {
namespace {

bool isHex(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

uint32_t hexValue(char c) {
    if (c >= '0' && c <= '9') return static_cast<uint32_t>(c - '0');
    if (c >= 'a' && c <= 'f') return static_cast<uint32_t>(c - 'a' + 10);
    return static_cast<uint32_t>(c - 'A' + 10);
}

bool readHex4(std::string_view json, size_t& pos, uint32_t& value) {
    if (pos + 4 > json.size()) return false;
    value = 0;
    for (size_t i = 0; i < 4; ++i) {
        const char c = json[pos++];
        if (!isHex(c)) return false;
        value = (value << 4) | hexValue(c);
    }
    return true;
}

void appendUtf8(std::string& out, uint32_t cp) {
    if (cp <= 0x7F) {
        out.push_back(static_cast<char>(cp));
    } else if (cp <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
}

bool parseJsonString(std::string_view json, size_t& pos, std::string& out) {
    if (pos >= json.size() || json[pos] != '"') return false;
    ++pos;
    std::string value;
    while (pos < json.size()) {
        const unsigned char c = static_cast<unsigned char>(json[pos++]);
        if (c == '"') {
            out = std::move(value);
            return true;
        }
        if (c < 0x20) return false;
        if (c != '\\') {
            value.push_back(static_cast<char>(c));
            continue;
        }
        if (pos >= json.size()) return false;
        const char escaped = json[pos++];
        switch (escaped) {
        case '"': value.push_back('"'); break;
        case '\\': value.push_back('\\'); break;
        case '/': value.push_back('/'); break;
        case 'b': value.push_back('\b'); break;
        case 'f': value.push_back('\f'); break;
        case 'n': value.push_back('\n'); break;
        case 'r': value.push_back('\r'); break;
        case 't': value.push_back('\t'); break;
        case 'u': {
            uint32_t cp = 0;
            if (!readHex4(json, pos, cp)) return false;
            if (cp >= 0xD800 && cp <= 0xDBFF) {
                if (pos + 2 > json.size() || json[pos] != '\\' || json[pos + 1] != 'u') return false;
                pos += 2;
                uint32_t low = 0;
                if (!readHex4(json, pos, low) || low < 0xDC00 || low > 0xDFFF) return false;
                cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
            } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
                return false;
            }
            appendUtf8(value, cp);
            break;
        }
        default: return false;
        }
    }
    return false;
}

bool findValue(std::string_view json, std::string_view key, size_t& valueStart) {
    size_t pos = 0;
    while (pos < json.size()) {
        const size_t keyPos = json.find('"', pos);
        if (keyPos == std::string_view::npos) return false;

        // A property key must begin at an object/array delimiter context, not
        // inside another JSON string. This prevents key-like text in device names
        // from being mistaken for an actual property.
        size_t before = keyPos;
        while (before > 0 && std::isspace(static_cast<unsigned char>(json[before - 1]))) --before;
        if (before == 0 || (json[before - 1] != '{' && json[before - 1] != ',')) {
            pos = keyPos + 1;
            std::string ignored;
            if (!parseJsonString(json, pos, ignored)) return false;
            continue;
        }

        size_t after = keyPos;
        std::string parsedKey;
        if (!parseJsonString(json, after, parsedKey)) return false;
        while (after < json.size() && std::isspace(static_cast<unsigned char>(json[after]))) ++after;
        if (after < json.size() && json[after] == ':' && parsedKey == key) {
            valueStart = after + 1;
            while (valueStart < json.size() && std::isspace(static_cast<unsigned char>(json[valueStart]))) ++valueStart;
            return valueStart < json.size();
        }
        pos = after;
    }
    return false;
}

} // namespace

ControlSession::ControlSession(SecurityCallbacks security, uint32_t maxPayloadBytes)
    : security_(std::move(security)), parser_(maxPayloadBytes) {}

void ControlSession::reset() {
    state_ = SessionState::Disconnected;
    identity_ = {};
    parser_.reset();
}

bool ControlSession::extractString(std::string_view json, std::string_view key, std::string& out) {
    size_t pos = 0;
    if (!findValue(json, key, pos)) return false;
    return parseJsonString(json, pos, out);
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
        if (!extractString(message.json, "deviceName", deviceName)) deviceName.clear();
        identity_ = {std::move(deviceId), std::move(deviceName)};
        state_ = SessionState::HelloReceived;
        action.accepted = true;
        action.responseType = MessageType::Capabilities;
        action.responseJson = "{\"protocolMajor\":1,\"protocolMinor\":1,\"pairingRequired\":true,\"transport\":\"quic-preferred\"}";
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
        action.accepted = true;
        action.authenticated = true;
        action.requestKeyframe = true;
        action.responseType = MessageType::PairResponse;
        action.responseJson =
            std::string("{\"status\":\"paired\",\"sessionToken\":\"") + token +
            "\",\"codec\":\"h264\",\"width\":1920,\"height\":1080,\"fps\":60,"
            "\"bitrateKbps\":8000,\"maxLatencyMs\":50,\"profileId\":\"balanced\"}";
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
