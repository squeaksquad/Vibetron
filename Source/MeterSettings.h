#pragma once

#include <juce_core/juce_core.h>
#include <cmath>

// User meter calibration. Stored with the session (not automatable).
struct MeterSettings
{
    static constexpr float defaultRefDbfs = -18.0f, defaultRiseMs = 300.0f, defaultFallMs = 300.0f, defaultOvershootPct = 1.5f;

    float refDbfs = defaultRefDbfs;         // level that reads 0 VU
    float riseMs = defaultRiseMs;           // time to reach 99% of a step
    float fallMs = defaultFallMs;           // time to fall back 99% of a step
    float overshootPct = defaultOvershootPct;
    bool peakMode = false;                  // needle follows sample peaks (PPM-style) instead of VU average

    bool operator== (const MeterSettings& o) const
    {
        return refDbfs == o.refDbfs && riseMs == o.riseMs && fallMs == o.fallMs && overshootPct == o.overshootPct
            && peakMode == o.peakMode;
    }
    bool operator!= (const MeterSettings& o) const { return ! (*this == o); }
};

// Second-order needle model derived from the settings.
struct Ballistics
{
    float wnRise = 13.5f, wnFall = 13.5f, zeta = 0.81f;

    // Peak mode: the detector envelope does the falling, so the needle just tracks it quickly with no overshoot.
    static Ballistics forPeak()
    {
        MeterSettings s;
        s.riseMs = 10.0f;
        s.fallMs = 40.0f;
        s.overshootPct = 0.0f;
        return fromTimes (s);
    }

    static Ballistics from (const MeterSettings& s) { return s.peakMode ? forPeak() : fromTimes (s); }

private:
    static Ballistics fromTimes (const MeterSettings& s)
    {
        Ballistics b;
        const double os = juce::jlimit (0.0, 0.3, (double) s.overshootPct / 100.0);
        if (os < 1.0e-4)
        {
            b.zeta = 1.0f;
        }
        else
        {
            const double l = std::log (os);
            b.zeta = (float) (-l / std::sqrt (juce::MathConstants<double>::pi * juce::MathConstants<double>::pi + l * l));
        }

        // Time for a unit-frequency oscillator with this damping to reach 99% of a step; scale wn to match.
        double y = 0.0, v = 0.0, t = 0.0;
        constexpr double dt = 1.0e-4;
        while (y < 0.99 && t < 100.0)
        {
            v += (1.0 - y - 2.0 * b.zeta * v) * dt;
            y += v * dt;
            t += dt;
        }
        b.wnRise = (float) (t / (juce::jmax (5.0f, s.riseMs) / 1000.0f));
        b.wnFall = (float) (t / (juce::jmax (5.0f, s.fallMs) / 1000.0f));
        return b;
    }
};
