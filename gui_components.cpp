#include "gui_components.h"
#include "resources.h"

void createButton(HWND hwnd, int buttonId, int x, int y, int width, int height, const wchar_t* text) {
    CreateWindow(L"BUTTON", text, WS_VISIBLE | WS_CHILD,
        x, y, width, height, hwnd, (HMENU)buttonId, NULL, NULL);
}


void CreateMainControls(HWND hwnd) {
    createButton(hwnd, BUTTON_RESET, 10, 10, 80, 30, L"Reset");
    createButton(hwnd, BUTTON_ZOOMOUT, 10, 50, 80, 30, L"Zoom -");
    createButton(hwnd, BUTTON_ZOOMIN, 10, 90, 80, 30, L"Zoom +");
}