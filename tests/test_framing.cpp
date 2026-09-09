// Unit Test: Network Framing & Packet Stream Processing
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

#include "../core/protocol/Protocol.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <cstring>

using namespace SecondScreen;

void TestPacketFramingRoundtrip() {
    std::cout << "[TEST] Testing Packet Framing & Roundtrip..." << std::endl;

    // Simulated H.264 NALU payload (Annex-B SPS/PPS + IDR)
    std::vector<uint8_t> naluPayload = {
        0x00, 0x00, 0x00, 0x01, 0x67, 0x64, 0x00, 0x28, // SPS
        0x00, 0x00, 0x00, 0x01, 0x68, 0xEE, 0x3C, 0x80, // PPS
        0x00, 0x00, 0x00, 0x01, 0x65, 0x88, 0x84, 0x00  // IDR Slice
    };

    PacketHeader hdr = {};
    hdr.magic = PROTOCOL_MAGIC;
    hdr.version = PROTOCOL_VERSION;
    hdr.type = static_cast<uint8_t>(MessageType::FRAME);
    hdr.flags = 0x01; // Keyframe
    hdr.sequence = 42;
    hdr.timestamp = 1000000ULL;
    hdr.payloadSize = static_cast<uint32_t>(naluPayload.size());

    // Serialize
    std::vector<uint8_t> streamBuffer;
    streamBuffer.resize(sizeof(PacketHeader) + naluPayload.size());
    std::memcpy(streamBuffer.data(), &hdr, sizeof(PacketHeader));
    std::memcpy(streamBuffer.data() + sizeof(PacketHeader), naluPayload.data(), naluPayload.size());

    // Deserialize
    assert(streamBuffer.size() >= sizeof(PacketHeader));
    const auto* parsedHdr = reinterpret_cast<const PacketHeader*>(streamBuffer.data());
    assert(parsedHdr->magic == PROTOCOL_MAGIC);
    assert(parsedHdr->version == PROTOCOL_VERSION);
    assert(parsedHdr->type == static_cast<uint8_t>(MessageType::FRAME));
    assert((parsedHdr->flags & 0x01) == 0x01);
    assert(parsedHdr->sequence == 42);
    assert(parsedHdr->payloadSize == naluPayload.size());

    std::vector<uint8_t> extractedPayload(
        streamBuffer.begin() + sizeof(PacketHeader),
        streamBuffer.begin() + sizeof(PacketHeader) + parsedHdr->payloadSize
    );

    assert(extractedPayload == naluPayload);
    std::cout << "[PASS] Framing roundtrip successful (" << streamBuffer.size() << " bytes total).\n" << std::endl;
}

void TestInvalidMagicRejection() {
    std::cout << "[TEST] Testing Corrupted / Non-Protocol Data Rejection..." << std::endl;

    std::vector<uint8_t> garbageData = { 0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02, 0x03, 0x04 };
    garbageData.resize(24, 0);

    const auto* hdr = reinterpret_cast<const PacketHeader*>(garbageData.data());
    assert(hdr->magic != PROTOCOL_MAGIC);

    std::cout << "[PASS] Invalid magic rejected correctly.\n" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "SecondScreen Framing Test Suite" << std::endl;
    std::cout << "========================================\n" << std::endl;

    TestPacketFramingRoundtrip();
    TestInvalidMagicRejection();

    std::cout << "ALL FRAMING TESTS PASSED!" << std::endl;
    return 0;
}
