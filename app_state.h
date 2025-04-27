#pragma once
#include <windows.h>
#include <vector>

struct PlotState {
    float offsetX;
    float offsetY;
    float scale;
    bool isDragging;
    POINT dragStart;
};

namespace AppState {
    extern HBRUSH hBackgroundBrush;
    extern PlotState plotState;
    extern std::vector<std::pair<float, float>> lines;

    void Initialize();
    void Shutdown();
}