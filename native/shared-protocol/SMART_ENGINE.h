#pragma once

#include <cstdint>
#include <string>

namespace second_screen::smart {

enum class ProfileId {
    Safe,
    Balanced,
    Quality,
    Ultra
};

enum class SceneActivity {
    Static,
    DesktopMotion,
    Video,
    HighMotion
};

struct StreamProfile {
    ProfileId id;
    uint32_t width;
    uint32_t height;
    uint32_t fps;
    uint32_t bitrateKbps;
    uint32_t latencyTargetMs;
};

struct Metrics {
    double rttMs{};
    double jitterMs{};
    double lossRatio{};
    double encodeMs{};
    double decodeMs{};
    double presentedFps{};
    uint32_t queueDepth{};
    bool thermalPressure{};
    bool batteryPressure{};
    SceneActivity scene{SceneActivity::DesktopMotion};
};

struct Decision {
    StreamProfile profile{};
    bool changed{};
    bool requestKeyFrame{};
    double qualityScore{};
};

class SmartStreamEngine {
public:
    SmartStreamEngine();

    void reset(ProfileId profile = ProfileId::Balanced);
    Decision update(const Metrics& metrics, uint64_t nowMs);

    const StreamProfile& currentProfile() const noexcept { return current_; }

private:
    static StreamProfile profile(ProfileId id);
    static double score(const Metrics& metrics, const StreamProfile& profile);

    ProfileId currentId_{ProfileId::Balanced};
    StreamProfile current_{profile(ProfileId::Balanced)};
    uint64_t lastChangeMs_{};
    uint32_t badSamples_{};
    uint32_t goodSamples_{};
};

}
