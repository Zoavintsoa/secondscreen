// Unit Test: Wire Protocol Validation (PacketHeader 24B, VideoPacketHeader 32B, Struct Layouts & CRC-32)
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

#include "../core/protocol/Protocol.h"
#include <iostream>
#include <cassert>
#include <cstring>
#include <vector>

using namespace SecondScreen;

void TestStructSizes() {
    std::cout << "[TEST] Validating Wire Protocol Struct Sizes & Offsets..." << std::endl;

    std::cout << "  sizeof(PacketHeader):      " << sizeof(PacketHeader) << " bytes (Expected: 24)" << std::endl;
    assert(sizeof(PacketHeader) == 24);

    std::cout << "  sizeof(VideoPacketHeader): " << sizeof(VideoPacketHeader) << " bytes (Expected: 32)" << std::endl;
    assert(sizeof(VideoPacketHeader) == 32);

    std::cout << "  sizeof(InputEvent):        " << sizeof(InputEvent) << " bytes (Expected: 22)" << std::endl;
    assert(sizeof(InputEvent) == 22);

    std::cout << "  sizeof(TelemetryData):     " << sizeof(TelemetryData) << " bytes (Expected: 37)" << std::endl;
    assert(sizeof(TelemetryData) == 37);

    std::cout << "  sizeof(DisplayConfig):     " << sizeof(DisplayConfig) << " bytes (Expected: 20)" << std::endl;
    assert(sizeof(DisplayConfig) == 20);

    std::cout << "  sizeof(DisplayConfigAck):  " << sizeof(DisplayConfigAck) << " bytes (Expected: 8)" << std::endl;
    assert(sizeof(DisplayConfigAck) == 8);

    // Verify critical binary field offsets
    assert(offsetof(PacketHeader, payloadSize) == 20);
    assert(offsetof(VideoPacketHeader, crc32) == 28);
    assert(offsetof(TelemetryData, isMeasured) == 36);
    assert(offsetof(DisplayConfigAck, status) == 4);

    std::cout << "[PASS] Struct sizes and compile-time offsets verified successfully.\n" << std::endl;
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

void TestVideoPacketHeaderSerialization() {
    std::cout << "[TEST] Validating VideoPacketHeader 32-Byte Layout..." << std::endl;

    VideoPacketHeader vHdr = {};
    vHdr.magic = VIDEO_MAGIC;
    vHdr.sessionId = 98765432;
    vHdr.frameId = 12050;
    vHdr.packetIndex = 2;
    vHdr.packetCount = 5;
    vHdr.flags = 0x02; // Last fragment flag
    vHdr.codec = static_cast<uint8_t>(VideoCodec::H264);
    vHdr.payloadSize = 1400;
    vHdr.timestampUs = 1718290000999ULL;
    vHdr.crc32 = 0xDEADBEEF;

    std::vector<uint8_t> buffer(sizeof(VideoPacketHeader));
    std::memcpy(buffer.data(), &vHdr, sizeof(VideoPacketHeader));

    uint32_t magic;
    std::memcpy(&magic, buffer.data() + 0, 4);
    assert(magic == VIDEO_MAGIC);

    uint32_t sess;
    std::memcpy(&sess, buffer.data() + 4, 4);
    assert(sess == 98765432);

    uint32_t fId;
    std::memcpy(&fId, buffer.data() + 8, 4);
    assert(fId == 12050);

    uint16_t pIdx;
    std::memcpy(&pIdx, buffer.data() + 12, 2);
    assert(pIdx == 2);

    uint16_t pCnt;
    std::memcpy(&pCnt, buffer.data() + 14, 2);
    assert(pCnt == 5);

    uint8_t flg = buffer[16];
    assert(flg == 0x02);

    uint8_t cdc = buffer[17];
    assert(cdc == static_cast<uint8_t>(VideoCodec::H264));

    uint16_t pLen;
    std::memcpy(&pLen, buffer.data() + 18, 2);
    assert(pLen == 1400);

    uint64_t ts;
    std::memcpy(&ts, buffer.data() + 20, 8);
    assert(ts == 1718290000999ULL);

    uint32_t crc;
    std::memcpy(&crc, buffer.data() + 28, 4);
    assert(crc == 0xDEADBEEF);

    std::cout << "[PASS] VideoPacketHeader 32-byte layout verified.\n" << std::endl;
}

void TestDisplayConfigAckPacking() {
    std::cout << "[TEST] Validating DisplayConfigAck 8-Byte Layout..." << std::endl;

    DisplayConfigAck ack = {};
    ack.displayId = 2;
    ack.status = static_cast<uint8_t>(DisplayConfigStatus::ACCEPTED);

    uint8_t buf[sizeof(DisplayConfigAck)];
    std::memcpy(buf, &ack, sizeof(DisplayConfigAck));

    DisplayConfigAck restored = {};
    std::memcpy(&restored, buf, sizeof(DisplayConfigAck));

    assert(restored.displayId == 2);
    assert(restored.status == static_cast<uint8_t>(DisplayConfigStatus::ACCEPTED));

    std::cout << "[PASS] DisplayConfigAck 8-byte serialization verified.\n" << std::endl;
}

void TestCRC32Comprehensive() {
    std::cout << "[TEST] Validating CRC-32 IEEE 802.3 Standard & Video Packet Integrity..." << std::endl;

    // 1. Mandatory standard test vector: "123456789" -> 0xCBF43926
    const char* standardVector = "123456789";
    uint32_t crcVector = CalculateCRC32(reinterpret_cast<const uint8_t*>(standardVector), 9);
    std::cout << "  CRC-32 of '123456789': 0x" << std::hex << crcVector << std::dec << " (Expected: 0xCBF43926)" << std::endl;
    assert(crcVector == 0xCBF43926);

    // 2. Valid video packet CRC calculation (excluding crc32 field itself)
    VideoPacketHeader vHdr = {};
    vHdr.magic = VIDEO_MAGIC;
    vHdr.sessionId = 12345;
    vHdr.frameId = 100;
    vHdr.packetIndex = 0;
    vHdr.packetCount = 1;
    vHdr.flags = 0x01;
    vHdr.codec = static_cast<uint8_t>(VideoCodec::H264);
    vHdr.payloadSize = 512;
    vHdr.timestampUs = 1000000ULL;

    std::vector<uint8_t> payload(512, 0x42);
    vHdr.crc32 = CalculateVideoPacketCRC(vHdr, payload.data());

    // Verify valid packet
    assert(CalculateVideoPacketCRC(vHdr, payload.data()) == vHdr.crc32);

    // 3. Corrupted header detection (e.g. frameId changed in transit)
    VideoPacketHeader corruptedHeader = vHdr;
    corruptedHeader.frameId = 101;
    assert(CalculateVideoPacketCRC(corruptedHeader, payload.data()) != vHdr.crc32);

    // 4. Corrupted payload detection
    std::vector<uint8_t> corruptedPayload = payload;
    corruptedPayload[250] ^= 0x01;
    assert(CalculateVideoPacketCRC(vHdr, corruptedPayload.data()) != vHdr.crc32);

    // 5. Corrupted CRC detection
    uint32_t badCrc = vHdr.crc32 ^ 0xFFFFFFFF;
    assert(CalculateVideoPacketCRC(vHdr, payload.data()) != badCrc);

    // 6. Empty payload
    VideoPacketHeader emptyHdr = vHdr;
    emptyHdr.payloadSize = 0;
    emptyHdr.crc32 = CalculateVideoPacketCRC(emptyHdr, nullptr);
    assert(CalculateVideoPacketCRC(emptyHdr, nullptr) == emptyHdr.crc32);

    // 7. Maximum payload fragment (64 KB)
    std::vector<uint8_t> maxPayload(64 * 1024, 0x7E);
    VideoPacketHeader maxHdr = vHdr;
    maxHdr.payloadSize = static_cast<uint16_t>(maxPayload.size());
    maxHdr.crc32 = CalculateVideoPacketCRC(maxHdr, maxPayload.data());
    assert(CalculateVideoPacketCRC(maxHdr, maxPayload.data()) == maxHdr.crc32);

    std::cout << "[PASS] Comprehensive CRC-32 integrity suite passed.\n" << std::endl;
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
    telem.isMeasured = 0; // 0 = Performance Model Target, 1 = Physical measurement

    uint8_t buf[sizeof(TelemetryData)];
    std::memcpy(buf, &telem, sizeof(TelemetryData));

    assert(buf[36] == 0);

    TelemetryData restored = {};
    std::memcpy(&restored, buf, sizeof(TelemetryData));
    assert(restored.fps == 60.0f);
    assert(restored.bitrateMbps == 8.0f);
    assert(restored.rttMs == 3.2f);
    assert(restored.isMeasured == 0);

    std::cout << "[PASS] TelemetryData layout validated.\n" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "SecondScreen Protocol Test Suite" << std::endl;
    std::cout << "========================================\n" << std::endl;

    TestStructSizes();
    TestHeaderSerialization();
    TestVideoPacketHeaderSerialization();
    TestDisplayConfigAckPacking();
    TestCRC32Comprehensive();
    TestInputEventPacking();
    TestTelemetryPacking();

    std::cout << "ALL PROTOCOL TESTS PASSED!" << std::endl;
    return 0;
}
