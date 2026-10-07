#pragma once
#include <string>
#include <unordered_map>
namespace second_screen {
struct PairingChallenge { std::string code; std::string deviceId; };
class PairingManager {
public:
    PairingChallenge createChallenge(const std::string& deviceId);
    bool confirm(const std::string& deviceId, const std::string& code);
    std::string issueSessionToken(const std::string& deviceId);
private:
    std::unordered_map<std::string, std::string> pending_;
    std::unordered_map<std::string, std::string> sessions_;
};
}
