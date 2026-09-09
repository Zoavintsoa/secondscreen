// SecondScreen AMD AMF (Advanced Media Framework) Hardware Encoder
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

#pragma once

#include "HardwareEncoder.h"

namespace SecondScreen {

    class AMFEncoder : public IHardwareEncoder {
    public:
        AMFEncoder();
        virtual ~AMFEncoder();

        static bool IsSupported(ID3D11Device* pDevice);

        virtual bool Initialize(ID3D11Device* pDevice, const EncoderConfig& config) override;
        virtual bool Reconfigure(const EncoderConfig& config) override;
        virtual bool EncodeTexture(ID3D11Texture2D* pTexture, uint64_t frameNumber, uint64_t captureTimestampUs, bool forceKeyFrame, EncodedVideoPacket* pOutPacket) override;
        virtual void Flush(std::vector<EncodedVideoPacket>& outPackets) override;
        virtual void Shutdown() override;

        virtual EncoderBackend GetBackendType() const override { return EncoderBackend::AMF; }
        virtual const char* GetBackendName() const override { return "AMD AMF (Hardware Accelerated)"; }
        virtual bool IsHardwareAccelerated() const override { return true; }

    private:
        HMODULE m_hAmfDll;
        EncoderConfig m_Config;
        bool m_Initialized;
    };

}
