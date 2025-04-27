#include "gui_components.h"
#include "resources.h"

HWND createLabel(HWND hwnd, int x, int y, int width, int height, const wchar_t* text) {
    return CreateWindow(L"STATIC", text, WS_VISIBLE | WS_CHILD | WS_TABSTOP,
        x, y, width, height, hwnd, NULL, NULL, NULL);
}
HWND createEditBox(HWND hwnd, int x, int y, int width, int height, const wchar_t* text = L"") {
    return CreateWindow(L"EDIT", text, WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP,
        x, y, width, height, hwnd, NULL, NULL, NULL);
}

HWND createButton(HWND hwnd, int buttonId, int x, int y, int width, int height, const wchar_t* text) {
    return CreateWindow(L"BUTTON", text, WS_VISIBLE | WS_CHILD,
        x, y, width, height, hwnd, (HMENU)buttonId, NULL, NULL);
}

void CreateMainControls(HWND hwnd) {
    createButton(hwnd, BUTTON_RESET, 10, 10, 80, 30, L"Reset");
    createButton(hwnd, BUTTON_ZOOMOUT, 10, 50, 80, 30, L"Zoom -");
    createButton(hwnd, BUTTON_ZOOMIN, 10, 90, 80, 30, L"Zoom +");

    createEditBox(hwnd, 100, 10, 200, 30);
    createLabel(hwnd, 100, 50, 200, 30, L"X");
}