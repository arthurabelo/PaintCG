#include "fill.h"
#include <cmath>
#include <GL/gl.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <algorithm>
#include <stack>
#include <cassert>

// Scanline fill (par-ímpar). polygon: lista de (x,y) inteiros
void fillPolygonScanline(const std::vector<std::pair<int,int>>& polygon, const Color& c){
    if(polygon.size() < 3) return;
    int miny = polygon[0].second, maxy = polygon[0].second;
    for(auto &p: polygon){ miny = std::min(miny, p.second); maxy = std::max(maxy, p.second); }
    for(int y = miny; y <= maxy; ++y){
        std::vector<int> intersections;
        for(size_t i=0;i<polygon.size();++i){
            auto p1 = polygon[i];
            auto p2 = polygon[(i+1)%polygon.size()];
            if(p1.second == p2.second) continue; // horizontal edge ignore (or handle per regra)
            int ymin = std::min(p1.second,p2.second);
            int ymax = std::max(p1.second,p2.second);
            if(y < ymin || y >= ymax) continue; // regra de preenchimento consistente
            // calcula interseção x
            float x = p1.first + (float)(y - p1.second) * (p2.first - p1.first) / (float)(p2.second - p1.second);
            intersections.push_back((int)std::floor(x));
        }
        std::sort(intersections.begin(), intersections.end());
        for(size_t k=0;k+1<intersections.size();k+=2){
            int xstart = intersections[k];
            int xend = intersections[k+1];
            for(int x=xstart; x<=xend; ++x){
                putPixel(x,y,c,true);
            }
        }
    }
}

// flood-fill 4-neighborhood iterative
extern Color *g_frame; // não é exportado aqui, mas assumimos setFrameBufferPointer foi chamado
int g_w = 0;
int g_h = 0;

void floodFill4(int x, int y, const Color& newColor){
    // Lê cor original via pixel buffer - assumem setFrameBufferPointer foi chamado no rasterizer
    // Para não depender de membros estáticos privados, faremos uma leitura via putPixelToBuffer? 
    // Aqui usamos glReadPixels lentamente como fallback caso framebuffer não exista.
    // Implementação assume o mesmo framebuffer interno do rasterizer foi configurado.

    // Pseudo-acesso: acessaremos via função auxiliar (não exportada); porém para manter simples,
    // vamos usar glReadPixels para ler cor inicial (menos eficiente, mais simples).
    Color orig;
    unsigned char pixel[4];
    glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    orig = Color(pixel[0], pixel[1], pixel[2], pixel[3]);
    if(orig == newColor) return;

    std::stack<std::pair<int,int>> st;
    st.push({x,y});
    while(!st.empty()){
        auto p = st.top(); st.pop();
        int px = p.first, py = p.second;
        // ler cor atual
        unsigned char buf[4];
        glReadPixels(px, py, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, buf);
        Color cur(buf[0], buf[1], buf[2], buf[3]);
        if(!(cur == orig)) continue;
        putPixel(px, py, newColor, true);
        if(px+1 < g_w) st.push({px+1, py});
        if(px-1 >= 0) st.push({px-1, py});
        if(py+1 < g_h) st.push({px, py+1});
        if(py-1 >= 0) st.push({px, py-1});
    }
}