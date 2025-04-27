#include "commands.h"
#include "app_state.h"

void HandleCommand(HWND hwnd, WPARAM wParam) {
    switch (LOWORD(wParam)) {
    case 1: // Reset
        AppState::plotState.offsetX = 400;
        AppState::plotState.offsetY = 300;
        InvalidateRect(hwnd, NULL, TRUE);
        break;

    case 2: // Zoom +
        AppState::plotState.scale *= 1.1f;
        InvalidateRect(hwnd, NULL, TRUE);
        break;
    }
}