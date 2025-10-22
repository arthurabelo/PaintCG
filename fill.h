#pragma once
#include "rasterizer.h"
#include <vector>

// preencher polígono via scanline (polígono simples, coordenadas inteiras)
void fillPolygonScanline(const std::vector<std::pair<int,int>>& polygon, const Color& c);

// flood fill 4-vizinhança iterativo (usa framebuffer interno setado via setFrameBufferPointer)
void floodFill4(int x, int y, const Color& newColor);