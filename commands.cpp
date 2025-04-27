#include "commands.h"
#include "app_state.h"

void HandleCommand(HWND hwnd, WPARAM wParam) {
    switch (LOWORD(wParam)) {
    case BUTTON_RESET:
        AppState::plotState.offsetX = 400;
        AppState::plotState.offsetY = 300;
        InvalidateRect(hwnd, NULL, TRUE);
        break;

    case BUTTON_ZOOMIN:
        AppState::plotState.scale *= 1.1f;
        InvalidateRect(hwnd, NULL, TRUE);
        break;
    }
}