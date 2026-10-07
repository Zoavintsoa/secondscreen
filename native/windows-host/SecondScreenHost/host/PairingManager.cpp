#include "PairingManager.h"
#include <random>
namespace second_screen {
PairingChallenge PairingManager::createChallenge(const std::string& deviceId) {
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(100000, 999999);
    const std::string code = std::to_string(dist(rng));
    pending_[deviceId] = code;
    return {code, deviceId};
}
bool PairingManager::confirm(const std::string& deviceId, const std::string& code) {
    auto it = pending_.find(deviceId);
    if (it == pending_.end() || it->second != code) return false;
    pending_.erase(it);
    return true;
}
std::string PairingManager::issueSessionToken(const std::string& deviceId) {
    static std::mt19937_64 rng{std::random_device{}()};
    const auto value = rng();
    const std::string token = std::to_string(value) + "-" + std::to_string(rng());
    sessions_[deviceId] = token;
    return token;
}
}
