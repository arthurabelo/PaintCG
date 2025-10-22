#include "rasterizer.h"
#include <cmath>
#include <GL/glut.h>
#include <algorithm>
#include <cassert>

static Color *g_frame = nullptr;
static int g_w = 0, g_h = 0;

void setFrameBufferPointer(Color* ptr, int w, int h){
    g_frame = ptr; g_w = w; g_h = h;
}

static inline bool inside(int x,int y){
    return x>=0 && y>=0 && x<g_w && y<g_h;
}

void putPixelToBuffer(int x,int y,const Color &c){
    if(!g_frame) return;
    if(!inside(x,y)) return;
    g_frame[y * g_w + x] = c;
}

void putPixelGL(int x,int y,const Color &c){
    // Assuma sistema coordenadas com (0,0) na janela em canto inferior esquerdo.
    glColor4ub(c.r,c.g,c.b,c.a);
    glBegin(GL_POINTS);
    glVertex2i(x,y);
    glEnd();
}

void putPixel(int x, int y, const Color &c, bool glDraw){
    putPixelToBuffer(x,y,c);
    if(glDraw){
        putPixelGL(x,y,c);
    }
}

// drawLineBresenham com técnica de redução (steep, swap endpoints)
void drawLineBresenham(int x0, int y0, int x1, int y1, const Color &c){
    bool steep = std::abs(y1 - y0) > std::abs(x1 - x0);
    if(steep){
        std::swap(x0,y0);
        std::swap(x1,y1);
    }
    if(x0 > x1){
        std::swap(x0,x1);
        std::swap(y0,y1);
    }
    int dx = x1 - x0;
    int dy = std::abs(y1 - y0);
    int error = dx / 2;
    int ystep = (y0 < y1) ? 1 : -1;
    int y = y0;
    for(int x = x0; x <= x1; ++x){
        if(steep)
            putPixel(y, x, c, true); // inversão de coordenadas
        else
            putPixel(x, y, c, true);
        error -= dy;
        if(error < 0){
            y += ystep;
            error += dx;
        }
    }
}

// midpoint circle (Bresenham-like)
void drawCircleBresenham(int xc, int yc, int r, const Color &c){
    int x = 0, y = r;
    int d = 3 - 2 * r;
    auto plot8 = [&](int xx, int yy){
        putPixel(xc + xx, yc + yy, c, true);
        putPixel(xc - xx, yc + yy, c, true);
        putPixel(xc + xx, yc - yy, c, true);
        putPixel(xc - xx, yc - yy, c, true);
        putPixel(xc + yy, yc + xx, c, true);
        putPixel(xc - yy, yc + xx, c, true);
        putPixel(xc + yy, yc - xx, c, true);
        putPixel(xc - yy, yc - xx, c, true);
    };
    while(y >= x){
        plot8(x,y);
        ++x;
        if(d > 0){
            --y;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
    }
}