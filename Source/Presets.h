#pragma once
#include "Dynamics.h"
namespace mo {
struct Preset {
  const char *name;
  const char *category;
  Parameters p;
};
inline const Preset presets[] = {
    {"MO Default", "Factory Essentials", {-90, 0, -1}},
    {"Gentle Vocal", "Factory Essentials", {-55, -5, -1}},
    {"Soft Control", "Factory Essentials", {-50, -8, -1}},
    {"Vocal Leveler", "Factory Essentials", {-45, -11, -1}},
    {"Vocal Presence", "Factory Essentials", {-42, -14, -1}},
    {"Tight Vocal", "Factory Essentials", {-38, -17, -1}},
    {"Heavy Compression", "Factory Essentials", {-40, -24, -2}},
    {"Maximum Density", "Factory Essentials", {-35, -30, -3}},
    {"Male Natural", "Male Vocals", {-50, -7, -1}},
    {"Male Warmth", "Male Vocals", {-46, -10, -1}},
    {"Male Pop Lead", "Male Vocals", {-43, -14, -1}},
    {"Male Ballad", "Male Vocals", {-52, -8, -1}},
    {"Male Power Vocal", "Male Vocals", {-40, -18, -2}},
    {"Deep Baritone", "Male Vocals", {-47, -10, -1}},
    {"Female Natural", "Female Vocals", {-53, -7, -1}},
    {"Female Soft", "Female Vocals", {-55, -9, -1}},
    {"Female Pop Lead", "Female Vocals", {-45, -14, -1}},
    {"Female Intimate", "Female Vocals", {-50, -11, -1}},
    {"Female Powerful", "Female Vocals", {-42, -19, -2}},
    {"Female Harmony", "Female Vocals", {-48, -10, -2}},
    {"Rap Lead", "Rap and Modern Music", {-40, -17, -1}},
    {"Trap Vocal", "Rap and Modern Music", {-38, -20, -2}},
    {"Melodic Rap", "Rap and Modern Music", {-46, -13, -1}},
    {"Drill Vocal", "Rap and Modern Music", {-38, -21, -2}},
    {"R&B Smooth", "Rap and Modern Music", {-48, -12, -1}},
    {"Hyper Compressed Rap", "Rap and Modern Music", {-35, -27, -3}},
    {"Podcast Voice", "Speech and Broadcast", {-45, -12, -1}},
    {"Radio Voice", "Speech and Broadcast", {-41, -16, -1}},
    {"Voice Over", "Speech and Broadcast", {-47, -11, -1}},
    {"Audiobook Narrator", "Speech and Broadcast", {-53, -8, -1}},
    {"Dialogue Leveler", "Speech and Broadcast", {-48, -10, -1}},
    {"Livestream Vocal", "Speech and Broadcast", {-40, -15, -2}},
    {"Rock Vocal", "Creative and Studio", {-40, -19, -2}},
    {"Acoustic Vocal", "Creative and Studio", {-55, -7, -1}},
    {"Whisper Vocal", "Creative and Studio", {-58, -18, -2}},
    {"Backing Vocals", "Creative and Studio", {-45, -12, -2}},
    {"Aggressive Vocal", "Creative and Studio", {-37, -23, -3}},
    {"Parallel Crush", "Creative and Studio", {-35, -32, -4}}};
} // namespace mo
