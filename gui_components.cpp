#include "gui_components.h"
#include "resources.h"

void CreateMainControls(HWND hwnd) {
    CreateWindow(L"BUTTON", L"Reset", WS_VISIBLE | WS_CHILD,
        10, 10, 80, 30, hwnd, (HMENU)1, NULL, NULL);

    CreateWindow(L"BUTTON", L"Zoom +", WS_VISIBLE | WS_CHILD,
        100, 10, 80, 30, hwnd, (HMENU)2, NULL, NULL);
}