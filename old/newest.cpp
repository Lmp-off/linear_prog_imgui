
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
#define LEFT_GROUPBOX_WIDTH 320

// константы
const int BUTTON_AREA_WIDTH = 400;
const float INITIAL_SCALE = 50.0f;
const float ZOOM_FACTOR = 1.2f;
const int GRID_STEP = 1;

// id для определения элемента
enum ElementId {
    BUTTON_RESET = 1,
    BUTTON_ZOOMIN,
    BUTTON_ZOOMOUT,
    BUTTON_ADDLINE,
    BUTTON_REMOVELINE,
    BUTTON_SHOW,
    CONDITIONS_PANEL,
    POINTS_PANEL,

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

static int LINES = 3;

// объявление функций
void DrawInfiniteGrid(HWND hwnd, Graphics& graphics, float offsetX, float offsetY, float scale, const RECT& plotArea);
void DrawAxesWithLabels(Graphics& graphics, float offsetX, float offsetY, float scale, const RECT& plotArea);

void DrawLineHatched(HWND hwnd, Graphics& graphics, float x1, float y1, float x2, float y2, PlotState& state);
void DrawInfiniteLine(Graphics& graphics, const PointF& p1, const PointF& p2, float offsetX, float offsetY, float scale, 
        const RECT& plotArea);

// =======================================================
// скролл панели условий
// =======================================================
struct ConditionsPanelData {
    int scrollPos;
    int totalHeight;
    std::vector<HWND> childControls;  // Child windows
    std::vector<int> lineHeights;     // Height of each line
};

void UpdateScrollInfo(HWND hwnd, ConditionsPanelData* pData) {
    SCROLLINFO si = { sizeof(SCROLLINFO) };
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    RECT rc;
    GetClientRect(hwnd, &rc);
    si.nMin = 0;
    si.nMax = pData->totalHeight;
    si.nPage = rc.bottom;
    si.nPos = pData->scrollPos;
    SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
}

void UpdateTotalHeight(ConditionsPanelData* pData) {
    pData->totalHeight = 0;
    for (int i = 0; i <= LINES; ++i) { // Учитываем текущее количество линий
        if (i < pData->lineHeights.size()) {
            pData->totalHeight += pData->lineHeights[i] + 5;
        }
    }
    pData->totalHeight += 10; // Добавляем отступ

    // Скрываем лишние элементы
    for (size_t j = 0; j < pData->childControls.size(); ++j) {
        int line = (j < 5) ? 0 : ((j - 5) / 7) + 1; // Расчет принадлежности к линии
        ShowWindow(pData->childControls[j], (line <= LINES) ? SW_SHOW : SW_HIDE);
    }
}

// Modified condInit to track control heights
void condInit(HWND hPanel, ConditionsPanelData* pData) {
    int yPos = 10;
    const int lineSpacing = 5;

    // X positions for controls in a line (L=0, Edit1=20, x+=84, Edit2=104, y->max=168)
    const int xPositions[] = {10, 30, 92, 110, 172, 188, 214};
    const int widths[] = {20, 60, 20, 60, 20, 20, 60};

    // first row
    HWND hStatic1 = CreateWindow(L"STATIC", L"L=", WS_VISIBLE | WS_CHILD,
        xPositions[0], yPos, widths[0], 20, hPanel, NULL, NULL, NULL);
    
    HWND hEdit1 = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
        xPositions[1], yPos, widths[1], 20, hPanel, NULL, NULL, NULL);
    
    HWND hStatic2 = CreateWindow(L"STATIC", L"x+", WS_VISIBLE | WS_CHILD,
        xPositions[2], yPos, widths[2], 20, hPanel, NULL, NULL, NULL);
    
    HWND hEdit2 = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
        xPositions[3], yPos, widths[3], 20, hPanel, NULL, NULL, NULL);
    
    HWND hStatic3 = CreateWindow(L"STATIC", L"y -> max", WS_VISIBLE | WS_CHILD,
        xPositions[4], yPos, widths[6], 20, hPanel, NULL, NULL, NULL);
    
    // Store controls and calculate line height
    int maxHeight = 0;
    HWND lineControls[] = {hStatic1, hEdit1, hStatic2, hEdit2, hStatic3};
    
    for (HWND hCtrl : lineControls) {
        pData->childControls.push_back(hCtrl);
        RECT rc;
        GetWindowRect(hCtrl, &rc);
        maxHeight = max(maxHeight, rc.bottom - rc.top);
    }

    pData->lineHeights.push_back(maxHeight);

    // lines
    for (int lineIdx = 0; lineIdx < 10; ++lineIdx) {
        int maxHeight = 0;
        
        // Create controls for this line
        std::wstring label = std::to_wstring(lineIdx + 1) + L")";
        HWND hStatic0 = CreateWindow(L"STATIC", label.c_str(), WS_VISIBLE | WS_CHILD,
            xPositions[0], yPos, widths[0], 20, hPanel, NULL, NULL, NULL);
        
        HWND hEdit1 = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
            xPositions[1], yPos, widths[1], 20, hPanel, NULL, NULL, NULL);
        
        HWND hStatic1 = CreateWindow(L"STATIC", L"x+", WS_VISIBLE | WS_CHILD,
            xPositions[2], yPos, widths[2], 20, hPanel, NULL, NULL, NULL);
        
        HWND hEdit2 = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
            xPositions[3], yPos, widths[3], 20, hPanel, NULL, NULL, NULL);

        HWND hStatic2 = CreateWindow(L"STATIC", L"y", WS_VISIBLE | WS_CHILD,
            xPositions[4], yPos, widths[4], 20, hPanel, NULL, NULL, NULL);

        HWND hButton1 = CreateWindow(L"BUTTON", L"<=", WS_VISIBLE | WS_CHILD,
            xPositions[5], yPos, widths[5], 20, hPanel, (HMENU)(BUTTON_SIGN + lineIdx), NULL, NULL);
        
        HWND hEdit3 = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
            xPositions[6], yPos, widths[6], 20, hPanel, NULL, NULL, NULL);

        // Store controls and calculate line height
        HWND lineControls[] = {hStatic0, hEdit1, hStatic1, hEdit2, hStatic2, hButton1, hEdit3};
        for (HWND hCtrl : lineControls) {
            pData->childControls.push_back(hCtrl);
            RECT rc;
            GetWindowRect(hCtrl, &rc);
            maxHeight = max(maxHeight, rc.bottom - rc.top);
        }

        pData->lineHeights.push_back(maxHeight);
        yPos += maxHeight + lineSpacing;
    }

    pData->totalHeight = yPos;
    UpdateScrollInfo(hPanel, pData);

    UpdateTotalHeight(pData);
}



void RepositionChildren(HWND hwnd, ConditionsPanelData* pData) {
    int yPos = 10 - pData->scrollPos;
    size_t controlIdx = 0;

    int counter = 0;

    for (int lineHeight : pData->lineHeights) {
        // Process lines
        for (int i = 0; i < (counter ? 7 : 5); ++i) {
            if (controlIdx >= pData->childControls.size()) break;
            HWND hChild = pData->childControls[controlIdx];
            
            // Get original X position and width
            RECT rc;
            GetWindowRect(hChild, &rc);
            MapWindowPoints(HWND_DESKTOP, hwnd, (LPPOINT)&rc, 2);
            
            SetWindowPos(hChild, NULL, 
                rc.left,  // Keep original X position
                yPos,     // Update Y position
                rc.right - rc.left, 
                rc.bottom - rc.top,
                SWP_NOZORDER | SWP_NOACTIVATE);
            
            controlIdx++;
        }

        yPos += lineHeight + 5;
        counter++;
    }
}

LRESULT CALLBACK ConditionsPanelWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ConditionsPanelData* pData = (ConditionsPanelData*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg) {
        case WM_CREATE: {
            pData = new ConditionsPanelData();
            pData->scrollPos = 0;
            pData->totalHeight = 0;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pData);
            condInit(hwnd, pData);
            break;
        }

        case WM_DESTROY: {
            if (pData) {
                delete pData;
                SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
            }
            break;
        }

        case WM_SIZE: {
            if (pData) {
                UpdateScrollInfo(hwnd, pData);
                RepositionChildren(hwnd, pData);
            }
            break;
        }

        case WM_VSCROLL: {
            if (!pData) break;
            SCROLLINFO si = { sizeof(SCROLLINFO) };
            si.fMask = SIF_ALL;
            GetScrollInfo(hwnd, SB_VERT, &si);

            int oldPos = si.nPos;
            int newPos = oldPos;

            switch (LOWORD(wParam)) {
                case SB_LINEUP:        newPos -= 10; break;
                case SB_LINEDOWN:      newPos += 10; break;
                case SB_PAGEUP:       newPos -= si.nPage; break;
                case SB_PAGEDOWN:     newPos += si.nPage; break;
                case SB_THUMBTRACK:   newPos = si.nTrackPos; break;
                case SB_THUMBPOSITION: newPos = HIWORD(wParam); break;
                default: break;
            }

            newPos = max(si.nMin, min(newPos, si.nMax - (int)si.nPage + 1));
            
            if (newPos != oldPos) {
                pData->scrollPos = newPos;
                si.fMask = SIF_POS;
                si.nPos = newPos;
                SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
                
                // Scroll the window content
                ScrollWindow(hwnd, 0, oldPos - newPos, NULL, NULL);
                UpdateWindow(hwnd);
            }
            break;
        }

        case WM_MOUSEWHEEL: {
            if (!pData) break;
            int delta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
            SendMessage(hwnd, WM_VSCROLL, (delta > 0) ? SB_LINEUP : SB_LINEDOWN, 0);
            return 0;
        }

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// =======================================================
// скролл панели точек
// =======================================================
// Add new data structure for Points Panel
struct PointsPanelData {
    int scrollPos;
    int totalHeight;
    std::vector<HWND> childControls;
    std::vector<int> lineHeights;
};

void UpdateScrollInfo(HWND hwnd, PointsPanelData* pData) {
    SCROLLINFO si = { sizeof(SCROLLINFO) };
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    RECT rc;
    GetClientRect(hwnd, &rc);
    si.nMin = 0;
    si.nMax = pData->totalHeight;
    si.nPage = rc.bottom;
    si.nPos = pData->scrollPos;
    SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
}


void UpdateTotalHeight(PointsPanelData* pData) {
    pData->totalHeight = 0;
    for (int i = 0; i <= LINES; ++i) { // Учитываем текущее количество линий
        if (i < pData->lineHeights.size()) {
            pData->totalHeight += pData->lineHeights[i] + 5;
        }
    }
    pData->totalHeight += 10; // Добавляем отступ

    // Скрываем лишние элементы
    for (size_t j = 0; j < pData->childControls.size(); ++j) {
        int line = (j < 4) ? 0 : ((j - 4) / 5) + 1; // Расчет принадлежности к линии
        ShowWindow(pData->childControls[j], (line <= LINES) ? SW_SHOW : SW_HIDE);
    }
}

void RepositionChildren(HWND hwnd, PointsPanelData* pData) {
    int yPos = 10 - pData->scrollPos;
    size_t controlIdx = 0;

    int counter = 0;

    for (int lineHeight : pData->lineHeights) {
        // Process lines
        for (int i = 0; i < (counter ? 5 : 4); ++i) {
            if (controlIdx >= pData->childControls.size()) break;
            HWND hChild = pData->childControls[controlIdx];
            
            // Get original X position and width
            RECT rc;
            GetWindowRect(hChild, &rc);
            MapWindowPoints(HWND_DESKTOP, hwnd, (LPPOINT)&rc, 2);
            
            SetWindowPos(hChild, NULL, 
                rc.left,  // Keep original X position
                yPos,     // Update Y position
                rc.right - rc.left, 
                rc.bottom - rc.top,
                SWP_NOZORDER | SWP_NOACTIVATE);
            
            controlIdx++;
        }

        yPos += lineHeight + 5;
        counter++;
    }
}

// Implement pointsInit similar to condInit but for points
void pointsInit(HWND hPanel, PointsPanelData* pData) {
    int yPos = 40;
    const int lineSpacing = 5;

    HWND hStatic1 = CreateWindow(L"STATIC", L"x1", WS_VISIBLE | WS_CHILD,
        110, 0, 20, 20, hPanel, NULL, NULL, NULL);
    HWND hStatic2 = CreateWindow(L"STATIC", L"y1", WS_VISIBLE | WS_CHILD,
        150, 0, 20, 20, hPanel, NULL, NULL, NULL);
    HWND hStatic3 = CreateWindow(L"STATIC", L"x2", WS_VISIBLE | WS_CHILD,
        190, 0, 20, 20, hPanel, NULL, NULL, NULL);
    HWND hStatic4 = CreateWindow(L"STATIC", L"y2", WS_VISIBLE | WS_CHILD,
        230, 0, 20, 20, hPanel, NULL, NULL, NULL);
    HWND lineControls[] = {hStatic1, hStatic2, hStatic3, hStatic4};
    int maxHeight = 0;
    for (HWND hCtrl : lineControls) {
        pData->childControls.push_back(hCtrl);
        RECT rc;
        GetWindowRect(hCtrl, &rc);
        maxHeight = max(maxHeight, rc.bottom - rc.top);
    }
    pData->lineHeights.push_back(maxHeight);

    const int xPositions[] = {10, 100, 140, 180, 220};
    const int widths[] = {80, 40, 40, 40, 40};

    for (int lineIdx = 0; lineIdx < 10; ++lineIdx) {
        std::wstring label = L"Прямая " + std::to_wstring(lineIdx+1) + L":";
        HWND hStatic = CreateWindow(L"STATIC", label.c_str(), WS_VISIBLE | WS_CHILD,
            xPositions[0], yPos, widths[0], 20, hPanel, NULL, NULL, NULL);

        HWND hEditX1 = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
            xPositions[1], yPos, widths[1], 20, hPanel, NULL, NULL, NULL);
        
        HWND hEditY1 = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
            xPositions[2], yPos, widths[2], 20, hPanel, NULL, NULL, NULL);

        HWND hEditX2 = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
            xPositions[3], yPos, widths[3], 20, hPanel, NULL, NULL, NULL);
        
        HWND hEditY2 = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
            xPositions[4], yPos, widths[4], 20, hPanel, NULL, NULL, NULL);

        // Store controls
        HWND lineControls[] = {hStatic, hEditX1, hEditY1, hEditX2, hEditY2};
        int maxHeight = 0;
        for (HWND hCtrl : lineControls) {
            pData->childControls.push_back(hCtrl);
            RECT rc;
            GetWindowRect(hCtrl, &rc);
            maxHeight = max(maxHeight, rc.bottom - rc.top);
        }
        pData->lineHeights.push_back(maxHeight);
        yPos += maxHeight + lineSpacing;
    }
    pData->totalHeight = yPos;

    UpdateTotalHeight(pData);
}

// Add new window procedure for Points Panel
LRESULT CALLBACK PointsPanelWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    PointsPanelData* pData = (PointsPanelData*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg) {
        case WM_CREATE: {
            pData = new PointsPanelData();
            pData->scrollPos = 0;
            pData->totalHeight = 0;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pData);
            pointsInit(hwnd, pData); // Implement this function
            break;
        }
        case WM_DESTROY: {
            if (pData) {
                delete pData;
                SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
            }
            break;
        }
        case WM_SIZE: {
            if (pData) {
                UpdateScrollInfo(hwnd, pData);
                RepositionChildren(hwnd, pData);
            }
            break;
        }

        case WM_VSCROLL: {
            if (!pData) break;
            SCROLLINFO si = { sizeof(SCROLLINFO) };
            si.fMask = SIF_ALL;
            GetScrollInfo(hwnd, SB_VERT, &si);

            int oldPos = si.nPos;
            int newPos = oldPos;

            switch (LOWORD(wParam)) {
                case SB_LINEUP:        newPos -= 10; break;
                case SB_LINEDOWN:      newPos += 10; break;
                case SB_PAGEUP:       newPos -= si.nPage; break;
                case SB_PAGEDOWN:     newPos += si.nPage; break;
                case SB_THUMBTRACK:   newPos = si.nTrackPos; break;
                case SB_THUMBPOSITION: newPos = HIWORD(wParam); break;
                default: break;
            }

            newPos = max(si.nMin, min(newPos, si.nMax - (int)si.nPage + 1));
            
            if (newPos != oldPos) {
                pData->scrollPos = newPos;
                si.fMask = SIF_POS;
                si.nPos = newPos;
                SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
                
                // Scroll the window content
                ScrollWindow(hwnd, 0, oldPos - newPos, NULL, NULL);
                UpdateWindow(hwnd);
            }
            break;
        }

        case WM_MOUSEWHEEL: {
            if (!pData) break;
            int delta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
            SendMessage(hwnd, WM_VSCROLL, (delta > 0) ? SB_LINEUP : SB_LINEDOWN, 0);
            return 0;
        }
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}



// =======================================================
// инициализация окна
// =======================================================
bool init(HWND hwnd) {
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    // hBackgroundBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
    hBackgroundBrush = CreateSolidBrush(RGB(212, 208, 200));

    // класс панели
    // ================================
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = ConditionsPanelWndProc;  // Custom window procedure for scrolling
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = hBackgroundBrush;
    wc.lpszClassName = L"ConditionsPanelClass";
    RegisterClassEx(&wc);

    WNDCLASSEX wcPoints = {};
    wcPoints.cbSize = sizeof(WNDCLASSEX);
    wcPoints.style = CS_HREDRAW | CS_VREDRAW;
    wcPoints.lpfnWndProc = PointsPanelWndProc;
    wcPoints.hInstance = GetModuleHandle(NULL);
    wcPoints.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcPoints.hbrBackground = hBackgroundBrush;
    wcPoints.lpszClassName = L"PointsPanelClass";
    RegisterClassEx(&wcPoints);

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

    // кнопки управления прямыми
    CreateWindow(L"BUTTON", L"Добавить", WS_VISIBLE | WS_CHILD,
        170, 20, 70, 24,
        hwnd, (HMENU)BUTTON_ADDLINE, NULL, NULL);
    CreateWindow(L"BUTTON", L"Удалить", WS_VISIBLE | WS_CHILD,
        250, 20, 70, 24,
        hwnd, (HMENU)BUTTON_REMOVELINE, NULL, NULL);

    // условие
    CreateWindow(L"STATIC", L"Условие", WS_VISIBLE | WS_CHILD,
        40, 20, 60, 20, hwnd, NULL, NULL, NULL);

    // панель уравнений
    HWND hConditionsPanel = CreateWindow(
        L"ConditionsPanelClass",
        NULL,
        WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_BORDER,
        20, 50,  // Position under "Условие"
        LEFT_GROUPBOX_WIDTH - 20, 120,  // Fixed width and height
        hwnd,
        (HMENU)CONDITIONS_PANEL,
        GetModuleHandle(NULL),
        NULL
    );

    // точки
    CreateWindow(L"STATIC", L"Прямые", WS_VISIBLE | WS_CHILD,
        40, 170, 60, 20, hwnd, NULL, NULL, NULL);

    // папнель точек
    HWND hPointsPanel = CreateWindow(
        L"PointsPanelClass",
        NULL,
        WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_BORDER,
        20, 190,
        LEFT_GROUPBOX_WIDTH - 20, 120,
        hwnd,
        (HMENU)POINTS_PANEL,
        GetModuleHandle(NULL),
        NULL
    );

    // кнопка построения
    CreateWindow(L"BUTTON", L"Построить прямые", WS_VISIBLE | WS_CHILD,
        20, 320, 200, 30,
        hwnd, (HMENU)BUTTON_SHOW, NULL, NULL);

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

        // Reset clip region to draw lines anywhere in the plot area
        // graphics.ResetClip();

        // DrawLineHatched(hwnd, graphics, 5.0f, 5.0f, 10.0f, 10.0f, state);
        DrawInfiniteLine(graphics, { 1., 1. }, { 0., 1. }, state.offsetX, state.offsetY, state.scale, plotArea);
        // DrawLineHatched(hwnd, graphics, 1,1, 0, 1, state);
        // DrawLineHatched(hwnd, graphics, 1,1, 1, 0, state);


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
        case BUTTON_RESET:
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
        case BUTTON_ADDLINE:
            if (LINES < 10) {
                LINES++;
                // Обновляем обе панели
                HWND hConditionsPanel = GetDlgItem(hwnd, CONDITIONS_PANEL);
                ConditionsPanelData* pCondData = (ConditionsPanelData*)GetWindowLongPtr(hConditionsPanel, GWLP_USERDATA);
                if (pCondData) {
                    UpdateTotalHeight(pCondData);
                    UpdateScrollInfo(hConditionsPanel, pCondData);
                    RepositionChildren(hConditionsPanel, pCondData);
                }

                HWND hPointsPanel = GetDlgItem(hwnd, POINTS_PANEL);
                PointsPanelData* pPointsData = (PointsPanelData*)GetWindowLongPtr(hPointsPanel, GWLP_USERDATA);
                if (pPointsData) {
                    UpdateTotalHeight(pPointsData);
                    UpdateScrollInfo(hPointsPanel, pPointsData);
                    RepositionChildren(hPointsPanel, pPointsData);
                }
                InvalidateRect(hwnd, NULL, TRUE);
            }
            break;

        case BUTTON_REMOVELINE:
            if (LINES > 3) {
                LINES--;
                // Обновляем панель условий
                HWND hConditionsPanel = GetDlgItem(hwnd, CONDITIONS_PANEL);
                ConditionsPanelData* pCondData = (ConditionsPanelData*)GetWindowLongPtr(hConditionsPanel, GWLP_USERDATA);
                if (pCondData) {
                    UpdateTotalHeight(pCondData);
                    UpdateScrollInfo(hConditionsPanel, pCondData);
                    RepositionChildren(hConditionsPanel, pCondData);
                }

                // Скрываем лишние элементы в панели точек
                HWND hPointsPanel = GetDlgItem(hwnd, POINTS_PANEL);
                PointsPanelData* pPointsData = (PointsPanelData*)GetWindowLongPtr(hPointsPanel, GWLP_USERDATA);
                if (pPointsData) {
                    UpdateTotalHeight(pPointsData);
                    UpdateScrollInfo(hPointsPanel, pPointsData);
                    RepositionChildren(hPointsPanel, pPointsData);
                }
                InvalidateRect(hwnd, NULL, TRUE);
            }
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
// отрисовка сетки
// =======================================================
void DrawInfiniteGrid(HWND hwnd, Graphics& graphics, float offsetX, float offsetY, float scale, const RECT& plotArea) {
    RECT rc;
    GetClientRect(hwnd, &rc);

    // фон
    SolidBrush white(Color(255, 255, 255));
    graphics.FillRectangle(&white, 0, 0, rc.right, rc.bottom);

    Pen gridPen(Color(150, 200, 200, 200));
    SolidBrush textBrush(Color(120, 120, 120));
    Font font(L"Arial", 10);

    float left = ScreenToWorldX(plotArea.left, offsetX, scale);
    float right = ScreenToWorldX(plotArea.right, offsetX, scale);
    float top = ScreenToWorldY(plotArea.top, offsetY, scale);
    float bottom = ScreenToWorldY(plotArea.bottom, offsetY, scale);

    // вертикальные линии
    float startX = floor(left / GRID_STEP) * GRID_STEP;
    float endX = ceil(right / GRID_STEP) * GRID_STEP;
    for (float x = startX; x <= endX; x += GRID_STEP) {
        int screenX = BUTTON_AREA_WIDTH + static_cast<int>(x * scale + offsetX);
        graphics.DrawLine(&gridPen, screenX, plotArea.top, screenX, plotArea.bottom-35);
    }

    // горизонтальные линии
    float startY = floor(top / GRID_STEP) * GRID_STEP;
    float endY = ceil(bottom / GRID_STEP) * GRID_STEP;
    for (float y = startY; y <= endY-1; y += GRID_STEP) {
        int screenY = static_cast<int>(y * scale + offsetY);
        graphics.DrawLine(&gridPen, BUTTON_AREA_WIDTH+25, screenY, plotArea.right, screenY);
    }
}

// =======================================================
// линия со штриховкой внутри графика
// =======================================================
void DrawLineHatched(HWND hwnd, Graphics& graphics, float x1, float y1, float x2, float y2, PlotState& state) {
    // 1. Use static pen to avoid GDI+ object creation/destruction every call
    static Pen linePen(Color(255, 0, 0, 0), 2);
    static bool firstRun = true;
    if (firstRun) {
        linePen.SetDashStyle(DashStyleDash);
        firstRun = false;
    }

    // 2. Get valid plot area from main window
    RECT plotArea;
    GetClientRect(GetParent(hwnd), &plotArea);
    plotArea.left = BUTTON_AREA_WIDTH + 10;  // Match WM_PAINT's drawing area
    plotArea.top += 60;
    plotArea.right -= 10;
    plotArea.bottom -= 70;

    // 3. Use your proven infinite line algorithm
    PointF p1(x1, y1), p2(x2, y2);
    float dx = p2.X - p1.X;
    
    if (fabs(dx) < 1e-6) { // Vertical line
        int screenX = WorldToScreenX(p1.X, state.offsetX, state.scale);
        if (screenX < plotArea.left || screenX > plotArea.right) return;
        graphics.DrawLine(&linePen, screenX, plotArea.top, screenX, plotArea.bottom);
    } else {
        float m = (p2.Y - p1.Y) / dx;
        float b = p1.Y - m * p1.X;

        // Calculate visible bounds in WORLD coordinates
        float worldLeft = ScreenToWorldX(plotArea.left, state.offsetX, state.scale);
        float worldRight = ScreenToWorldX(plotArea.right, state.offsetX, state.scale);

        // Calculate line endpoints at plot edges
        PointF start(worldLeft, m * worldLeft + b);
        PointF end(worldRight, m * worldRight + b);

        // Convert to screen coordinates
        int x1 = WorldToScreenX(start.X, state.offsetX, state.scale);
        int y1 = WorldToScreenY(start.Y, state.offsetY, state.scale);
        int x2 = WorldToScreenX(end.X, state.offsetX, state.scale);
        int y2 = WorldToScreenY(end.Y, state.offsetY, state.scale);

        // 4. Clip before drawing
        if (x1 > plotArea.right && x2 > plotArea.right) return;
        if (x1 < plotArea.left && x2 < plotArea.left) return;
        
        graphics.DrawLine(&linePen, x1, y1, x2, y2);
    }
}

void DrawInfiniteLine(Graphics& graphics, const PointF& p1, const PointF& p2, float offsetX, float offsetY, float scale, const RECT& plotArea) {
    if (p1.X == p2.X && p1.Y == p2.Y) return;

    Pen linePen(Color(255, 0, 0, 0), 1);


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
// =======================================================
// отрисовка осей
// =======================================================
void DrawAxesWithLabels(Graphics& graphics, float offsetX, float offsetY, float scale, const RECT& plotArea) {
    Pen axisPen(Color(255, 0, 0, 0), 2);
    SolidBrush textBrush(Color(0, 0, 0));
    Font font(L"Arial", 10);

    // Define margins (in pixels) for labels
    const int LEFT_MARGIN = 25;    // Space for Y-axis labels
    const int BOTTOM_MARGIN = 35;  // Space for X-axis labels
    const int LABEL_OFFSET = 5;    // Additional spacing between labels and edges

    // Calculate adjusted plot area (smaller to account for margins)
    RECT adjustedPlotArea = plotArea;
    adjustedPlotArea.left += LEFT_MARGIN;
    adjustedPlotArea.bottom -= BOTTOM_MARGIN;

    // X-axis labels (positioned above bottom margin)
    const int X_LABEL_Y = plotArea.bottom - BOTTOM_MARGIN + LABEL_OFFSET;
    float worldLeftX = ScreenToWorldX(adjustedPlotArea.left, offsetX, scale);
    float worldRightX = ScreenToWorldX(adjustedPlotArea.right, offsetX, scale);
    
    float xStart = floor(worldLeftX / GRID_STEP) * GRID_STEP;
    float xEnd = ceil(worldRightX / GRID_STEP) * GRID_STEP;
    
    for (float x = xStart; x <= xEnd; x += GRID_STEP) {
        int screenX = WorldToScreenX(x, offsetX, scale);
        if (screenX < adjustedPlotArea.left || screenX > adjustedPlotArea.right) continue;
        
        std::wstring text = std::to_wstring(static_cast<int>(x));
        // Center-align the text under the grid line
        graphics.DrawString(text.c_str(), -1, &font,
            PointF(screenX - 10, X_LABEL_Y), &textBrush);
    }

    // Y-axis labels (positioned right of left margin)
    const int Y_LABEL_X = plotArea.left + LEFT_MARGIN - LABEL_OFFSET - 15;
    float worldTopY = ScreenToWorldY(adjustedPlotArea.top, offsetY, scale);
    float worldBottomY = ScreenToWorldY(adjustedPlotArea.bottom, offsetY, scale);
    
    float yStart = floor(worldTopY / GRID_STEP) * GRID_STEP;
    float yEnd = ceil(worldBottomY / GRID_STEP) * GRID_STEP;
    
    for (float y = yStart; y <= yEnd; y += GRID_STEP) {
        int screenY = WorldToScreenY(y, offsetY, scale);
        if (screenY < adjustedPlotArea.top || screenY > adjustedPlotArea.bottom) continue;
        
        std::wstring text = std::to_wstring(static_cast<int>(-y));
        // Vertically center-align the text
        graphics.DrawString(text.c_str(), -1, &font,
            PointF(Y_LABEL_X, screenY - 8), &textBrush);
    }

    // Draw axis lines within adjusted plot area
    int screenY0 = WorldToScreenY(0, offsetY, scale);
    if (screenY0 >= adjustedPlotArea.top && screenY0 <= adjustedPlotArea.bottom) {
        graphics.DrawLine(&axisPen, 
            adjustedPlotArea.left, screenY0,
            adjustedPlotArea.right, screenY0);
    }

    int screenX0 = WorldToScreenX(0, offsetX, scale);
    if (screenX0 >= adjustedPlotArea.left && screenX0 <= adjustedPlotArea.right) {
        graphics.DrawLine(&axisPen,
            screenX0, adjustedPlotArea.top,
            screenX0, adjustedPlotArea.bottom);
    }
}