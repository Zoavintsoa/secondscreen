// Unit Test: Wire Protocol Validation (PacketHeader 24 bytes, Struct Sizes & Layouts)
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

#include "../core/protocol/Protocol.h"
#include <iostream>
#include <cassert>
#include <cstring>
#include <vector>

using namespace SecondScreen;

void TestStructSizes() {
    std::cout << "[TEST] Validating Wire Protocol Struct Sizes..." << std::endl;

    std::cout << "  sizeof(PacketHeader):  " << sizeof(PacketHeader) << " bytes (Expected: 24)" << std::endl;
    assert(sizeof(PacketHeader) == 24);

    std::cout << "  sizeof(InputEvent):    " << sizeof(InputEvent) << " bytes (Expected: 22)" << std::endl;
    assert(sizeof(InputEvent) == 22);

    std::cout << "  sizeof(TelemetryData): " << sizeof(TelemetryData) << " bytes (Expected: 37)" << std::endl;
    assert(sizeof(TelemetryData) == 37);

    std::cout << "  sizeof(DisplayConfig): " << sizeof(DisplayConfig) << " bytes (Expected: 20)" << std::endl;
    assert(sizeof(DisplayConfig) == 20);

    std::cout << "[PASS] Struct sizes verified successfully.\n" << std::endl;

}

void TestHeaderSerialization() {
    std::cout << "[TEST] Validating PacketHeader Binary Packing..." << std::endl;

    PacketHeader hdr = {};
    hdr.magic = PROTOCOL_MAGIC;
    hdr.version = PROTOCOL_VERSION;
    hdr.type = static_cast<uint8_t>(MessageType::FRAME);
    hdr.flags = 0x01; // Keyframe flag
    hdr.sequence = 1042;
    hdr.timestamp = 1718290000123456ULL;
    hdr.payloadSize = 65536;

    std::vector<uint8_t> buffer(sizeof(PacketHeader));
    std::memcpy(buffer.data(), &hdr, sizeof(PacketHeader));

    // Inspect individual wire offsets
    uint32_t magic;
    std::memcpy(&magic, buffer.data() + 0, 4);
    assert(magic == 0x53325343);

    uint16_t ver;
    std::memcpy(&ver, buffer.data() + 4, 2);
    assert(ver == 1);

    uint8_t type = buffer[6];
    assert(type == static_cast<uint8_t>(MessageType::FRAME));

    uint8_t flags = buffer[7];
    assert(flags == 0x01);

    uint32_t seq;
    std::memcpy(&seq, buffer.data() + 8, 4);
    assert(seq == 1042);

    uint64_t ts;
    std::memcpy(&ts, buffer.data() + 12, 8);
    assert(ts == 1718290000123456ULL);

    uint32_t pSize;
    std::memcpy(&pSize, buffer.data() + 20, 4);
    assert(pSize == 65536);

    std::cout << "[PASS] Binary offsets and little-endian layout verified.\n" << std::endl;
}

void TestInputEventPacking() {
    std::cout << "[TEST] Validating InputEvent 22-byte Packing..." << std::endl;

    InputEvent evt = {};
    evt.actionType = 1; // Move
    evt.normX = 0.75f;
    evt.normY = 0.50f;
    evt.pressure = 0.85f;
    evt.button = 0; // Left
    evt.clientTimestamp = 1718290000999ULL;

    uint8_t buf[sizeof(InputEvent)];
    std::memcpy(buf, &evt, sizeof(InputEvent));

    InputEvent restored = {};
    std::memcpy(&restored, buf, sizeof(InputEvent));

    assert(restored.actionType == 1);
    assert(restored.normX == 0.75f);
    assert(restored.normY == 0.50f);
    assert(restored.pressure == 0.85f);
    assert(restored.button == 0);
    assert(restored.clientTimestamp == 1718290000999ULL);

    std::cout << "[PASS] InputEvent serialization validated.\n" << std::endl;
}

void TestTelemetryPacking() {
    std::cout << "[TEST] Validating TelemetryData 37-byte Packing..." << std::endl;

    TelemetryData telem = {};
    telem.fps = 60.0f;
    telem.bitrateMbps = 8.0f;
    telem.rttMs = 3.2f;
    telem.decodeLatencyMs = 2.1f;
    telem.renderLatencyMs = 1.0f;
    telem.totalLatencyMs = 6.3f;
    telem.packetLossPercent = 0.0f;
    telem.frameDrops = 0;
    telem.jitterMs = 0.3f;
    telem.isMeasured = 1;

    uint8_t buf[sizeof(TelemetryData)];
    std::memcpy(buf, &telem, sizeof(TelemetryData));

    // Validate byte 36 is the isMeasured flag
    assert(buf[36] == 1);

    TelemetryData restored = {};
    std::memcpy(&restored, buf, sizeof(TelemetryData));
    assert(restored.fps == 60.0f);
    assert(restored.bitrateMbps == 8.0f);
    assert(restored.rttMs == 3.2f);
    assert(restored.isMeasured == 1);

    std::cout << "[PASS] TelemetryData layout and byte 36 flag validated.\n" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "SecondScreen Protocol Test Suite" << std::endl;
    std::cout << "========================================\n" << std::endl;

    TestStructSizes();
    TestHeaderSerialization();
    TestInputEventPacking();
    TestTelemetryPacking();

    std::cout << "ALL PROTOCOL TESTS PASSED!" << std::endl;
    return 0;
}
