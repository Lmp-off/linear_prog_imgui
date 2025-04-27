#include "app_state.h"

namespace AppState {
    HBRUSH hBackgroundBrush;
    PlotState plotState = { 400.0f, 300.0f, 1.0f, false, {0} };
    std::vector<std::pair<float, float>> lines;

    void Initialize() {
        hBackgroundBrush = CreateSolidBrush(RGB(212, 208, 200));
    }

    void Shutdown() {
        DeleteObject(hBackgroundBrush);
    }
}