// Core Protocol v1 - SecondScreen Common Wire Protocol Definition
// Platform-independent binary packet layouts and enums

#pragma once

#include <cstdint>
#include <cstddef>

#pragma pack(push, 1)


namespace SecondScreen {
    constexpr uint32_t PROTOCOL_VERSION = 1;
    constexpr uint16_t DEFAULT_HOST_PORT = 9876;
    constexpr uint16_t DEFAULT_DISCOVERY_PORT = 9877;
    constexpr uint16_t DEFAULT_VIDEO_PORT = 9878;


    enum class MessageType : uint8_t {
        HELLO = 0x01,
        DISCOVER = 0x02,
        PAIR_REQUEST = 0x03,
        PAIR_RESPONSE = 0x04,
        DISPLAY_CONFIG = 0x05,
        STREAM_START = 0x06,
        STREAM_STOP = 0x07,
        FRAME = 0x08,
        PING = 0x09,
        PONG = 0x0A,
        TELEMETRY = 0x0B,
        INPUT_EVENT = 0x0C,
        DISCONNECT = 0x0D,
        KEYFRAME_REQUEST = 0x0E,
        DISPLAY_CONFIG_ACK = 0x0F
    };

    enum class DisplayConfigStatus : uint8_t {
        ACCEPTED = 0x00,
        REJECTED = 0x01,
        UNSUPPORTED = 0x02,
        BUSY = 0x03
    };

    enum class DisplayMode : uint8_t {
        EXTEND = 0x00,
        DUPLICATE = 0x01,
        SECOND_SCREEN_ONLY = 0x02
    };

    enum class VideoCodec : uint8_t {
        H264 = 0x01,
        HEVC = 0x02,
        AV1 = 0x03
    };

    enum class EncoderType : uint8_t {
        NVENC = 0x01,
        AMF = 0x02,
        QUICKSYNC = 0x03,
        VIDEOTOOLBOX = 0x04,
        SOFTWARE = 0x05
    };

    constexpr uint32_t PROTOCOL_MAGIC = 0x53325343; // 'S2SC' (Control / Generic)
    constexpr uint32_t VIDEO_MAGIC = 0x53325644;    // 'S2VD' (UDP Low-Latency Video Datagram)
    constexpr size_t PACKET_HEADER_SIZE = 24;
    constexpr size_t VIDEO_PACKET_HEADER_SIZE = 32;
    constexpr size_t INPUT_EVENT_SIZE = 22;
    constexpr size_t TELEMETRY_DATA_SIZE = 37;
    constexpr size_t DISPLAY_CONFIG_SIZE = 20;
    constexpr size_t DISPLAY_CONFIG_ACK_SIZE = 8;
    constexpr uint32_t MAX_CONTROL_PAYLOAD_SIZE = 64 * 1024;
    constexpr uint32_t MAX_VIDEO_PAYLOAD_SIZE = 16 * 1024 * 1024;

    // TCP Port 9876 Generic Control Header (24 Bytes)
    struct PacketHeader {
        uint32_t magic;      // 0x53325343 ('S2SC')
        uint16_t version;    // PROTOCOL_VERSION
        uint8_t  type;       // MessageType
        uint8_t  flags;      // Bit flags (e.g. 0x01 = Keyframe)
        uint32_t sequence;   // Incrementing packet index
        uint64_t timestamp;  // Microsecond timestamp
        uint32_t payloadSize;// Length of payload immediately following header
    };

    // UDP Port 9878 Video Streaming Fragment Header (32 Bytes)
    struct VideoPacketHeader {
        uint32_t magic;         // 0x53325644 ('S2VD')
        uint32_t sessionId;     // Authenticated session identifier
        uint32_t frameId;       // Monotonic frame counter
        uint16_t packetIndex;   // Fragment index (0..packetCount-1)
        uint16_t packetCount;   // Total fragments composing this frame
        uint8_t  flags;         // Bit 0: Keyframe, Bit 1: Last fragment
        uint8_t  codec;         // VideoCodec (H.264 = 1, HEVC = 2, AV1 = 3)
        uint16_t payloadSize;   // Length of H.264 fragment payload in this UDP packet
        uint64_t timestampUs;   // Capture microsecond timestamp
        uint32_t crc32;         // IEEE 802.3 CRC32 of payload (poly: 0xEDB88320)
    };

    struct DisplayConfig {
        uint32_t displayId;
        uint32_t width;
        uint32_t height;
        uint32_t refreshRate;
        uint8_t  orientation; // 0 = Landscape, 1 = Portrait
        uint8_t  displayMode; // DisplayMode
        uint16_t dpiScalePercent;
    };

    struct DisplayConfigAck {
        uint32_t displayId;
        uint8_t  status;      // DisplayConfigStatus (0 = ACCEPTED, 1 = REJECTED, 2 = UNSUPPORTED, 3 = BUSY)
        uint8_t  reserved[3]; // Padding for 8-byte alignment
    };

    struct TelemetryData {
        float    fps;
        float    bitrateMbps;
        float    rttMs;
        float    decodeLatencyMs;
        float    renderLatencyMs;
        float    totalLatencyMs;
        float    packetLossPercent;
        uint32_t frameDrops;
        float    jitterMs;
        uint8_t  isMeasured; // 1 = Real hardware measurement, 0 = Estimate
    };

    struct InputEvent {
        uint8_t  actionType; // 0 = Down, 1 = Move, 2 = Up, 3 = Wheel, 4 = Stylus
        float    normX;      // 0.0f - 1.0f
        float    normY;      // 0.0f - 1.0f
        float    pressure;   // 0.0f - 1.0f (or Apple Pencil / S-Pen pressure)
        uint8_t  button;     // 0 = Left, 1 = Middle, 2 = Right
        uint64_t clientTimestamp;
    };

    static_assert(sizeof(PacketHeader) == PACKET_HEADER_SIZE, "PacketHeader must be exactly 24 bytes");
    static_assert(sizeof(VideoPacketHeader) == VIDEO_PACKET_HEADER_SIZE, "VideoPacketHeader must be exactly 32 bytes");
    static_assert(sizeof(InputEvent) == INPUT_EVENT_SIZE, "InputEvent must be exactly 22 bytes");
    static_assert(sizeof(TelemetryData) == TELEMETRY_DATA_SIZE, "TelemetryData must be exactly 37 bytes");
    static_assert(sizeof(DisplayConfig) == DISPLAY_CONFIG_SIZE, "DisplayConfig must be exactly 20 bytes");
    static_assert(sizeof(DisplayConfigAck) == DISPLAY_CONFIG_ACK_SIZE, "DisplayConfigAck must be exactly 8 bytes");

    // Exact binary offset compile-time assertions
    static_assert(offsetof(PacketHeader, payloadSize) == 20, "PacketHeader payloadSize must be at offset 20");
    static_assert(offsetof(VideoPacketHeader, crc32) == 28, "VideoPacketHeader crc32 must be at offset 28");
    static_assert(offsetof(TelemetryData, isMeasured) == 36, "TelemetryData isMeasured must be at offset 36");
    static_assert(offsetof(DisplayConfigAck, status) == 4, "DisplayConfigAck status must be at offset 4");

    // Standard CRC-32 (IEEE 802.3 / ISO 3309)
    // Polynomial (Reflected) : 0xEDB88320
    // Initial Value          : 0xFFFFFFFF
    // Final XOR              : 0xFFFFFFFF
    // Test Vector "123456789": 0xCBF43926
    inline uint32_t ComputeCRC32(const uint8_t* data, size_t length, uint32_t runningCrc = 0xFFFFFFFF) {
        uint32_t crc = runningCrc;
        for (size_t i = 0; i < length; ++i) {
            crc ^= data[i];
            for (int j = 0; j < 8; ++j) {
                crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
            }
        }
        return crc;
    }

    inline uint32_t FinalizeCRC32(uint32_t runningCrc) {
        return ~runningCrc;
    }

    inline uint32_t CalculateCRC32(const uint8_t* data, size_t length) {
        return FinalizeCRC32(ComputeCRC32(data, length, 0xFFFFFFFF));
    }

    // Computes CRC32 over VideoPacketHeader (bytes 0..27, excluding crc32 field itself) + Payload
    inline uint32_t CalculateVideoPacketCRC(const VideoPacketHeader& header, const uint8_t* payload) {
        constexpr size_t headerCoverageSize = offsetof(VideoPacketHeader, crc32); // 28 bytes
        uint32_t running = ComputeCRC32(reinterpret_cast<const uint8_t*>(&header), headerCoverageSize, 0xFFFFFFFF);
        if (header.payloadSize > 0 && payload != nullptr) {
            running = ComputeCRC32(payload, header.payloadSize, running);
        }
        return FinalizeCRC32(running);
    }
}


#pragma pack(pop)
