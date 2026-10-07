#pragma once

#include <cstdint>
#include <windows.h>

namespace second_screen::frame_ipc {

inline constexpr uint32_t kMagic = 0x49535353; // "SSSI"
inline constexpr uint32_t kVersion = 1;
inline constexpr wchar_t kStateName[] = L"Global\SecondScreen.FrameState";
inline constexpr wchar_t kReadyEventName[] = L"Global\SecondScreen.FrameReady";
inline constexpr wchar_t kTextureNames[3][64] = {
    L"Global\SecondScreen.FrameTexture0",
    L"Global\SecondScreen.FrameTexture1",
    L"Global\SecondScreen.FrameTexture2"
};

struct alignas(8) SharedState {
    uint32_t magic;
    uint32_t version;
    volatile LONG64 sequence;
    volatile LONG slot;
    uint32_t width;
    uint32_t height;
    uint64_t timestampUs;
    LUID adapterLuid;
};

static_assert(sizeof(SharedState) <= 128);

}
