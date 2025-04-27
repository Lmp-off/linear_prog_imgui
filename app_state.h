#pragma once
#include <windows.h>
#include <vector>

// id для определения элемента
enum ElementId {
    BUTTON_RESET = 1,
    BUTTON_ZOOMIN,
    BUTTON_ZOOMOUT,
    BUTTON_ADDLINE,
    BUTTON_REMOVELINE,
    BUTTON_SHOW,
    CONDITIONS_PANEL,
    POINTS_PANEL,

    BUTTON_SIGN
};

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