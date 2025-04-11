#include <windows.h>
#include <gdiplus.h>
#include <cmath>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#pragma comment (lib, "Gdiplus.lib")

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lParam) ((int)(short)LOWORD(lParam))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lParam) ((int)(short)HIWORD(lParam))
#endif

using namespace Gdiplus;

struct ScrollState {
    POINT dragStart;
    float offsetX;
    float offsetY;
    bool isDragging;
    float scale;
    std::vector<PointF> points;
};

const int BUTTON_AREA_WIDTH = 220;
const float INITIAL_SCALE = 50.0f;
const float ZOOM_FACTOR = 1.2f;
const int GRID_STEP = 5;

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

void DrawInfiniteGrid(Graphics& graphics, float offsetX, float offsetY, float scale, const RECT& plotArea) {
    Pen gridPen(Color(150, 220, 220, 220));
    SolidBrush textBrush(Color(120, 120, 120));
    Font font(L"Arial", 10);

    float left = ScreenToWorldX(plotArea.left, offsetX, scale);
    float right = ScreenToWorldX(plotArea.right, offsetX, scale);
    float top = ScreenToWorldY(plotArea.top, offsetY, scale);
    float bottom = ScreenToWorldY(plotArea.bottom, offsetY, scale);

    // Vertical grid lines
    float startX = floor(left / GRID_STEP) * GRID_STEP;
    float endX = ceil(right / GRID_STEP) * GRID_STEP;
    for (float x = startX; x <= endX; x += GRID_STEP) {
        int screenX = BUTTON_AREA_WIDTH + static_cast<int>(x * scale + offsetX);
        graphics.DrawLine(&gridPen, screenX, plotArea.top, screenX, plotArea.bottom);
    }

    // Horizontal grid lines
    float startY = floor(top / GRID_STEP) * GRID_STEP;
    float endY = ceil(bottom / GRID_STEP) * GRID_STEP;
    for (float y = startY; y <= endY; y += GRID_STEP) {
        int screenY = static_cast<int>(y * scale + offsetY);
        graphics.DrawLine(&gridPen, BUTTON_AREA_WIDTH, screenY, plotArea.right, screenY);
    }
}

void DrawAxesWithLabels(Graphics& graphics, float offsetX, float offsetY, float scale, const RECT& plotArea) {
    Pen axisPen(Color(255, 0, 0, 0), 2);
    SolidBrush textBrush(Color(0, 0, 0));
    Font font(L"Arial", 10);

    // X axis
    int screenY = static_cast<int>(0 * scale + offsetY);
    graphics.DrawLine(&axisPen,
        BUTTON_AREA_WIDTH, screenY,
        plotArea.right, screenY);

    // Y axis
    int screenX = BUTTON_AREA_WIDTH + static_cast<int>(0 * scale + offsetX);
    graphics.DrawLine(&axisPen,
        screenX, plotArea.top,
        screenX, plotArea.bottom);

    // X labels
    float xStart = floor(ScreenToWorldX(BUTTON_AREA_WIDTH, offsetX, scale) / GRID_STEP) * GRID_STEP;
    float xEnd = ceil(ScreenToWorldX(plotArea.right, offsetX, scale) / GRID_STEP) * GRID_STEP;
    for (float x = xStart; x <= xEnd; x += GRID_STEP) {
        if (x == 0) continue;
        int labelX = BUTTON_AREA_WIDTH + static_cast<int>(x * scale + offsetX);
        std::wstring text = std::to_wstring(static_cast<int>(x));
        graphics.DrawString(text.c_str(), -1, &font,
            PointF(labelX, screenY + 15), &textBrush);
    }

    // Y labels
    float yStart = floor(ScreenToWorldY(plotArea.top, offsetY, scale) / GRID_STEP) * GRID_STEP;
    float yEnd = ceil(ScreenToWorldY(plotArea.bottom, offsetY, scale) / GRID_STEP) * GRID_STEP;
    for (float y = yStart; y <= yEnd; y += GRID_STEP) {
        if (y == 0) continue;
        int labelY = static_cast<int>(y * scale + offsetY);
        std::wstring text = std::to_wstring(static_cast<int>(-y));
        graphics.DrawString(text.c_str(), -1, &font,
            PointF(screenX + 15, labelY), &textBrush);
    }

    // Origin label
    graphics.DrawString(L"0", -1, &font,
        PointF(screenX + 5, screenY + 5), &textBrush);
}

void DrawInfiniteLine(Graphics& graphics, const PointF& p1, const PointF& p2,
    float offsetX, float offsetY, float scale, const RECT& plotArea, Color color) {
    if (p1.X == p2.X && p1.Y == p2.Y) return;

    Pen linePen(color, 2);
    float worldLeft = ScreenToWorldX(plotArea.left, offsetX, scale);
    float worldRight = ScreenToWorldX(plotArea.right, offsetX, scale);

    // Calculate line equation y = mx + b
    float dx = p2.X - p1.X;
    float dy = p2.Y - p1.Y;

    // In DrawInfiniteLine function, fix vertical line detection:
    if (fabs(dx) < 1e-6) {
        float x = p1.X;
        int screenX = WorldToScreenX(x, offsetX, scale);
        graphics.DrawLine(&linePen, screenX, plotArea.top, screenX, plotArea.bottom);
    }
    else {
        float m = dy / dx;
        float b = p1.Y - m * p1.X;

        PointF start, end;
        start.X = worldLeft;
        start.Y = m * start.X + b;
        end.X = worldRight;
        end.Y = m * end.X + b;

        int x1 = WorldToScreenX(start.X, offsetX, scale);
        int y1 = WorldToScreenY(start.Y, offsetY, scale);
        int x2 = WorldToScreenX(end.X, offsetX, scale);
        int y2 = WorldToScreenY(end.Y, offsetY, scale);

        graphics.DrawLine(&linePen, x1, y1, x2, y2);
    }
}

void DrawHatchedRegionForLine(Graphics& graphics, float m, float b, bool isVertical, float xVertical,
    float offsetX, float offsetY, float scale, const RECT& plotArea, bool isBelow) {

    HatchBrush hatchBrush(HatchStyleForwardDiagonal, Color(200, 0, 0, 200), Color(50, 255, 255, 255));
    GraphicsPath path;

    float worldLeft = ScreenToWorldX(plotArea.left, offsetX, scale);
    float worldRight = ScreenToWorldX(plotArea.right, offsetX, scale);
    float worldTop = ScreenToWorldY(plotArea.top, offsetY, scale);
    float worldBottom = ScreenToWorldY(plotArea.bottom, offsetY, scale);

    if (isVertical) {
        float x = xVertical;
        int screenX = WorldToScreenX(x, offsetX, scale);
        if (x <= worldLeft) return;
        if (x >= worldRight) {
            path.AddRectangle(Rect(plotArea.left, plotArea.top, plotArea.right - plotArea.left, plotArea.bottom - plotArea.top));
        }
        else {
            if (isBelow) {
                path.AddRectangle(Rect(plotArea.left, plotArea.top, screenX - plotArea.left, plotArea.bottom - plotArea.top));
            }
            else {
                path.AddRectangle(Rect(screenX, plotArea.top, plotArea.right - screenX, plotArea.bottom - plotArea.top));
            }
        }
    }
    else {
        float yLeft = m * worldLeft + b;
        float yRight = m * worldRight + b;

        yLeft = std::clamp(yLeft, worldTop, worldBottom);
        yRight = std::clamp(yRight, worldTop, worldBottom);

        int screenLeftX = WorldToScreenX(worldLeft, offsetX, scale);
        int screenLeftY = WorldToScreenY(yLeft, offsetY, scale);
        int screenRightX = WorldToScreenX(worldRight, offsetX, scale);
        int screenRightY = WorldToScreenY(yRight, offsetY, scale);

        path.StartFigure();
        if (isBelow) {
            path.AddLine(screenLeftX, plotArea.bottom, screenLeftX, screenLeftY);
            path.AddLine(screenLeftX, screenLeftY, screenRightX, screenRightY);
            path.AddLine(screenRightX, screenRightY, screenRightX, plotArea.bottom);
        }
        else {
            path.AddLine(screenLeftX, plotArea.top, screenLeftX, screenLeftY);
            path.AddLine(screenLeftX, screenLeftY, screenRightX, screenRightY);
            path.AddLine(screenRightX, screenRightY, screenRightX, plotArea.top);
        }
        path.CloseFigure();
    }

    graphics.FillPath(&hatchBrush, &path);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static ScrollState scroll = { {0}, 0.0f, 0.0f, false, INITIAL_SCALE, {} };
    static ULONG_PTR gdiplusToken;
    static HWND hEditX[6], hEditY[6], hButtonDraw, hButtonReset, hButtonZoomIn, hButtonZoomOut;

    switch (msg) {
    case WM_CREATE: {
        GdiplusStartupInput gdiplusStartupInput;
        GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

        hButtonReset = CreateWindowW(L"BUTTON", L"Reset", WS_VISIBLE | WS_CHILD,
            10, 10, 60, 30, hwnd, (HMENU)2, NULL, NULL);
        hButtonZoomIn = CreateWindowW(L"BUTTON", L"Zoom+", WS_VISIBLE | WS_CHILD,
            80, 10, 60, 30, hwnd, (HMENU)3, NULL, NULL);
        hButtonZoomOut = CreateWindowW(L"BUTTON", L"Zoom-", WS_VISIBLE | WS_CHILD,
            150, 10, 60, 30, hwnd, (HMENU)4, NULL, NULL);
        hButtonDraw = CreateWindowW(L"BUTTON", L"Draw", WS_VISIBLE | WS_CHILD,
            10, 240, 90, 30, hwnd, (HMENU)1, NULL, NULL);

        int startY = 50;
        for (int line = 0; line < 3; line++) {
            int xCol = 10 + line * 140;
            wchar_t lineLabel[10];
            wsprintf(lineLabel, L"Line %d", line + 1);
            CreateWindowW(L"STATIC", lineLabel, WS_VISIBLE | WS_CHILD,
                xCol, startY, 100, 20, hwnd, NULL, NULL, NULL);

            CreateWindowW(L"STATIC", L"X1:", WS_VISIBLE | WS_CHILD,
                xCol, startY + 30, 30, 20, hwnd, NULL, NULL, NULL);
            hEditX[line * 2] = CreateWindowW(L"EDIT", L"0", WS_VISIBLE | WS_CHILD | WS_BORDER,
                xCol + 40, startY + 30, 50, 20, hwnd, NULL, NULL, NULL);

            CreateWindowW(L"STATIC", L"Y1:", WS_VISIBLE | WS_CHILD,
                xCol, startY + 60, 30, 20, hwnd, NULL, NULL, NULL);
            hEditY[line * 2] = CreateWindowW(L"EDIT", L"0", WS_VISIBLE | WS_CHILD | WS_BORDER,
                xCol + 40, startY + 60, 50, 20, hwnd, NULL, NULL, NULL);

            CreateWindowW(L"STATIC", L"X2:", WS_VISIBLE | WS_CHILD,
                xCol, startY + 90, 30, 20, hwnd, NULL, NULL, NULL);
            hEditX[line * 2 + 1] = CreateWindowW(L"EDIT", L"0", WS_VISIBLE | WS_CHILD | WS_BORDER,
                xCol + 40, startY + 90, 50, 20, hwnd, NULL, NULL, NULL);

            CreateWindowW(L"STATIC", L"Y2:", WS_VISIBLE | WS_CHILD,
                xCol, startY + 120, 30, 20, hwnd, NULL, NULL, NULL);
            hEditY[line * 2 + 1] = CreateWindowW(L"EDIT", L"0", WS_VISIBLE | WS_CHILD | WS_BORDER,
                xCol + 40, startY + 120, 50, 20, hwnd, NULL, NULL, NULL);
        }
        return 0;
    }

    case WM_COMMAND: {
        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        int plotWidth = clientRect.right - BUTTON_AREA_WIDTH;

        switch (LOWORD(wParam)) {
        case 1: {
            scroll.points.clear();
            wchar_t text[32];
            for (int i = 0; i < 6; i++) {
                float x = 0, y = 0;
                GetWindowTextW(hEditX[i], text, 32);
                x = _wtof(text);
                GetWindowTextW(hEditY[i], text, 32);
                y = _wtof(text);
                scroll.points.emplace_back(x, y); // Remove Y inversion
            }
            break;
        }
        case 2:
            scroll.offsetX = plotWidth / 2.0f;
            scroll.offsetY = clientRect.bottom / 2.0f;
            scroll.scale = INITIAL_SCALE;
            break;

        case 3:
            scroll.scale *= ZOOM_FACTOR;
            break;

        case 4:
            scroll.scale /= ZOOM_FACTOR;
            break;
        }
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        RECT plotArea;
        GetClientRect(hwnd, &plotArea);

        if (pt.x > BUTTON_AREA_WIDTH) {
            scroll.dragStart = pt;
            scroll.isDragging = true;
            SetCapture(hwnd);
        }
        return 0;
    }

    case WM_MOUSEMOVE:
        if (scroll.isDragging) {
            int dx = GET_X_LPARAM(lParam) - scroll.dragStart.x;
            int dy = GET_Y_LPARAM(lParam) - scroll.dragStart.y;

            scroll.offsetX += dx;
            scroll.offsetY += dy;

            scroll.dragStart.x = GET_X_LPARAM(lParam);
            scroll.dragStart.y = GET_Y_LPARAM(lParam);

            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;

    case WM_LBUTTONUP:
        scroll.isDragging = false;
        ReleaseCapture();
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT clientRect;
        GetClientRect(hwnd, &clientRect);

        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hbmMem = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
        SelectObject(hdcMem, hbmMem);

        Graphics graphics(hdcMem);
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);
        SolidBrush bgBrush(Color(255, 255, 255));
        graphics.FillRectangle(&bgBrush, 0, 0, clientRect.right, clientRect.bottom);

        SolidBrush panelBrush(Color(240, 240, 240));
        graphics.FillRectangle(&panelBrush, 0, 0, BUTTON_AREA_WIDTH, clientRect.bottom);

        RECT plotArea = clientRect;
        plotArea.left = BUTTON_AREA_WIDTH;

        graphics.SetClip(Rect(plotArea.left, plotArea.top,
            plotArea.right - plotArea.left, plotArea.bottom - plotArea.top));

        DrawInfiniteGrid(graphics, scroll.offsetX, scroll.offsetY, scroll.scale, plotArea);
        DrawAxesWithLabels(graphics, scroll.offsetX, scroll.offsetY, scroll.scale, plotArea);

        // if (scroll.points.size() == 6) {
        //     for (int i = 0; i < 3; i++) {
        //         const auto& p1 = scroll.points[i * 2];
        //         const auto& p2 = scroll.points[i * 2 + 1];

        //         float dx = p2.X - p1.X;
        //         float dy = p2.Y - p1.Y;
        //         bool isVertical = fabs(dx) < 1e-6;
        //         float m = 0.0f, b = 0.0f, xVertical = p1.X;

        //         if (!isVertical) {
        //             m = dy / dx;
        //             b = p1.Y - m * p1.X;
        //         }

        //         DrawHatchedRegionForLine(graphics, m, b, isVertical, xVertical,
        //             scroll.offsetX, scroll.offsetY, scroll.scale, plotArea, true);
        //     }
        // }

        // draw hatched area
        if (scroll.points.size() == 6) {
            HatchBrush hatchBrush(HatchStyleForwardDiagonal, Color(200, 0, 0, 200), Color(50, 255, 255, 255));
            GraphicsPath path;

            float worldLeft = ScreenToWorldX(plotArea.left, scroll.offsetX, scroll.scale);
            float worldRight = ScreenToWorldX(plotArea.right, scroll.offsetX, scroll.scale);
            float worldBottom = ScreenToWorldY(plotArea.bottom, scroll.offsetY, scroll.scale);

            // Find first x-axis intersection (including vertical lines)
            float firstXIntercept = INFINITY;
            for (int i = 0; i < 3; i++) {
                const PointF& p1 = scroll.points[i * 2];
                const PointF& p2 = scroll.points[i * 2 + 1];

                float dx = p2.X - p1.X;
                bool isVertical = fabs(dx) < 1e-6f;

                if (isVertical) {
                    // Vertical line at x=p1.X, check if it intersects x-axis (y=0)
                    float xVertical = p1.X;
                    if (xVertical >= 0 && xVertical < firstXIntercept) {
                        firstXIntercept = xVertical;
                    }
                }
                else {
                    // Regular line equation
                    float m = (p2.Y - p1.Y) / dx;
                    float b = p1.Y - m * p1.X;

                    // Calculate x-intercept (y=0) for non-horizontal lines
                    if (m != 0) {
                        float x0 = -b / m;
                        if (x0 >= 0 && x0 < firstXIntercept) {
                            firstXIntercept = x0;
                        }
                    }
                }
            }

            // Determine x range bounds
            float startX = max(0.0f, worldLeft);
            float endX = (firstXIntercept != INFINITY) ?
                min(worldRight, firstXIntercept) :
                worldRight;

            if (startX < endX) {
                float step = 1.0f / scroll.scale;
                std::vector<PointF> upperPoints;

                for (float x = startX; x <= endX; x += step) {
                    float minY = INFINITY;
                    bool hasValidLine = false;

                    for (int i = 0; i < 3; i++) {
                        const PointF& p1 = scroll.points[i * 2];
                        const PointF& p2 = scroll.points[i * 2 + 1];
                        float dx = p2.X - p1.X;
                        if (fabs(dx) < 1e-6f) continue;

                        float m = (p2.Y - p1.Y) / dx;
                        float b = p1.Y - m * p1.X;
                        float y_line = m * x + b;

                        if (y_line > 0.0f && y_line < minY) {
                            minY = y_line;
                            hasValidLine = true;
                        }
                    }

                    float upperY = hasValidLine ? min(minY, worldBottom) : 0.0f;
                    upperPoints.emplace_back(x, upperY);
                }

                if (!upperPoints.empty()) {
                    // Build path from points
                    path.AddLine(
                        WorldToScreenX(startX, scroll.offsetX, scroll.scale),
                        WorldToScreenY(0.0f, scroll.offsetY, scroll.scale),
                        WorldToScreenX(upperPoints.front().X, scroll.offsetX, scroll.scale),
                        WorldToScreenY(upperPoints.front().Y, scroll.offsetY, scroll.scale)
                    );

                    for (size_t i = 1; i < upperPoints.size(); i++) {
                        path.AddLine(
                            WorldToScreenX(upperPoints[i - 1].X, scroll.offsetX, scroll.scale),
                            WorldToScreenY(upperPoints[i - 1].Y, scroll.offsetY, scroll.scale),
                            WorldToScreenX(upperPoints[i].X, scroll.offsetX, scroll.scale),
                            WorldToScreenY(upperPoints[i].Y, scroll.offsetY, scroll.scale)
                        );
                    }

                    // Close path to x-axis at the end point
                    path.AddLine(
                        WorldToScreenX(upperPoints.back().X, scroll.offsetX, scroll.scale),
                        WorldToScreenY(upperPoints.back().Y, scroll.offsetY, scroll.scale),
                        WorldToScreenX(endX, scroll.offsetX, scroll.scale),
                        WorldToScreenY(0.0f, scroll.offsetY, scroll.scale)
                    );
                    path.CloseFigure();
                }
            }
            graphics.FillPath(&hatchBrush, &path);
        }

        // Draw lines
        if (scroll.points.size() == 6) {
            Color lineColors[] = {
                Color(255, 0, 120, 215),
                Color(255, 255, 0, 0),
                Color(255, 0, 150, 0)
            };

            for (int i = 0; i < 3; i++) {
                const auto& p1 = scroll.points[i * 2];
                const auto& p2 = scroll.points[i * 2 + 1];
                DrawInfiniteLine(graphics, p1, p2, scroll.offsetX, scroll.offsetY,
                    scroll.scale, plotArea, lineColors[i % 3]);
            }
        }

        BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, hdcMem, 0, 0, SRCCOPY);

        DeleteObject(hbmMem);
        DeleteDC(hdcMem);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_DESTROY:
        GdiplusShutdown(gdiplusToken);
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"PointPlotterClass";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(0, CLASS_NAME, L"Point Plotter with Lines",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}