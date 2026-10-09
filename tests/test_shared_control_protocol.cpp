#include "../native/shared-protocol/CONTROL_PROTOCOL.h"
#include "../native/shared-protocol/CONTROL_SESSION.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using namespace second_screen::control;

static void testFragmentedAndCoalescedFrames() {
    const auto first = makeControlFrame(MessageType::Hello, R"({"deviceId":"device-1"})");
    const auto second = makeControlFrame(MessageType::Ping, R"({"nonce":7})");
    assert(!first.empty() && !second.empty());

    ControlFrameParser parser;
    ControlMessage message{};
    assert(parser.push(first.data(), 5, message) == ParseStatus::NeedMoreData);
    assert(parser.push(first.data() + 5, first.size() - 5, message) == ParseStatus::MessageReady);
    assert(message.type == MessageType::Hello);
    assert(message.json == R"({"deviceId":"device-1"})");

    std::vector<std::uint8_t> joined = first;
    joined.insert(joined.end(), second.begin(), second.end());
    parser.reset();
    assert(parser.push(joined.data(), joined.size(), message) == ParseStatus::MessageReady);
    assert(message.type == MessageType::Hello);
    assert(parser.push(nullptr, 0, message) == ParseStatus::MessageReady);
    assert(message.type == MessageType::Ping);
    assert(message.json == R"({"nonce":7})");
}

static void testMalformedFramesAreRejected() {
    auto badMagic = makeControlFrame(MessageType::Hello, "{}");
    badMagic[0] = 'X';
    ControlFrameParser magicParser;
    ControlMessage message{};
    assert(magicParser.push(badMagic.data(), badMagic.size(), message) == ParseStatus::Invalid);
    assert(magicParser.invalid());

    auto badVersion = makeControlFrame(MessageType::Hello, "{}");
    badVersion[4] = 2;
    ControlFrameParser versionParser;
    assert(versionParser.push(badVersion.data(), badVersion.size(), message) == ParseStatus::Invalid);

    const auto tooLarge = makeControlFrame(MessageType::Hello, R"({"123456789":"x"})");
    ControlFrameParser sizeParser(8);
    assert(sizeParser.push(tooLarge.data(), tooLarge.size(), message) == ParseStatus::Invalid);
}

static void testPropertyNamesInsideStringsCannotSpoofIdentity() {
    ControlSession session;
    const auto spoofed = session.onMessage({
        MessageType::Hello, 1, 0,
        R"({"deviceName":"text with \"deviceId\":\"attacker\""})"
    });
    assert(spoofed.close && !spoofed.accepted);
    assert(session.state() == SessionState::Disconnected);

    const auto nestedSpoof = session.onMessage({
        MessageType::Hello, 1, 0,
        R"({"metadata":{"deviceId":"attacker"}})"
    });
    assert(nestedSpoof.close && !nestedSpoof.accepted);
    assert(session.state() == SessionState::Disconnected);
}

static void testPairingAndAuthenticationGates() {
    const std::string token(64, 'b');
    ControlSession::SecurityCallbacks security{
        [token](const std::string& deviceId, const std::string& candidate) {
            return deviceId == "device-1" && candidate == token;
        },
        [](const std::string& deviceId, const std::string& code) {
            return deviceId == "device-1" && code == "123456";
        },
        [token](const std::string&) { return token; }
    };

    ControlSession session(security);
    auto denied = session.onMessage({MessageType::KeyframeRequest, 1, 0, "{}"});
    assert(denied.close && !denied.accepted);
    session.reset();

    auto hello = session.onMessage({
        MessageType::Hello, 1, 0,
        R"({"deviceId":"device-1","deviceName":"Camera \"A\"\\Lab\n1"})"
    });
    assert(hello.accepted && !hello.close);
    assert(hello.responseType == MessageType::Capabilities);
    assert(session.identity().deviceName == "Camera \"A\"\\Lab\n1");

    auto paired = session.onMessage({MessageType::PairRequest, 1, 0, R"({"code":"123456"})"});
    assert(paired.accepted && paired.authenticated && paired.requestKeyframe);
    assert(paired.responseType == MessageType::PairResponse);
    assert(paired.responseJson.find(token) != std::string::npos);
    assert(session.authenticated());

    session.reset();
    hello = session.onMessage({MessageType::Hello, 1, 0, R"({"deviceId":"device-1"})"});
    assert(hello.accepted);
    auto auth = session.onMessage({
        MessageType::Auth, 1, 0,
        std::string(R"({"sessionToken":")") + token + R"("})"
    });
    assert(auth.accepted && auth.authenticated);
    assert(auth.responseType == MessageType::StreamConfig);
}

int main() {
    testFragmentedAndCoalescedFrames();
    testMalformedFramesAreRejected();
    testPropertyNamesInsideStringsCannotSpoofIdentity();
    testPairingAndAuthenticationGates();
    std::cout << "Shared control protocol tests passed.\n";
    return 0;
}
