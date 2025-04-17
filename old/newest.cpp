
#include <windows.h>
#include <gdiplus.h>
#include <cmath>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iostream>
#pragma comment (lib, "Gdiplus.lib")

using namespace Gdiplus;

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lParam) ((int)(short)LOWORD(lParam))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lParam) ((int)(short)HIWORD(lParam))
#endif

// размеры окна
#define MIN_WIDTH 1024
#define MIN_HEIGHT 768
#define LEFT_GROUPBOX_WIDTH 380

// константы
const int BUTTON_AREA_WIDTH = 400;
const float INITIAL_SCALE = 50.0f;
const float ZOOM_FACTOR = 1.2f;
const int GRID_STEP = 5;

// id для определения элемента
enum ElementId {
    BUTTON_RESET = 1,
    BUTTON_ZOOMIN,
    BUTTON_ZOOMOUT,
    BUTTON_SIGN
};

// глобальные переменные
static ULONG_PTR gdiplusToken;
static HBRUSH hBackgroundBrush;

static HWND hRightGroupBox;

struct PlotState {
    POINT dragStart;
    float offsetX;
    float offsetY;
    bool isDragging;
    float scale;
};

// объявление функций
void DrawInfiniteGrid(HWND hwnd, Graphics& graphics, float offsetX, float offsetY, float scale, const RECT& plotArea);
void DrawAxesWithLabels(Graphics& graphics, float offsetX, float offsetY, float scale, const RECT& plotArea);

// =======================================================
// инициализация окна
// =======================================================
bool init(HWND hwnd) {
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    // hBackgroundBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
    hBackgroundBrush = CreateSolidBrush(RGB(212, 208, 200));

    // левая часть окна
    // ================================
    // левая и правая часть
    CreateWindow(
        L"BUTTON", NULL,
        WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
        10, 10, LEFT_GROUPBOX_WIDTH - 6, MIN_HEIGHT - 40, // Position and size
        hwnd, (HMENU)1, NULL, NULL);

    hRightGroupBox = CreateWindow(
        L"BUTTON", NULL,
        WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
        LEFT_GROUPBOX_WIDTH + 6, 10, MIN_WIDTH - LEFT_GROUPBOX_WIDTH - 16, MIN_HEIGHT - 40, // Position and size
        hwnd, (HMENU)2, NULL, NULL);

    // кнопки графика
    CreateWindow(L"BUTTON", L"Сброс", WS_VISIBLE | WS_CHILD,
        LEFT_GROUPBOX_WIDTH + 18, 26, 80, 30,
        hwnd, (HMENU)BUTTON_RESET, NULL, NULL);
    CreateWindow(L"BUTTON", L"+", WS_VISIBLE | WS_CHILD,
        LEFT_GROUPBOX_WIDTH + 18 + 120, 26, 40, 30,
        hwnd, (HMENU)BUTTON_ZOOMIN, NULL, NULL);
    CreateWindow(L"BUTTON", L"-", WS_VISIBLE | WS_CHILD,
        LEFT_GROUPBOX_WIDTH + 18 + 170, 26, 40, 30,
        hwnd, (HMENU)BUTTON_ZOOMOUT, NULL, NULL);

    // условие
    CreateWindow(L"STATIC", L"Условие", WS_VISIBLE | WS_CHILD,
        40, 20, 60, 20, hwnd, NULL, NULL, NULL);

    CreateWindow(L"STATIC", L"L=", WS_VISIBLE | WS_CHILD,
        20, 40, 20, 20, hwnd, NULL, NULL, NULL);
    CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
        40, 40, 60, 20, hwnd, NULL, NULL, NULL);
    CreateWindow(L"STATIC", L"x+", WS_VISIBLE | WS_CHILD,
        104, 40, 20, 20, hwnd, NULL, NULL, NULL);
    CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
        124, 40, 60, 20, hwnd, NULL, NULL, NULL);
    CreateWindow(L"STATIC", L"y -> max", WS_VISIBLE | WS_CHILD,
        186, 40, 60, 20, hwnd, NULL, NULL, NULL);

    CreateWindow(L"STATIC", L"1)", WS_VISIBLE | WS_CHILD,
        20, 66, 20, 20, hwnd, NULL, NULL, NULL);
    CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
        40, 66, 60, 20, hwnd, NULL, NULL, NULL);
    CreateWindow(L"STATIC", L"x+", WS_VISIBLE | WS_CHILD,
        104, 66, 20, 20, hwnd, NULL, NULL, NULL);
    CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
        124, 66, 60, 20, hwnd, NULL, NULL, NULL);
    CreateWindow(L"STATIC", L"y", WS_VISIBLE | WS_CHILD,
        186, 66, 20, 20, hwnd, NULL, NULL, NULL);
    CreateWindow(L"BUTTON", L"<=", WS_VISIBLE | WS_CHILD,
        196, 65, 22, 22,
        hwnd, (HMENU)BUTTON_SIGN, NULL, NULL);
    CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
        224, 66, 60, 20, hwnd, NULL, NULL, NULL);

    CreateWindow(L"STATIC", L"2)", WS_VISIBLE | WS_CHILD,
        20, 90, 20, 20, hwnd, NULL, NULL, NULL);
    CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
        40, 90, 60, 20, hwnd, NULL, NULL, NULL);
    CreateWindow(L"STATIC", L"x+", WS_VISIBLE | WS_CHILD,
        104, 90, 20, 20, hwnd, NULL, NULL, NULL);
    CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
        124, 90, 60, 20, hwnd, NULL, NULL, NULL);
    CreateWindow(L"STATIC", L"y", WS_VISIBLE | WS_CHILD,
        186, 90, 20, 20, hwnd, NULL, NULL, NULL);
    CreateWindow(L"BUTTON", L"<=", WS_VISIBLE | WS_CHILD,
        196, 89, 22, 22,
        hwnd, (HMENU)BUTTON_SIGN, NULL, NULL);
    CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
        224, 90, 60, 20, hwnd, NULL, NULL, NULL);

    CreateWindow(L"STATIC", L"3)", WS_VISIBLE | WS_CHILD,
        20, 114, 20, 20, hwnd, NULL, NULL, NULL);
    CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
        40, 114, 60, 20, hwnd, NULL, NULL, NULL);
    CreateWindow(L"STATIC", L"x+", WS_VISIBLE | WS_CHILD,
        104, 114, 20, 20, hwnd, NULL, NULL, NULL);
    CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
        124, 114, 60, 20, hwnd, NULL, NULL, NULL);
    CreateWindow(L"STATIC", L"y", WS_VISIBLE | WS_CHILD,
        186, 114, 20, 20, hwnd, NULL, NULL, NULL);
    CreateWindow(L"BUTTON", L"<=", WS_VISIBLE | WS_CHILD,
        196, 113, 22, 22,
        hwnd, (HMENU)BUTTON_SIGN, NULL, NULL);
    CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
        224, 114, 60, 20, hwnd, NULL, NULL, NULL);

    CreateWindow(L"STATIC", L"x>=0, y>=0", WS_VISIBLE | WS_CHILD,
        60, 138, 100, 20, hwnd, NULL, NULL, NULL);

    // прямые
    CreateWindow(L"STATIC", L"Прямые", WS_VISIBLE | WS_CHILD,
        40, 170, 60, 20, hwnd, NULL, NULL, NULL);

    CreateWindow(L"STATIC", L"x1", WS_VISIBLE | WS_CHILD,
        60, 190, 20, 20, hwnd, NULL, NULL, NULL);
    CreateWindow(L"STATIC", L"y1", WS_VISIBLE | WS_CHILD,
        95, 190, 20, 20, hwnd, NULL, NULL, NULL);
    CreateWindow(L"STATIC", L"x2", WS_VISIBLE | WS_CHILD,
        130, 190, 20, 20, hwnd, NULL, NULL, NULL);
    CreateWindow(L"STATIC", L"y2", WS_VISIBLE | WS_CHILD,
        165, 190, 20, 20, hwnd, NULL, NULL, NULL);

    for (int i = 0; i < 3; i++) {
        std::wstring label = std::to_wstring(i + 1) + L")";
        CreateWindow(L"STATIC", label.c_str(), WS_VISIBLE | WS_CHILD,
            20, 210 + i * 20, 20, 20, hwnd, NULL, NULL, NULL);

        for (int j = 0; j < 4; j++) {
            CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
                40 + j * 40, 210 + i * 20, 40, 20, hwnd, NULL, NULL, NULL);
        }
    }


    // Conditions table
    // const wchar_t* headers[] = { L"1", L"2", L"3" };
    // for (int col = 0; col < 3; col++) {
    //     CreateWindow(L"STATIC", headers[col], WS_VISIBLE | WS_CHILD | SS_CENTER,
    //         40 + col * 120, 40, 30, 20, hwnd, NULL, NULL, NULL);
    // }

    // const wchar_t* labels[] = { L"x1", L"y1", L"x2", L"y2" };
    // for (int row = 0; row < 4; row++) {
    //     CreateWindow(L"STATIC", labels[row], WS_VISIBLE | WS_CHILD,
    //         10, 70 + row * 30, 30, 20, hwnd, NULL, NULL, NULL);

    //     for (int col = 0; col < 3; col++) {
    //         CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
    //             40 + col * 120, 70 + row * 30, 100, 20, hwnd, NULL, NULL, NULL);
    //     }
    // }


    return true;
}

// =======================================================
// обработка событий
// =======================================================
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static PlotState state = { {0}, 0.0f, 0.0f, false, INITIAL_SCALE };

    switch (msg) {
    case WM_CREATE: {
        init(hwnd);

        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        int plotWidth = clientRect.right - BUTTON_AREA_WIDTH;
        state.offsetX = plotWidth / 2.0f;
        state.offsetY = clientRect.bottom / 2.0f;
        state.scale = INITIAL_SCALE;

        break;
    }
    case WM_SIZE: {
        RECT rc;
        GetClientRect(hwnd, &rc);
        InvalidateRect(hwnd, NULL, TRUE);

        // изменение размеров окна
        MoveWindow(hRightGroupBox,
            LEFT_GROUPBOX_WIDTH + 10, 10,
            rc.right - LEFT_GROUPBOX_WIDTH - 16, max(rc.bottom - 20, MIN_HEIGHT - 40),
            TRUE);
        UpdateWindow(hwnd);

        break;
    }

                // case WM_CTLCOLORSTATIC: {
                //     HDC hdcStatic = (HDC)wParam;
                //     SetBkColor(hdcStatic, GetSysColor(COLOR_WINDOW));
                //     return (LRESULT)hBackgroundBrush;
                // }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN: {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
        return (LRESULT)hBackgroundBrush;
    }

    case WM_ERASEBKGND: {
        RECT rc;
        GetClientRect(hwnd, &rc);
        FillRect((HDC)wParam, &rc, hBackgroundBrush);
        return 1;
    }

    case WM_GETMINMAXINFO: {
        MINMAXINFO* mmi = (MINMAXINFO*)lParam;
        mmi->ptMinTrackSize.x = MIN_WIDTH;
        mmi->ptMinTrackSize.y = MIN_HEIGHT;
        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc;
        GetClientRect(hwnd, &rc);

        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hbmMem = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
        SelectObject(hdcMem, hbmMem);

        // отрисовка
        // ================================
        // заполнение фона
        FillRect(hdcMem, &rc, hBackgroundBrush);

        // отрисовка графика
        Graphics graphics(hdcMem);
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);

        RECT plotArea = rc;
        plotArea.left = BUTTON_AREA_WIDTH;

        graphics.SetClip(Rect(
            plotArea.left - 2,
            plotArea.top + 60,
            plotArea.right - plotArea.left - 14,
            plotArea.bottom - plotArea.top - 70
        ));
        SolidBrush plotBrush(GetSysColor(COLOR_WINDOW));

        // graphics.FillRectangle(&plotBrush, 0, 0, plotArea.right - plotArea.left, plotArea.bottom - plotArea.top);
        DrawInfiniteGrid(hwnd, graphics, state.offsetX, state.offsetY, state.scale, plotArea);
        DrawAxesWithLabels(graphics, state.offsetX, state.offsetY, state.scale, plotArea);

        // освобождение ресурсов
        BitBlt(hdc, 0, 0, rc.right, rc.bottom, hdcMem, 0, 0, SRCCOPY);
        DeleteObject(hbmMem);
        DeleteDC(hdcMem);

        EndPaint(hwnd, &ps);
        break;
    }
    // обработка нажатий
    case WM_COMMAND: {
        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        int plotWidth = clientRect.right - BUTTON_AREA_WIDTH;

        switch (LOWORD(wParam)) {
        // case 1: {
        //     state.points.clear();
        //     wchar_t text[32];
        //     for (int i = 0; i < 6; i++) {
        //         float x = 0, y = 0;
        //         GetWindowTextW(hEditX[i], text, 32);
        //         x = _wtof(text);
        //         GetWindowTextW(hEditY[i], text, 32);
        //         y = _wtof(text);
        //         state.points.emplace_back(x, y); // Remove Y inversion
        //     }
        //     break;
        // }
        case 1:
            state.offsetX = plotWidth / 2.0f;
            state.offsetY = clientRect.bottom / 2.0f;
            state.scale = INITIAL_SCALE;
            break;

        case 2:
            state.scale *= ZOOM_FACTOR;
            break;

        case 3:
            state.scale /= ZOOM_FACTOR;
            break;
        }
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    }
    // нажатие ЛКМ
    case WM_LBUTTONDOWN: {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        RECT plotArea;
        GetClientRect(hwnd, &plotArea);

        if (pt.x > BUTTON_AREA_WIDTH) {
            state.dragStart = pt;
            state.isDragging = true;
            SetCapture(hwnd);
        }
        return 0;
    }
    // перемещение мыши
    case WM_MOUSEMOVE:
        if (state.isDragging) {
            int dx = GET_X_LPARAM(lParam) - state.dragStart.x;
            int dy = GET_Y_LPARAM(lParam) - state.dragStart.y;

            state.offsetX += dx;
            state.offsetY += dy;

            state.dragStart.x = GET_X_LPARAM(lParam);
            state.dragStart.y = GET_Y_LPARAM(lParam);

            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    // отжатие кнопки
    case WM_LBUTTONUP:
        state.isDragging = false;
        ReleaseCapture();
        return 0;
    // закрытие окна
    case WM_DESTROY: {
        DeleteObject(hBackgroundBrush);
        GdiplusShutdown(gdiplusToken);
        PostQuitMessage(0);
        break;
    }
    default: {
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    }

    return 0;
}


// =======================================================
// точка входа
// =======================================================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"PlotterWindowClass";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"Геометрический метод",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        MIN_WIDTH, MIN_HEIGHT,
        NULL, NULL, hInstance, NULL
    );

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

// =======================================================
// преобразование координат
// =======================================================
float ScreenToWorldX(int x, float offsetX, float scale) {
    return (x - BUTTON_AREA_WIDTH - offsetX) / scale;
}

float ScreenToWorldY(int y, float offsetY, float scale) {
    return (y - offsetY) / scale;
}

int WorldToScreenX(float worldX, float offsetX, float scale) {
    return BUTTON_AREA_WIDTH + static_cast<int>(worldX * scale + offsetX);
}

int WorldToScreenY(float worldY, float offsetY, float scale) {
    return static_cast<int>(worldY * scale + offsetY);
}

// =======================================================
// отрисовка линий
// =======================================================
void DrawNormalLine(Graphics& graphics, int x1, int y1, int x2, int y2) {
    Pen linePen(Color(150, 200, 200, 200));
    graphics.DrawLine(&linePen, x1, y1, x2, y2);
}

void DrawHatchedLine(Graphics& graphics, int x1, int y1, int x2, int y2) {
    HatchBrush hatchBrush(HatchStyleBackwardDiagonal, 
                        Color(150, 200, 200, 200),
                        Color(0, 0, 0, 0));
    Pen hatchPen(&hatchBrush, 1);
    graphics.DrawLine(&hatchPen, x1, y1, x2, y2);
}

// =======================================================
// шаг сетки
// =======================================================
float ComputeGridStep(float scale) {
    const float targetPixels = 50.0f; // Target 50 pixels between grid lines
    float approxStep = targetPixels / scale;

    // Calculate magnitude and normalize
    float magnitude = powf(10, floorf(log10f(approxStep)));
    float normalized = approxStep / magnitude;

    // Round to nearest 1, 2, or 5
    if (normalized < 1.5f) {
        return 1.0f * magnitude;
    } else if (normalized < 3.0f) {
        return 2.0f * magnitude;
    } else if (normalized < 7.0f) {
        return 5.0f * magnitude;
    } else {
        return 10.0f * magnitude;
    }
}

// =======================================================
// отрисовка сетки
// =======================================================
void DrawGridLines(Graphics& graphics, float offsetX, float offsetY, float scale, 
                   const RECT& plotArea, float gridStep) {
    // Vertical lines
    float left = ScreenToWorldX(plotArea.left, offsetX, scale);
    float right = ScreenToWorldX(plotArea.right, offsetX, scale);
    float startX = floorf(left / gridStep) * gridStep;
    float endX = ceilf(right / gridStep) * gridStep;

    for (float x = startX; x <= endX; x += gridStep) {
        int screenX = BUTTON_AREA_WIDTH + static_cast<int>(x * scale + offsetX);
        if (x > 0) { // Right of Y-axis
            DrawHatchedLine(graphics, screenX, plotArea.top, screenX, plotArea.bottom);
        } else {
            DrawNormalLine(graphics, screenX, plotArea.top, screenX, plotArea.bottom);
        }
    }

    // Horizontal lines
    float top = ScreenToWorldY(plotArea.top, offsetY, scale);
    float bottom = ScreenToWorldY(plotArea.bottom, offsetY, scale);
    float startY = floorf(top / gridStep) * gridStep;
    float endY = ceilf(bottom / gridStep) * gridStep;

    for (float y = startY; y <= endY; y += gridStep) {
        int screenY = static_cast<int>(y * scale + offsetY);
        if (y > 0) { // Above X-axis (world coordinates)
            DrawHatchedLine(graphics, BUTTON_AREA_WIDTH, screenY, plotArea.right, screenY);
        } else {
            DrawNormalLine(graphics, BUTTON_AREA_WIDTH, screenY, plotArea.right, screenY);
        }
    }
}


// =======================================================
// Updated Grid Drawing Entry Point
// =======================================================
void DrawInfiniteGrid(HWND hwnd, Graphics& graphics, float offsetX, float offsetY,
                     float scale, const RECT& plotArea) {
    RECT rc;
    GetClientRect(hwnd, &rc);

    // Background
    SolidBrush white(Color(255, 255, 255));
    graphics.FillRectangle(&white, 0, 0, rc.right, rc.bottom);

    // Calculate dynamic grid step
    float gridStep = ComputeGridStep(scale);

    // Draw grid using line functions
    DrawGridLines(graphics, offsetX, offsetY, scale, plotArea, gridStep);
}

// =======================================================
// Axes Drawing Using Line Functions
// =======================================================
void DrawAxesWithLabels(Graphics& graphics, float offsetX, float offsetY, float scale,
                        const RECT& plotArea) {
    // Create axis-specific pen
    Pen axisPen(Color(255, 0, 0, 0), 2);

    // Calculate axis positions
    int xAxisY = static_cast<int>(0 * scale + offsetY);
    int yAxisX = BUTTON_AREA_WIDTH + static_cast<int>(0 * scale + offsetX);

    // Draw X-axis
    DrawNormalLine(graphics, BUTTON_AREA_WIDTH, xAxisY, plotArea.right, xAxisY);
    
    // Draw Y-axis
    DrawNormalLine(graphics, yAxisX, plotArea.top, yAxisX, plotArea.bottom);

    // Rest of label drawing code remains the same...
    // [Label drawing implementation from original code]
}

// // =======================================================
// // Infinite Grid Drawing
// // =======================================================
// void DrawInfiniteGrid(HWND hwnd, Graphics& graphics, float offsetX, float offsetY, 
//                       float scale, const RECT& plotArea) {
//     RECT rc;
//     GetClientRect(hwnd, &rc);

//     // Background
//     SolidBrush white(Color(255, 255, 255));
//     graphics.FillRectangle(&white, 0, 0, rc.right, rc.bottom);

//     // Calculate dynamic grid step
//     float gridStep = ComputeGridStep(scale);

//     // Draw grid lines
//     DrawGridLines(graphics, offsetX, offsetY, scale, plotArea, gridStep);
// }

// // =======================================================
// // Axes Drawing with Hatching
// // =======================================================
// void DrawAxesWithLabels(Graphics& graphics, float offsetX, float offsetY, float scale, 
//                         const RECT& plotArea) {
//     Pen axisPen(Color(255, 0, 0, 0), 2);
//     SolidBrush textBrush(Color(0, 0, 0));
//     Font font(L"Arial", 10);

//     // Calculate axis positions
//     int screenY = static_cast<int>(0 * scale + offsetY);       // X-axis
//     int screenX = BUTTON_AREA_WIDTH + static_cast<int>(0 * scale + offsetX); // Y-axis

//     // Draw X-axis
//     graphics.DrawLine(&axisPen, BUTTON_AREA_WIDTH, screenY, plotArea.right, screenY);

//     // Draw Y-axis
//     graphics.DrawLine(&axisPen, screenX, plotArea.top, screenX, plotArea.bottom);

//     // Hatching for x>0 and y>0
//     int hatchLeft = max(screenX, BUTTON_AREA_WIDTH);
//     int hatchTop = max(screenY, plotArea.top);
//     int hatchRight = plotArea.right;
//     int hatchBottom = plotArea.bottom;

//     if (hatchLeft < hatchRight && hatchTop < hatchBottom) {
//         HatchBrush hatchBrush(
//             HatchStyleBackwardDiagonal, 
//             Color(50, 200, 200, 200),  // Semi-transparent hatch
//             Color(0, 0, 0, 0)          // Transparent background
//         );
//         graphics.FillRectangle(
//             &hatchBrush,
//             hatchLeft,
//             hatchTop,
//             hatchRight - hatchLeft,
//             hatchBottom - hatchTop
//         );
//     }

//     // Rest of label drawing code remains the same...
//     // [Label drawing code from original implementation]
// }
// // =======================================================
// // отрисовка сетки
// // =======================================================
// void DrawInfiniteGrid(HWND hwnd, Graphics& graphics, float offsetX, float offsetY, float scale, const RECT& plotArea) {
//     RECT rc;
//     GetClientRect(hwnd, &rc);

//     // фон
//     SolidBrush white(Color(255, 255, 255));
//     graphics.FillRectangle(&white, 0, 0, rc.right, rc.bottom);

//     Pen gridPen(Color(150, 200, 200, 200));
//     SolidBrush textBrush(Color(120, 120, 120));
//     Font font(L"Arial", 10);

//     float left = ScreenToWorldX(plotArea.left, offsetX, scale);
//     float right = ScreenToWorldX(plotArea.right, offsetX, scale);
//     float top = ScreenToWorldY(plotArea.top, offsetY, scale);
//     float bottom = ScreenToWorldY(plotArea.bottom, offsetY, scale);

//     // вертикальные линии
//     float startX = floor(left / GRID_STEP) * GRID_STEP;
//     float endX = ceil(right / GRID_STEP) * GRID_STEP;
//     for (float x = startX; x <= endX; x += GRID_STEP) {
//         int screenX = BUTTON_AREA_WIDTH + static_cast<int>(x * scale + offsetX);
//         graphics.DrawLine(&gridPen, screenX, plotArea.top, screenX, plotArea.bottom);
//     }

//     // горизонтальные линии
//     float startY = floor(top / GRID_STEP) * GRID_STEP;
//     float endY = ceil(bottom / GRID_STEP) * GRID_STEP;
//     for (float y = startY; y <= endY; y += GRID_STEP) {
//         int screenY = static_cast<int>(y * scale + offsetY);
//         graphics.DrawLine(&gridPen, BUTTON_AREA_WIDTH, screenY, plotArea.right, screenY);
//     }
// }

// // =======================================================
// // отрисовка осей
// // =======================================================
// void DrawAxesWithLabels(Graphics& graphics, float offsetX, float offsetY, float scale, const RECT& plotArea) {
//     Pen axisPen(Color(255, 0, 0, 0), 2);
//     SolidBrush textBrush(Color(0, 0, 0));
//     Font font(L"Arial", 10);

//     // X axis
//     int screenY = static_cast<int>(0 * scale + offsetY);
//     graphics.DrawLine(&axisPen,
//         BUTTON_AREA_WIDTH, screenY,
//         plotArea.right, screenY);

//     // Y axis
//     int screenX = BUTTON_AREA_WIDTH + static_cast<int>(0 * scale + offsetX);
//     graphics.DrawLine(&axisPen,
//         screenX, plotArea.top,
//         screenX, plotArea.bottom);

//     // X labels
//     float xStart = floor(ScreenToWorldX(BUTTON_AREA_WIDTH, offsetX, scale) / GRID_STEP) * GRID_STEP;
//     float xEnd = ceil(ScreenToWorldX(plotArea.right, offsetX, scale) / GRID_STEP) * GRID_STEP;
//     for (float x = xStart; x <= xEnd; x += GRID_STEP) {
//         if (x == 0) continue;
//         int labelX = BUTTON_AREA_WIDTH + static_cast<int>(x * scale + offsetX);
//         std::wstring text = std::to_wstring(static_cast<int>(x));
//         graphics.DrawString(text.c_str(), -1, &font,
//             PointF(labelX, screenY + 15), &textBrush);
//     }

//     // Y labels
//     float yStart = floor(ScreenToWorldY(plotArea.top, offsetY, scale) / GRID_STEP) * GRID_STEP;
//     float yEnd = ceil(ScreenToWorldY(plotArea.bottom, offsetY, scale) / GRID_STEP) * GRID_STEP;
//     for (float y = yStart; y <= yEnd; y += GRID_STEP) {
//         if (y == 0) continue;
//         int labelY = static_cast<int>(y * scale + offsetY);
//         std::wstring text = std::to_wstring(static_cast<int>(-y));
//         graphics.DrawString(text.c_str(), -1, &font,
//             PointF(screenX + 15, labelY), &textBrush);
//     }

//     // Origin label
//     graphics.DrawString(L"0", -1, &font,
//         PointF(screenX + 5, screenY + 5), &textBrush);
// }
