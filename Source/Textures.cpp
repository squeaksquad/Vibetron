#include "Textures.h"

namespace
{
    float hash (int x, int y, int seed)
    {
        auto h = (uint32_t) (x * 374761393 + y * 668265263 + seed * 2147483647);
        h = (h ^ (h >> 13)) * 1274126177u;
        return (float) ((h ^ (h >> 16)) & 0xffffff) / (float) 0xffffff;
    }

    float valueNoise (float x, float y, int seed)
    {
        const int xi = (int) std::floor (x), yi = (int) std::floor (y);
        const float xf = x - (float) xi, yf = y - (float) yi;
        const float u = xf * xf * (3 - 2 * xf), v = yf * yf * (3 - 2 * yf);
        const float a = hash (xi, yi, seed), b = hash (xi + 1, yi, seed);
        const float c = hash (xi, yi + 1, seed), d = hash (xi + 1, yi + 1, seed);
        return juce::jmap (v, juce::jmap (u, a, b), juce::jmap (u, c, d));
    }

    float fbm (float x, float y, int seed, int octaves)
    {
        float sum = 0, amp = 0.5f, norm = 0;
        for (int i = 0; i < octaves; ++i)
        {
            sum += amp * valueNoise (x, y, seed + i * 31);
            norm += amp;
            x *= 2.03f; y *= 2.03f; amp *= 0.5f;
        }
        return sum / norm;
    }

    // Signed value (-1..1) → white or black at proportional alpha.
    juce::PixelARGB signedPixel (float n, float maxAlpha)
    {
        const auto a = (juce::uint8) juce::jlimit (0, 255, (int) (std::abs (n) * maxAlpha * 255.0f));
        juce::PixelARGB p (a, n > 0 ? a : 0, n > 0 ? a : 0, n > 0 ? a : 0);  // premultiplied
        return p;
    }

    template <typename Fn>
    juce::Image make (int w, int h, juce::Image::PixelFormat fmt, Fn&& fn)
    {
        juce::Image img (fmt, w, h, true);
        juce::Image::BitmapData bd (img, juce::Image::BitmapData::writeOnly);
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                *reinterpret_cast<juce::PixelARGB*> (bd.getPixelPointer (x, y)) = fn (x, y);
        return img;
    }
}

namespace textures
{
    const juce::Image& brushedMetal()
    {
        static const juce::Image img = []
        {
            constexpr int W = 1600, H = 800;
            std::vector<float> rowBias (H), line (W);
            juce::Random rng (0x5eed);
            for (auto& b : rowBias) b = (rng.nextFloat() - 0.5f) * 0.9f;

            juce::Image out (juce::Image::ARGB, W, H, true);
            juce::Image::BitmapData bd (out, juce::Image::BitmapData::writeOnly);
            for (int y = 0; y < H; ++y)
            {
                // Long horizontal streaks: heavily low-passed noise along x, plus a per-row bias.
                float v = 0;
                for (int x = 0; x < W; ++x)
                {
                    v = v * 0.985f + (rng.nextFloat() - 0.5f) * 0.25f;
                    line[(size_t) x] = v;
                }
                const bool scratch = rng.nextFloat() < 0.012f;
                const int s0 = rng.nextInt (W), s1 = s0 + 40 + rng.nextInt (260);
                for (int x = 0; x < W; ++x)
                {
                    float n = rowBias[(size_t) y] * 0.55f + line[(size_t) x] * 1.4f
                              + (fbm ((float) x * 0.004f, (float) y * 0.05f, 7, 3) - 0.5f) * 0.35f;
                    if (scratch && x >= s0 && x < s1) n += 0.9f;
                    *reinterpret_cast<juce::PixelARGB*> (bd.getPixelPointer (x, y)) = signedPixel (juce::jlimit (-1.0f, 1.0f, n), 0.55f);
                }
            }
            return out;
        }();
        return img;
    }

    const juce::Image& spunAluminium()
    {
        static const juce::Image img = []
        {
            // Conic sheen stops (turns from 18 deg clockwise of 12 o'clock) and a lathe-ring modulation on radius.
            struct Stop { float t; juce::uint32 argb; };
            static constexpr Stop stops[] { { 0.00f, 0xffdcdee1 }, { 0.11f, 0xff8e9196 }, { 0.24f, 0xfff5f6f7 },
                                            { 0.36f, 0xffa0a3a8 }, { 0.49f, 0xffd2d5d9 }, { 0.61f, 0xff85888d },
                                            { 0.74f, 0xfff1f2f4 }, { 0.87f, 0xff999ca2 }, { 1.00f, 0xffdcdee1 } };
            constexpr int S = 640;
            constexpr float R = S * 0.5f, rings = 32.0f;
            return make (S, S, juce::Image::ARGB, [&] (int x, int y)
            {
                const float dx = (float) x + 0.5f - R, dy = (float) y + 0.5f - R;
                const float r = std::sqrt (dx * dx + dy * dy) / R;
                if (r > 1.0f)
                    return juce::PixelARGB (0, 0, 0, 0);

                float deg = juce::radiansToDegrees (std::atan2 (dx, -dy)) - 18.0f;
                deg = std::fmod (deg + 720.0f, 360.0f);
                const float t = deg / 360.0f;
                int i = 0;
                while (stops[i + 1].t < t) ++i;
                float u = (t - stops[i].t) / (stops[i + 1].t - stops[i].t);
                u = 0.5f - 0.5f * std::cos (u * juce::MathConstants<float>::pi);
                auto c = juce::Colour (stops[i].argb).interpolatedWith (juce::Colour (stops[i + 1].argb), u);

                // Lathe rings: a fine periodic groove plus per-ring brightness jitter.
                const float ringPos = r * rings;
                const float groove = std::sin (ringPos * juce::MathConstants<float>::twoPi) * 0.035f;
                const float jitter = (hash ((int) (ringPos * 3.0f), 0, 77) - 0.5f) * 0.06f;
                const float micro = (hash (x, y, 5) - 0.5f) * 0.025f;
                c = c.withMultipliedBrightness (1.0f + groove + jitter + micro);

                const float edge = juce::jlimit (0.0f, 1.0f, (1.0f - r) * R / 1.5f);  // anti-aliased rim
                const auto a = (juce::uint8) juce::roundToInt (edge * 255.0f);
                juce::PixelARGB px (255, c.getRed(), c.getGreen(), c.getBlue());
                px.multiplyAlpha (a);
                return px;
            });
        }();
        return img;
    }

    const juce::Image& filmGrain()
    {
        static const juce::Image img = []
        {
            juce::Random rng (0xf11);
            return make (1600, 800, juce::Image::ARGB, [&] (int, int)
            {
                return signedPixel ((rng.nextFloat() - 0.5f) * 2.0f, 0.06f);
            });
        }();
        return img;
    }
}
