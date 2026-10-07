#include "SMART_ENGINE.h"

#include <algorithm>

namespace second_screen::smart {

namespace {
constexpr uint64_t kMinDwellMs = 1500;
constexpr uint32_t kBadSamplesToDegrade = 3;
constexpr uint32_t kGoodSamplesToRecover = 12;

bool isWorse(ProfileId a, ProfileId b) {
    return static_cast<int>(a) < static_cast<int>(b);
}

ProfileId lower(ProfileId id) {
    if (id == ProfileId::Safe) return id;
    return static_cast<ProfileId>(static_cast<int>(id) - 1);
}

ProfileId higher(ProfileId id) {
    if (id == ProfileId::Ultra) return id;
    return static_cast<ProfileId>(static_cast<int>(id) + 1);
}
}

StreamProfile SmartStreamEngine::profile(ProfileId id) {
    switch (id) {
    case ProfileId::Safe:     return {id, 1280, 720, 30, 4000, 70};
    case ProfileId::Quality:  return {id, 2560, 1440, 60, 16000, 50};
    case ProfileId::Ultra:    return {id, 3840, 2160, 60, 30000, 50};
    case ProfileId::Balanced:
    default:                  return {id, 1920, 1080, 60, 8000, 50};
    }
}

SmartStreamEngine::SmartStreamEngine() = default;

void SmartStreamEngine::reset(ProfileId profileId) {
    currentId_ = profileId;
    current_ = profile(profileId);
    lastChangeMs_ = 0;
    badSamples_ = 0;
    goodSamples_ = 0;
}

double SmartStreamEngine::score(const Metrics& m, const StreamProfile& p) {
    const double frameBudget = 1000.0 / std::max<uint32_t>(1, p.fps);
    const double computePenalty =
        std::max(0.0, (m.encodeMs - frameBudget) / frameBudget) +
        std::max(0.0, (m.decodeMs - frameBudget) / frameBudget);

    const double networkPenalty =
        std::min(1.0, m.rttMs / 100.0) * 0.30 +
        std::min(1.0, m.jitterMs / 30.0) * 0.20 +
        std::min(1.0, m.lossRatio * 20.0) * 0.30;

    const double fpsPenalty =
        m.presentedFps > 0.0
            ? std::max(0.0, 1.0 - m.presentedFps / static_cast<double>(p.fps)) * 0.20
            : 0.20;

    const double pressurePenalty =
        (m.thermalPressure ? 0.10 : 0.0) +
        (m.batteryPressure ? 0.05 : 0.0);

    return std::clamp(
        1.0 - (computePenalty * 0.35 + networkPenalty + fpsPenalty + pressurePenalty),
        0.0, 1.0);
}

SmartStreamEngine::Decision SmartStreamEngine::update(const Metrics& metrics, uint64_t nowMs) {
    const double quality = score(metrics, current_);
    const double frameBudget = 1000.0 / std::max<uint32_t>(1, current_.fps);

    const bool bad =
        quality < 0.55 ||
        metrics.encodeMs > frameBudget * 0.90 ||
        metrics.decodeMs > frameBudget * 0.90 ||
        metrics.lossRatio > 0.02 ||
        metrics.rttMs > current_.latencyTargetMs * 1.8;

    const bool good =
        quality > 0.82 &&
        metrics.lossRatio < 0.002 &&
        metrics.rttMs < current_.latencyTargetMs &&
        metrics.encodeMs < frameBudget * 0.65 &&
        metrics.decodeMs < frameBudget * 0.65 &&
        metrics.presentedFps >= current_.fps * 0.95;

    if (bad) {
        ++badSamples_;
        goodSamples_ = 0;
    } else if (good) {
        ++goodSamples_;
        badSamples_ = 0;
    } else {
        badSamples_ = 0;
        goodSamples_ = 0;
    }

    ProfileId target = currentId_;
    if (badSamples_ >= kBadSamplesToDegrade &&
        (lastChangeMs_ == 0 || nowMs - lastChangeMs_ >= kMinDwellMs)) {
        target = lower(currentId_);
    } else if (goodSamples_ >= kGoodSamplesToRecover &&
               (lastChangeMs_ == 0 || nowMs - lastChangeMs_ >= kMinDwellMs)) {
        target = higher(currentId_);
    }

    Decision result{};
    result.profile = current_;
    result.qualityScore = quality;

    if (target != currentId_) {
        currentId_ = target;
        current_ = profile(target);
        lastChangeMs_ = nowMs;
        badSamples_ = 0;
        goodSamples_ = 0;
        result.profile = current_;
        result.changed = true;
        result.requestKeyFrame = true;
    }

    return result;
}

}
