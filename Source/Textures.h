#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Procedural surface textures, generated once at high resolution and shared.
namespace textures
{
    const juce::Image& brushedMetal();  // grey-signed overlay: white/black streaks on transparent
    const juce::Image& spunAluminium(); // circular knob face: conic spun-metal sheen with lathe rings
    const juce::Image& filmGrain();     // fine, very faint noise for the whole faceplate
}
