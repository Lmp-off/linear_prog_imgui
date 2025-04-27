#pragma once
#include <Windows.h>
#include "app_state.h"

HWND createLabel(HWND hwnd, int x, int y, int width, int height, const wchar_t* text);
HWND createEditBox(HWND hwnd, int x, int y, int width, int height, const wchar_t* text);
HWND createButton(HWND hwnd, int buttonId, int x, int y, int width, int height, const wchar_t* text);

void CreateMainControls(HWND hwnd);