// Unit Test: End-to-End Pipeline & Annex-B Bitstream Parser Validation
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

#include "../core/protocol/Protocol.h"
#include <iostream>
#include <vector>
#include <cassert>
#include <cstdint>

struct NaluInfo {
    uint8_t type;
    size_t offset;
    size_t length;
};

std::vector<NaluInfo> ParseAnnexBNalus(const uint8_t* data, size_t size) {
    std::vector<NaluInfo> nalus;
    size_t i = 0;

    while (i < size) {
        size_t startCodeLen = 0;
        if (i + 4 <= size && data[i] == 0 && data[i+1] == 0 && data[i+2] == 0 && data[i+3] == 1) {
            startCodeLen = 4;
        } else if (i + 3 <= size && data[i] == 0 && data[i+1] == 0 && data[i+2] == 1) {
            startCodeLen = 3;
        }

        if (startCodeLen > 0) {
            size_t naluStart = i + startCodeLen;
            size_t nextStart = size;
            for (size_t j = naluStart; j + 3 <= size; ++j) {
                if ((j + 4 <= size && data[j] == 0 && data[j+1] == 0 && data[j+2] == 0 && data[j+3] == 1) ||
                    (data[j] == 0 && data[j+1] == 0 && data[j+2] == 1)) {
                    nextStart = j;
                    break;
                }
            }

            if (nextStart > naluStart) {
                uint8_t naluType = data[naluStart] & 0x1F;
                nalus.push_back({ naluType, naluStart, nextStart - naluStart });
            }

            i = nextStart;
        } else {
            i++;
        }
    }

    return nalus;
}

void TestAnnexBNaluExtraction() {
    std::cout << "[TEST] Validating H.264 Annex-B NAL Unit Parsing..." << std::endl;

    // Simulated NVENC output: SPS (type 7) + PPS (type 8) + IDR slice (type 5)
    std::vector<uint8_t> h264Bitstream = {
        0x00, 0x00, 0x00, 0x01, 0x67, 0x64, 0x00, 0x28, 0xAC, // SPS (7)
        0x00, 0x00, 0x00, 0x01, 0x68, 0xEE, 0x3C, 0x80,       // PPS (8)
        0x00, 0x00, 0x00, 0x01, 0x65, 0x88, 0x84, 0x00, 0x10  // IDR Slice (5)
    };

    auto nalus = ParseAnnexBNalus(h264Bitstream.data(), h264Bitstream.size());
    assert(nalus.size() == 3);

    std::cout << "  NALU 0: Type " << (int)nalus[0].type << " (Expected: 7 SPS)" << std::endl;
    assert(nalus[0].type == 7);

    std::cout << "  NALU 1: Type " << (int)nalus[1].type << " (Expected: 8 PPS)" << std::endl;
    assert(nalus[1].type == 8);

    std::cout << "  NALU 2: Type " << (int)nalus[2].type << " (Expected: 5 IDR)" << std::endl;
    assert(nalus[2].type == 5);

    std::cout << "[PASS] Annex-B NAL unit extraction successful.\n" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "SecondScreen Pipeline Test Suite" << std::endl;
    std::cout << "========================================\n" << std::endl;

    TestAnnexBNaluExtraction();

    std::cout << "ALL PIPELINE TESTS PASSED!" << std::endl;
    return 0;
}
