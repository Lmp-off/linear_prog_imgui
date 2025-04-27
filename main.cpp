// main.cpp
#include <windows.h>

#include "resources.h"
#include "app_state.h"
#include "gui_components.h"
#include "graphics.h"

const wchar_t CLASS_NAME[] = L"MainWindowClass";

// обработка событий окна
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        AppState::Initialize();
        InitializeGraphics();
        CreateMainControls(hwnd);
        break;
    case WM_DESTROY:
        ShutdownGraphics();
        AppState::Shutdown();
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

// точка входа
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow) {
    // Register window class
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    // Create window
    HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"Geometry Method",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600, NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}