// SecondScreen Hardware Video Encoder Abstraction
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

#pragma once

#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <vector>
#include <string>
#include <memory>
#include <cstdint>
#include <functional>

namespace SecondScreen {

    enum class EncoderBackend {
        UNAVAILABLE = 0,
        NVENC = 1,        // NVIDIA NVENC (Low latency CBR)
        QUICKSYNC = 2,    // Intel QuickSync Video
        AMF = 3,          // AMD Advanced Media Framework
        SOFTWARE = 4      // CPU fallback (e.g. OpenH264 / libx264)
    };

    enum class CodecType {
        H264 = 1,
        HEVC = 2,
        AV1 = 3
    };

    enum class RateControlMode {
        CBR = 0, // Constant Bitrate (Recommended for sub-16ms LAN streaming)
        VBR = 1, // Variable Bitrate
        CQP = 2  // Constant Quantization Parameter
    };

    struct EncoderConfig {
        CodecType codec = CodecType::H264;
        UINT width = 1920;
        UINT height = 1080;
        UINT fps = 60;
        UINT bitrateKbps = 20000;        // 20 Mbps default
        UINT maxBitrateKbps = 25000;     // 25 Mbps ceiling
        UINT gopSize = 60;               // Keyframe interval (1 sec at 60fps)
        RateControlMode rateControl = RateControlMode::CBR;
        bool lowLatencyMode = true;      // Zero B-frames, slice-based intra-refresh
    };

    struct EncodedVideoPacket {
        std::vector<uint8_t> data;
        bool isKeyFrame;
        uint64_t frameNumber;
        uint64_t captureTimestampUs;
        uint64_t encodeTimestampUs;
        UINT width;
        UINT height;
        CodecType codec;
    };

    // Common Interface for all platform and vendor encoder backends
    class IHardwareEncoder {
    public:
        virtual ~IHardwareEncoder() = default;

        virtual bool Initialize(ID3D11Device* pDevice, const EncoderConfig& config) = 0;
        virtual bool Reconfigure(const EncoderConfig& config) = 0;
        virtual bool EncodeTexture(ID3D11Texture2D* pTexture, uint64_t frameNumber, uint64_t captureTimestampUs, bool forceKeyFrame, EncodedVideoPacket* pOutPacket) = 0;
        virtual void Flush(std::vector<EncodedVideoPacket>& outPackets) = 0;
        virtual void Shutdown() = 0;

        virtual EncoderBackend GetBackendType() const = 0;
        virtual const char* GetBackendName() const = 0;
        virtual bool IsHardwareAccelerated() const = 0;
    };

    // Factory to detect and instantiate genuine hardware encoders
    class HardwareEncoderFactory {
    public:
        // Probe system GPU adapters and instantiate the optimal available hardware encoder
        static std::unique_ptr<IHardwareEncoder> CreateEncoder(ID3D11Device* pDevice, const EncoderConfig& config);

        // Detect available GPU encoder backends without allocating session
        static std::vector<EncoderBackend> DetectAvailableBackends(ID3D11Device* pDevice);

        static const char* BackendToString(EncoderBackend backend);
    };

}
