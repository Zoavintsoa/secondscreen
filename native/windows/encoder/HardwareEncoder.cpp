// SecondScreen Hardware Video Encoder Factory Implementation
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

#include "HardwareEncoder.h"
#include "NVENCEncoder.h"
#include "QuickSyncEncoder.h"
#include "AMFEncoder.h"
#include <iostream>

namespace SecondScreen {

    std::vector<EncoderBackend> HardwareEncoderFactory::DetectAvailableBackends(ID3D11Device* pDevice) {
        std::vector<EncoderBackend> backends;

        if (NVENCEncoder::IsSupported(pDevice)) {
            backends.push_back(EncoderBackend::NVENC);
        }
        if (QuickSyncEncoder::IsSupported(pDevice)) {
            backends.push_back(EncoderBackend::QUICKSYNC);
        }
        if (AMFEncoder::IsSupported(pDevice)) {
            backends.push_back(EncoderBackend::AMF);
        }

        return backends;
    }

    std::unique_ptr<IHardwareEncoder> HardwareEncoderFactory::CreateEncoder(ID3D11Device* pDevice, const EncoderConfig& config) {
        // Probe hardware in order of performance capability
        if (NVENCEncoder::IsSupported(pDevice)) {
            auto encoder = std::make_unique<NVENCEncoder>();
            if (encoder->Initialize(pDevice, config)) {
                return encoder;
            }
        }

        std::wcerr << L"[Encoder Factory] NVENC is unavailable. Quick Sync and AMF are detected but not enabled because their H.264 pipelines are not implemented." << std::endl;
        return nullptr;
    }

    const char* HardwareEncoderFactory::BackendToString(EncoderBackend backend) {
        switch (backend) {
            case EncoderBackend::NVENC:     return "NVIDIA NVENC";
            case EncoderBackend::QUICKSYNC: return "Intel QuickSync";
            case EncoderBackend::AMF:       return "AMD AMF";
            case EncoderBackend::SOFTWARE:  return "Software CPU Fallback";
            default:                        return "UNAVAILABLE";
        }
    }

}
