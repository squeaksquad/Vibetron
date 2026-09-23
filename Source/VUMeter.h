#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "Theme.h"
#include "MeterSettings.h"

// Glass-fronted backlit VU meter. Default ballistics follow IEC 60268-17 (~300 ms to 99%, ~1.5% overshoot)
// and are user-adjustable. The needle is linear in voltage, like the real thing.
class VUMeter : public juce::Component
{
public:
    VUMeter (const Palette& palette, juce::String channelLabel);

    // rectified: full-wave rectified average (linear). peak: highest |sample| since the last frame (linear).
    // The needle reads the average (VU) or a falling peak envelope (peak mode); the PEAK lamp lights above the reference.
    void advance (float rectified, float peak, const MeterSettings& s, const Ballistics& b, double dt);
    void paint (juce::Graphics&) override;

    static float positionForVu (float vuDb);

private:
    const Palette& pal;
    juce::String label;
    float pos = 0.0f, vel = 0.0f, peakHold = 0.0f;
    float peakEnvDb = -100.0f;
    bool peakMode = false;  // face legend reads PEAK instead of VU
};
