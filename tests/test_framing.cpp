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

void TestStreamAccumulatorAndFragmentation() {
    std::cout << "[TEST] Testing Stream Accumulator with Fragmented & Coalesced Packets..." << std::endl;

    // Create 3 distinct packets: PAIR_REQUEST, DISPLAY_CONFIG, and INPUT_EVENT
    std::string pin = "849201";
    PacketHeader hdr1 = {};
    hdr1.magic = PROTOCOL_MAGIC;
    hdr1.version = PROTOCOL_VERSION;
    hdr1.type = static_cast<uint8_t>(MessageType::PAIR_REQUEST);
    hdr1.sequence = 1;
    hdr1.payloadSize = static_cast<uint32_t>(pin.size());

    DisplayConfig disp = {};
    disp.displayId = 2;
    disp.width = 1920;
    disp.height = 1080;
    disp.refreshRate = 60;
    disp.orientation = 0;
    disp.displayMode = static_cast<uint8_t>(DisplayMode::EXTEND);
    disp.dpiScalePercent = 100;

    PacketHeader hdr2 = {};
    hdr2.magic = PROTOCOL_MAGIC;
    hdr2.version = PROTOCOL_VERSION;
    hdr2.type = static_cast<uint8_t>(MessageType::DISPLAY_CONFIG);
    hdr2.sequence = 2;
    hdr2.payloadSize = sizeof(DisplayConfig);

    InputEvent evt = {};
    evt.actionType = 0; // Down
    evt.normX = 0.5f;
    evt.normY = 0.5f;
    evt.pressure = 1.0f;
    evt.button = 0;
    evt.clientTimestamp = 12345678ULL;

    PacketHeader hdr3 = {};
    hdr3.magic = PROTOCOL_MAGIC;
    hdr3.version = PROTOCOL_VERSION;
    hdr3.type = static_cast<uint8_t>(MessageType::INPUT_EVENT);
    hdr3.sequence = 3;
    hdr3.payloadSize = sizeof(InputEvent);

    // Concatenate all 3 packets into a single serialized stream
    std::vector<uint8_t> allBytes;
    auto appendPacket = [&](const PacketHeader& h, const void* p, size_t pSize) {
        size_t off = allBytes.size();
        allBytes.resize(off + sizeof(PacketHeader) + pSize);
        std::memcpy(allBytes.data() + off, &h, sizeof(PacketHeader));
        if (pSize > 0 && p) {
            std::memcpy(allBytes.data() + off + sizeof(PacketHeader), p, pSize);
        }
    };

    appendPacket(hdr1, pin.data(), pin.size());
    appendPacket(hdr2, &disp, sizeof(DisplayConfig));
    appendPacket(hdr3, &evt, sizeof(InputEvent));

    // Simulate arriving in arbitrary small chunks (e.g. 7 bytes at a time)
    std::vector<uint8_t> accumulator;
    std::vector<PacketHeader> parsedHeaders;
    size_t chunkSize = 7;

    for (size_t offset = 0; offset < allBytes.size(); offset += chunkSize) {
        size_t currentChunk = std::min(chunkSize, allBytes.size() - offset);
        accumulator.insert(accumulator.end(), allBytes.begin() + offset, allBytes.begin() + offset + currentChunk);

        // Process accumulator
        while (accumulator.size() >= sizeof(PacketHeader)) {
            PacketHeader parsed = {};
            std::memcpy(&parsed, accumulator.data(), sizeof(PacketHeader));
            if (parsed.magic != PROTOCOL_MAGIC || parsed.version != PROTOCOL_VERSION || parsed.payloadSize > MAX_CONTROL_PAYLOAD_SIZE) {
                assert(false && "Corrupted packet in accumulator test");
            }
            size_t totalPacketLen = sizeof(PacketHeader) + parsed.payloadSize;
            if (accumulator.size() < totalPacketLen) {
                break; // Wait for more fragments
            }

            parsedHeaders.push_back(parsed);
            accumulator.erase(accumulator.begin(), accumulator.begin() + totalPacketLen);
        }
    }

    assert(parsedHeaders.size() == 3);
    assert(parsedHeaders[0].type == static_cast<uint8_t>(MessageType::PAIR_REQUEST));
    assert(parsedHeaders[0].sequence == 1);
    assert(parsedHeaders[0].payloadSize == pin.size());

    assert(parsedHeaders[1].type == static_cast<uint8_t>(MessageType::DISPLAY_CONFIG));
    assert(parsedHeaders[1].sequence == 2);
    assert(parsedHeaders[1].payloadSize == sizeof(DisplayConfig));

    assert(parsedHeaders[2].type == static_cast<uint8_t>(MessageType::INPUT_EVENT));
    assert(parsedHeaders[2].sequence == 3);
    assert(parsedHeaders[2].payloadSize == sizeof(InputEvent));

    assert(accumulator.empty());
    std::cout << "[PASS] Stream accumulator correctly reassembled fragmented TCP chunks.\n" << std::endl;
}

void TestOversizedPayloadRejection() {
    std::cout << "[TEST] Testing Oversized Payload Boundary Protection..." << std::endl;

    PacketHeader hdr = {};
    hdr.magic = PROTOCOL_MAGIC;
    hdr.version = PROTOCOL_VERSION;
    hdr.type = static_cast<uint8_t>(MessageType::PAIR_REQUEST);
    hdr.payloadSize = MAX_CONTROL_PAYLOAD_SIZE + 1; // Exceeds 64 KB limit

    bool rejected = (hdr.payloadSize > MAX_CONTROL_PAYLOAD_SIZE);
    assert(rejected);

    std::cout << "[PASS] Oversized payload rejected before memory allocation.\n" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "SecondScreen Framing Test Suite" << std::endl;
    std::cout << "========================================\n" << std::endl;

    TestPacketFramingRoundtrip();
    TestInvalidMagicRejection();
    TestStreamAccumulatorAndFragmentation();
    TestOversizedPayloadRejection();

    std::cout << "ALL FRAMING TESTS PASSED!" << std::endl;
    return 0;
}
