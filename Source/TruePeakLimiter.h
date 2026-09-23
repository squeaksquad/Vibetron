#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <vector>

// Stereo-linked lookahead limiter with 8x true-peak detection (windowed-sinc interpolation).
// Latency is constant whether or not limiting is active, so toggling never shifts timing.
class TruePeakLimiter
{
public:
    void prepare (double sampleRate, float ceilingDb);
    int getLatencySamples() const { return interpHalf + lookahead - 1; }

    // Processes up to 2 channels in place. When active is false the signal is only delayed and the
    // gain envelope releases back to unity, so switching on or off is click-free.
    void process (juce::AudioBuffer<float>& buffer, int numChannels, bool active);

private:
    static constexpr int oversample = 8;
    static constexpr int interpHalf = 16;       // taps each side; detection latency in samples
    static constexpr int interpTaps = 2 * interpHalf;
    static constexpr int maxChannels = 2;

    float ceiling = 1.0f, releaseCoeff = 0.0f, env = 1.0f;
    float gridGuard = 1.0f;  // worst-case under-read between 8x detection points, for content up to 20 kHz
    int lookahead = 1;

    std::array<std::array<float, interpTaps>, oversample - 1> phase {};   // fractional offsets k/8
    std::array<std::array<float, interpTaps>, maxChannels> history {};
    int histPos = 0;

    std::vector<float> gainRing, minRing;                     // sliding min (L + 1) and moving average (L)
    int gainPos = 0, minPos = 0;
    double minSum = 0.0;

    std::array<std::vector<float>, maxChannels> delay;
    int delayPos = 0;

};
