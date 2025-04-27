#pragma once
#include <Windows.h>
#include "app_state.h"

void createButton(HWND hwnd, int buttonId, int x, int y, int width, int height, const wchar_t* text);

void CreateMainControls(HWND hwnd);