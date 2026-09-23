#pragma once

#include <array>

// The three operating modes. gainDb is internal only and never surfaced in the UI or parameter text.
struct ModeInfo
{
    const char* numeral;
    const char* name;
    const char* tag;
    float gainDb;
    const char* notes;
};

inline constexpr int numModes = 3;

inline constexpr std::array<ModeInfo, numModes> kModes {{
    { "I", "Silk", "AIR \xc2\xb7 OPENNESS", 0.3f,
      "A whisper of harmonic grace. Silk lifts the veil between you and the performance: transients bloom "
      "with ribbon-like ease, the soundstage breathes a quarter-inch wider, and the air above 12 kHz turns "
      "palpable. Not brighter. Truer." },
    { "II", "Velvet", "WARMTH \xc2\xb7 DENSITY", 0.6f,
      "The sound of a warm room at 2 a.m. Midrange takes on a liquid, tactile density; vocals step out from "
      "the speakers and sit beside you. Low end gains the rounded authority of well-biased tape, and the "
      "blacks get blacker." },
    { "III", "Obsidian", "DEPTH \xc2\xb7 HOLOGRAPHY", 0.9f,
      "Our most uncompromising topology. A holographic, three-dimensional presentation with bass you feel as "
      "much as hear, and image specificity that borders on uncanny. Once heard, the bypassed signal may sound "
      "permanently flat." },
}};
