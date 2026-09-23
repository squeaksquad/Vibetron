#include "TruePeakLimiter.h"

void TruePeakLimiter::prepare (double sampleRate, float ceilingDb)
{
    ceiling = juce::Decibels::decibelsToGain (ceilingDb);
    lookahead = juce::jmax (1, juce::roundToInt (0.0015 * sampleRate));
    releaseCoeff = (float) std::exp (-1.0 / (0.08 * sampleRate));
    env = 1.0f;

    // A peak can fall between detection points; for a tone at f the worst under-read is cos(pi f / (8 fs)).
    const double fMax = juce::jmin (20000.0, 0.45 * sampleRate);
    gridGuard = (float) (1.0 / std::cos (juce::MathConstants<double>::pi * fMax / (oversample * sampleRate)));

    // Blackman-windowed sinc for the in-between points of an 8x upsample.
    for (int k = 0; k < oversample - 1; ++k)
    {
        const double t = (k + 1) / (double) oversample;
        double sum = 0.0;
        for (int j = 0; j < interpTaps; ++j)
        {
            const double u = t - (double) (j - interpHalf + 1);  // distance from this tap to the point
            const double x = juce::MathConstants<double>::pi * u;
            const double sinc = std::abs (u) < 1.0e-9 ? 1.0 : std::sin (x) / x;
            const double wpos = u / interpHalf;
            const double win = std::abs (wpos) >= 1.0 ? 0.0
                             : 0.42 + 0.5 * std::cos (juce::MathConstants<double>::pi * wpos)
                                    + 0.08 * std::cos (2.0 * juce::MathConstants<double>::pi * wpos);
            phase[(size_t) k][(size_t) j] = (float) (sinc * win);
            sum += sinc * win;
        }
        for (auto& c : phase[(size_t) k])
            c = (float) (c / sum);
    }

    for (auto& h : history) h.fill (0.0f);
    histPos = 0;

    gainRing.assign ((size_t) lookahead + 1, 1.0f);
    minRing.assign ((size_t) lookahead, 1.0f);
    gainPos = minPos = 0;
    minSum = (double) lookahead;

    for (auto& d : delay) d.assign ((size_t) getLatencySamples() + 1, 0.0f);
    delayPos = 0;
}

void TruePeakLimiter::process (juce::AudioBuffer<float>& buffer, int numChannels, bool active)
{
    numChannels = juce::jmin (numChannels, maxChannels);
    const int n = buffer.getNumSamples();
    const int delayLen = (int) delay[0].size();

    for (int i = 0; i < n; ++i)
    {
        // True-peak estimate for the sample interpHalf behind the input, and the span after it.
        float tp = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto& h = history[(size_t) ch];
            h[(size_t) histPos] = buffer.getSample (ch, i);

            // h in time order: oldest at histPos + 1.
            std::array<float, interpTaps> ordered;
            for (int j = 0; j < interpTaps; ++j)
                ordered[(size_t) j] = h[(size_t) ((histPos + 1 + j) % interpTaps)];

            tp = juce::jmax (tp, std::abs (ordered[(size_t) interpHalf - 1]));
            for (const auto& coeffs : phase)
            {
                float y = 0.0f;
                for (int j = 0; j < interpTaps; ++j)
                    y += ordered[(size_t) j] * coeffs[(size_t) j];
                tp = juce::jmax (tp, std::abs (y));
            }
        }
        histPos = (histPos + 1) % interpTaps;
        tp *= gridGuard;

        const float required = (active && tp > ceiling) ? ceiling / tp : 1.0f;

        // Sliding minimum over L + 1, then a moving average over L: the smoothed gain is guaranteed to be
        // at or below the required gain when the peak reaches the (delayed) output.
        gainRing[(size_t) gainPos] = required;
        gainPos = (gainPos + 1) % (int) gainRing.size();
        float m = 1.0f;
        for (auto g : gainRing) m = juce::jmin (m, g);

        minSum += (double) m - (double) minRing[(size_t) minPos];
        minRing[(size_t) minPos] = m;
        minPos = (minPos + 1) % lookahead;
        const float avg = (float) (minSum / lookahead);

        env = avg < env ? avg : avg + (env - avg) * releaseCoeff;

        // Delay the audio by the detector + lookahead latency and apply the envelope.
        const int readPos = (delayPos + 1) % delayLen;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto& d = delay[(size_t) ch];
            d[(size_t) delayPos] = buffer.getSample (ch, i);
            buffer.setSample (ch, i, d[(size_t) readPos] * env);
        }
        delayPos = readPos;
    }

    // Periodically resync the running sum to avoid floating-point drift.
    double exact = 0.0;
    for (auto v : minRing) exact += v;
    minSum = exact;
}
