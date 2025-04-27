#pragma once
#include <windows.h>
#include <gdiplus.h>

#pragma comment (lib, "Gdiplus.lib")
using namespace Gdiplus;

void InitializeGraphics();
void ShutdownGraphics();
void DrawScene(HWND hwnd, HDC hdc);