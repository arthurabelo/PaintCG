#pragma once
#include <vector>
#include <cstdint>

struct Color {
    uint8_t r,g,b,a;
    Color(uint8_t r_=0,uint8_t g_=0,uint8_t b_=0,uint8_t a_=255):r(r_),g(g_),b(b_),a(a_){}
    bool operator==(const Color& o) const { return r==o.r && g==o.g && b==o.b && a==o.a; }
};

void setFrameBufferPointer(Color* ptr, int w, int h); // apontar framebuffer externo
void putPixel(int x, int y, const Color &c, bool glDraw=true);

// Bresenham - redução ao primeiro octante (usa glBegin(GL_POINTS) internamente quando glDraw=true)
void drawLineBresenham(int x0, int y0, int x1, int y1, const Color &c);

// Bresenham circle (midpoint / Bresenham)
void drawCircleBresenham(int xc, int yc, int radius, const Color &c);