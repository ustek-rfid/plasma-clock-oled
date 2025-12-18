#pragma once

namespace Config {
    constexpr int RepositionIntervalMs = 30000;  // 30 seconds
    constexpr int ClockUpdateIntervalMs = 1000;  // 1 second
    constexpr int BottomOffset = 4;              // pixels from bottom edge
    constexpr int HorizontalPadding = 20;        // min pixels from edges
    constexpr const char* FontFamily = "Sans";
    constexpr int FontSize = 14;
    constexpr const char* FontColor = "#999999"; // dimmed white
}
