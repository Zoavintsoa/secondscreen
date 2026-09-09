// Core Protocol v1 - SecondScreen Common Wire Protocol Definition
// Platform-independent binary packet layouts and enums

#pragma once

#include <cstdint>
#include <cstddef>

#pragma pack(push, 1)


namespace SecondScreen {
    constexpr uint32_t PROTOCOL_VERSION = 1;
    constexpr uint16_t DEFAULT_HOST_PORT = 5000;
    constexpr uint16_t DEFAULT_DISCOVERY_PORT = 5001;


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
        DISCONNECT = 0x0D
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

    constexpr uint32_t PROTOCOL_MAGIC = 0x53325343; // 'S2SC'
    constexpr size_t PACKET_HEADER_SIZE = 24;
    constexpr size_t INPUT_EVENT_SIZE = 22;
    constexpr size_t TELEMETRY_DATA_SIZE = 37;
    constexpr size_t DISPLAY_CONFIG_SIZE = 20;
    constexpr uint32_t MAX_CONTROL_PAYLOAD_SIZE = 64 * 1024;
    constexpr uint32_t MAX_VIDEO_PAYLOAD_SIZE = 16 * 1024 * 1024;

    struct PacketHeader {
        uint32_t magic;      // 0x53325343 ('S2SC')
        uint16_t version;    // PROTOCOL_VERSION
        uint8_t  type;       // MessageType
        uint8_t  flags;      // Bit flags (e.g. 0x01 = Keyframe)
        uint32_t sequence;   // Incrementing packet index
        uint64_t timestamp;  // Microsecond timestamp
        uint32_t payloadSize;// Length of payload immediately following header
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
    static_assert(sizeof(InputEvent) == INPUT_EVENT_SIZE, "InputEvent must be exactly 22 bytes");
    static_assert(sizeof(TelemetryData) == TELEMETRY_DATA_SIZE, "TelemetryData must be exactly 37 bytes");
    static_assert(sizeof(DisplayConfig) == DISPLAY_CONFIG_SIZE, "DisplayConfig must be exactly 20 bytes");
}


#pragma pack(pop)
