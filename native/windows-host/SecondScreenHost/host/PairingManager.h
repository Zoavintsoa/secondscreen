#pragma once
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

namespace second_screen {

struct PairingChallenge {
    std::string code;
    std::string deviceId;
    uint64_t expiresAtUnixMs;
};

class PairingManager {
public:
    PairingChallenge createChallenge(const std::string& deviceId);
    bool confirm(const std::string& deviceId, const std::string& code);
    std::string issueSessionToken(const std::string& deviceId);
    bool validateSessionToken(const std::string& deviceId, const std::string& token) const;
    void revoke(const std::string& deviceId);

private:
    static std::string randomBytesHex(size_t byteCount);
    static uint64_t nowUnixMs();

    mutable std::mutex mutex_;
    std::unordered_map<std::string, PairingChallenge> pending_;
    std::unordered_map<std::string, std::string> sessions_;
};

}
