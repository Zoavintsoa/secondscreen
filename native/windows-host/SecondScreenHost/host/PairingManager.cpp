#include "PairingManager.h"

#include <windows.h>
#include <bcrypt.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>

#pragma comment(lib, "Bcrypt.lib")

namespace second_screen {

namespace {
bool constantTimeEqual(const std::string& a, const std::string& b) {
    const size_t maxLen = std::max(a.size(), b.size());
    unsigned char diff = static_cast<unsigned char>(a.size() ^ b.size());
    for (size_t i = 0; i < maxLen; ++i) {
        const unsigned char av = i < a.size()
            ? static_cast<unsigned char>(a[i]) : 0;
        const unsigned char bv = i < b.size()
            ? static_cast<unsigned char>(b[i]) : 0;
        diff = static_cast<unsigned char>(diff | (av ^ bv));
    }
    return diff == 0;
}
} // namespace

uint64_t PairingManager::nowUnixMs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

std::string PairingManager::randomBytesHex(size_t byteCount) {
    std::string bytes(byteCount, '\0');
    if (BCryptGenRandom(nullptr,
                        reinterpret_cast<PUCHAR>(bytes.data()),
                        static_cast<ULONG>(bytes.size()),
                        BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) {
        return {};
    }

    static constexpr char hex[] = "0123456789abcdef";
    std::string out;
    out.reserve(byteCount * 2);
    for (unsigned char b : bytes) {
        out.push_back(hex[b >> 4]);
        out.push_back(hex[b & 0x0F]);
    }
    return out;
}

PairingChallenge PairingManager::createChallenge(const std::string& deviceId) {
    std::lock_guard lock(mutex_);

    const std::string entropy = randomBytesHex(4);
    if (entropy.empty()) return {};

    // Six digits are a human-friendly pairing code. It is short-lived and is
    // never used as the long-term session credential.
    const uint32_t value =
        static_cast<uint32_t>(std::stoul(entropy.substr(0, 8), nullptr, 16) % 900000u) + 100000u;

    PairingChallenge challenge{
        std::to_string(value),
        deviceId,
        nowUnixMs() + 120000
    };
    pending_[deviceId] = challenge;
    return challenge;
}

bool PairingManager::confirm(const std::string& deviceId, const std::string& code) {
    std::lock_guard lock(mutex_);
    const auto it = pending_.find(deviceId);
    if (it == pending_.end()) return false;

    const bool valid = constantTimeEqual(it->second.code, code) && nowUnixMs() <= it->second.expiresAtUnixMs;
    pending_.erase(it);
    return valid;
}

std::string PairingManager::issueSessionToken(const std::string& deviceId) {
    std::lock_guard lock(mutex_);
    const std::string token = randomBytesHex(32);
    if (token.empty()) return {};
    sessions_[deviceId] = token;
    return token;
}

bool PairingManager::validateSessionToken(const std::string& deviceId,
                                          const std::string& token) const {
    std::lock_guard lock(mutex_);
    const auto it = sessions_.find(deviceId);
    return it != sessions_.end() && !token.empty() && constantTimeEqual(it->second, token);
}

void PairingManager::revoke(const std::string& deviceId) {
    std::lock_guard lock(mutex_);
    sessions_.erase(deviceId);
    pending_.erase(deviceId);
}

}
