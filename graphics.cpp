#include "graphics.h"
#include "app_state.h"
#include <gdiplus.h>

ULONG_PTR gdiplusToken;

void InitializeGraphics() {
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
}

void ShutdownGraphics() {
    Gdiplus::GdiplusShutdown(gdiplusToken);
}

void DrawScene(HWND hwnd, HDC hdc) {
    RECT rc;
    GetClientRect(hwnd, &rc);

    // ���
    FillRect(hdc, &rc, AppState::hBackgroundBrush);

    // ���������
    Rectangle(hdc, 100, 100, 200, 200);
}