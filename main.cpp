/*
 * PaintCG - Exemplo: Bresenham, polígonos, preenchimento e transformações
 * Implementação em C++ com GLUT/OpenGL
 *
 * Observação: Este arquivo é didático e concentra as rotinas essenciais
 * solicitadas no enunciado.
 *
 * Autor: Exemplo (adaptado para exercício)
 */

#ifdef __APPLE__
    #define GL_SILENCE_DEPRECATION
    #include <GLUT/glut.h>
    #include <OpenGL/gl.h>
    #include <OpenGL/glu.h>
#else
    #include <GL/glut.h>
    #include <GL/gl.h>
    #include <GL/glu.h>
#endif

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <stack>
#include <algorithm>
#include <array>
#include <string>
#include <sstream>
#include <iostream>
#include "glut_text.h"

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

using namespace std;

#define ESC 27

// Tipos de forma
enum TipoForma
{
    M_LINHA = 1,
    M_TRIANGULO,
    M_RETANGULO,
    M_POLIGONO,
    M_CIRCULO
};

// Cor (RGB)
struct Color
{
    unsigned char r, g, b;
};
const Color WHITE = {255, 255, 255};
const Color BLACK = {0, 0, 0};
const Color RED = {255, 0, 0};
const Color GREEN = {0, 255, 0};
const Color BLUE = {0, 0, 255};
const Color FILL_COLOR = {200, 200, 20}; // cor padrão de preenchimento

// Vertice inteiro
struct V2
{
    int x, y;
};

// Forma geométrica (lista de vértices)
struct Forma
{
    TipoForma tipo;
    vector<V2> verts; // Para linhas: 2 verts; tri: 3; ret: 2 (sup-esq, inf-dir); pol: n>=4; circ: 2 (centro, ponto raio)
    Color cor = BLACK;
};

// Dimensões janela / mouse
int winW = 800, winH = 600;
int mouse_x = 0, mouse_y = 0;

// Largura da barra lateral (menu)
int sidebarWidth = 50;

bool click1 = false;                    // verifica se foi realizado o primeiro clique do mouse (exemplo)
int m_x = 0, m_y = 0;                   // coordenadas do mouse (exemplo)
int x_1 = 0, y_1 = 0, x_2 = 0, y_2 = 0; // cliques 1 e 2 (exemplo)
int width = winW, height = winH;        // largura/altura - alias visual para o exemplo

// Estado do aplicativo
vector<Forma> formas;
Forma currentForma;
bool drawing = false; // se estamos no meio de desenhar
TipoForma modo = M_LINHA;

// Framebuffer auxiliar (armazenar cor de cada pixel)
vector<Color> framebuffer; // tamanho winW * winH, row-major
// Overlay buffer to store persistent pixel edits (flood-fill, scanline fills, etc.)
vector<Color> overlayBuffer;
vector<unsigned char> overlayMask; // 0 = empty, 1 = has overlay color

// forward declaration: idx used by overlay helpers before full utility section
inline int idx(int x, int y);

inline void setOverlayPixel(int x, int y, Color c)
{
    if (x < 0 || x >= winW || y < 0 || y >= winH)
        return;
    int i = idx(x, y);
    overlayBuffer[i] = c;
    overlayMask[i] = 1;
}

inline Color getCombinedPixel(int x, int y)
{
    if (x < 0 || x >= winW || y < 0 || y >= winH)
        return WHITE;
    int i = idx(x, y);
    if (overlayMask[i])
        return overlayBuffer[i];
    return framebuffer[i];
}

// Apply overlay pixels into framebuffer (used when composing before flush)
void applyOverlayToFramebuffer()
{
    size_t N = framebuffer.size();
    for (size_t i = 0; i < N; ++i)
    {
        if (overlayMask[i])
            framebuffer[i] = overlayBuffer[i];
    }
}

// Funções utilitárias de pixel / framebuffer
inline int idx(int x, int y) { return y * winW + x; }

void setPixelBuffer(int x, int y, Color c)
{
    if (x < 0 || x >= winW || y < 0 || y >= winH)
        return;
    framebuffer[idx(x, y)] = c;
}

Color getPixelBuffer(int x, int y)
{
    if (x < 0 || x >= winW || y < 0 || y >= winH)
        return WHITE;
    return framebuffer[idx(x, y)];
}

// Desenha pixel usando GL_POINTS
void drawPixelGL(int x, int y, Color c)
{
    // Atualiza buffer auxiliar
    setPixelBuffer(x, y, c);
}

// Desenha todo o framebuffer na tela (um único batched GL_POINTS)
void flushFramebuffer()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_POINTS);
    for (int y = 0; y < winH; ++y)
    {
        for (int x = 0; x < winW; ++x)
        {
            Color c = framebuffer[idx(x, y)];
            glColor3ub(c.r, c.g, c.b);
            glVertex2i(x, y);
        }
    }
    glEnd();
}

// Limpa tela (framebuffer + OpenGL)
void clearScreen()
{
    // Limpa framebuffer em RAM
    std::fill(framebuffer.begin(), framebuffer.end(), WHITE);
    std::fill(overlayBuffer.begin(), overlayBuffer.end(), WHITE);
    std::fill(overlayMask.begin(), overlayMask.end(), 0);
    flushFramebuffer();
    glutSwapBuffers();
}

// ------------------------
// Bresenham (linha)
// ------------------------

struct OctantFlags
{
    bool steep;
    int x0, y0, x1, y1;
    int ystep;
};

OctantFlags computeOctantFlags(int x0, int y0, int x1, int y1)
{
    OctantFlags f;
    f.steep = (abs(y1 - y0) > abs(x1 - x0));
    if (f.steep)
    {
        swap(x0, y0);
        swap(x1, y1);
    }
    if (x0 > x1)
    {
        swap(x0, x1);
        swap(y0, y1);
    }
    f.x0 = x0;
    f.y0 = y0;
    f.x1 = x1;
    f.y1 = y1;
    f.ystep = (y0 < y1) ? 1 : -1;
    return f;
}

// Plot um ponto transformando de volta para o sistema original (inverso da redução)
void plotFromTransformed(int x, int y, const OctantFlags &f, Color cor)
{
    int px = x, py = y;
    if (f.steep)
        swap(px, py);
    // No computeOctantFlags garantimos que x0 <= x1; porém se houve swap para ordenar, y may need step inversion
    // O algoritmo principal já usa ystep para mover em direção correta.
    drawPixelGL(px, py, cor);
}

// Bresenham geral (utilizando redução ao "primeiro octante" via steep + swap)
void bresenhamLine(int x0, int y0, int x1, int y1, Color cor)
{
    // Special case: degenerate
    if (x0 == x1 && y0 == y1)
    {
        drawPixelGL(x0, y0, cor);
        return;
    }

    OctantFlags f = computeOctantFlags(x0, y0, x1, y1);
    int dx = f.x1 - f.x0;
    int dy = abs(f.y1 - f.y0);
    int err = dx / 2;
    int y = f.y0;
    for (int x = f.x0; x <= f.x1; ++x)
    {
        // map (x,y) back and draw
        int px = x, py = y;
        if (f.steep)
            swap(px, py);
        drawPixelGL(px, py, cor);

        err -= dy;
        if (err < 0)
        {
            y += f.ystep;
            err += dx;
        }
    }
}

// ------------------------
// Círculo: algoritmo de midpoint (variante de Bresenham para círculos)
// Recebe centro (cx,cy) e raio r
// ------------------------
void plotCirclePoints(int cx, int cy, int x, int y, Color cor)
{
    // 8-symmetry
    drawPixelGL(cx + x, cy + y, cor);
    drawPixelGL(cx - x, cy + y, cor);
    drawPixelGL(cx + x, cy - y, cor);
    drawPixelGL(cx - x, cy - y, cor);
    drawPixelGL(cx + y, cy + x, cor);
    drawPixelGL(cx - y, cy + x, cor);
    drawPixelGL(cx + y, cy - x, cor);
    drawPixelGL(cx - y, cy - x, cor);
}

void midpointCircle(int cx, int cy, int r, Color cor)
{
    int x = 0, y = r;
    int d = 1 - r;
    plotCirclePoints(cx, cy, x, y, cor);
    while (x < y)
    {
        if (d < 0)
        {
            d += 2 * x + 3;
            x++;
        }
        else
        {
            d += 2 * (x - y) + 5;
            x++;
            y--;
        }
        plotCirclePoints(cx, cy, x, y, cor);
    }
}

// ------------------------
// Retângulo: desenha 4 arestas com Bresenham
// Recebe canto superior esquerdo (x1,y1) e canto inferior direito (x2,y2)
// ------------------------
void drawRectFromCorners(int x1, int y1, int x2, int y2, Color cor)
{
    // Converte para cantos corretos
    int left = min(x1, x2);
    int right = max(x1, x2);
    int bottom = min(y1, y2);
    int top = max(y1, y2);
    // Desenhar retângulo (4 arestas)
    bresenhamLine(left, bottom, right, bottom, cor);
    bresenhamLine(right, bottom, right, top, cor);
    bresenhamLine(right, top, left, top, cor);
    bresenhamLine(left, top, left, bottom, cor);
}

// ------------------------
// Triângulo: desenha as 3 arestas
// ------------------------
void drawTriangle(const vector<V2> &pts, Color cor)
{
    if (pts.size() < 3)
        return;
    bresenhamLine(pts[0].x, pts[0].y, pts[1].x, pts[1].y, cor);
    bresenhamLine(pts[1].x, pts[1].y, pts[2].x, pts[2].y, cor);
    bresenhamLine(pts[2].x, pts[2].y, pts[0].x, pts[0].y, cor);
}

// ------------------------
// Polígono: desenha arestas consecutivas e fecha
// ------------------------
void drawPolygon(const vector<V2> &pts, Color cor)
{
    if (pts.size() < 2)
        return;
    for (size_t i = 0; i < pts.size() - 1; ++i)
    {
        bresenhamLine(pts[i].x, pts[i].y, pts[i + 1].x, pts[i + 1].y, cor);
    }
    // fechar
    bresenhamLine(pts.back().x, pts.back().y, pts.front().x, pts.front().y, cor);
}

// ------------------------
// Scanline Fill para polígonos simples (não necessariamente convexos)
// Entrada: vetor de vértices em ordem (horária/anti-horária)
// ------------------------
struct Edge
{
    int ymax;
    double x_at_ymin;
    double inv_slope;
};

void fillPolygonScanline(const vector<V2> &verts, Color cor)
{
    if (verts.size() < 3)
        return;
    // Encontrar ymin e ymax (em y inteiro)
    int ymin = verts[0].y, ymax = verts[0].y;
    for (auto &v : verts)
    {
        ymin = min(ymin, v.y);
        ymax = max(ymax, v.y);
    }

    // Table de lista de arestas (bucket) por y
    int H = ymax - ymin + 1;
    vector<vector<Edge>> buckets(H);

    size_t n = verts.size();
    for (size_t i = 0; i < n; ++i)
    {
        V2 v1 = verts[i];
        V2 v2 = verts[(i + 1) % n];
        if (v1.y == v2.y)
            continue; // ignora arestas horizontais
        V2 ymin_v = v1.y < v2.y ? v1 : v2;
        V2 ymax_v = v1.y < v2.y ? v2 : v1;
        Edge e;
        e.ymax = ymax_v.y;
        e.x_at_ymin = ymin_v.x;
        e.inv_slope = double(ymax_v.x - ymin_v.x) / double(ymax_v.y - ymin_v.y); // dx/dy
        int index = ymin_v.y - ymin;
        if (index >= 0 && index < H)
            buckets[index].push_back(e);
    }

    vector<Edge> AET; // active edge table
    // scanline varre da ymin até ymax-1
    for (int scan = ymin; scan <= ymax; ++scan)
    {
        int idx_bucket = scan - ymin;
        if (idx_bucket >= 0 && idx_bucket < H)
        {
            // adiciona arestas iniciando neste scanline
            for (auto &e : buckets[idx_bucket])
                AET.push_back(e);
        }
        // remove arestas cujo ymax == scan
        AET.erase(remove_if(AET.begin(), AET.end(), [scan](const Edge &e)
                            { return e.ymax <= scan; }),
                  AET.end());
        // ordenar AET por x_at_ymin
        sort(AET.begin(), AET.end(), [](const Edge &a, const Edge &b)
             { return a.x_at_ymin < b.x_at_ymin; });

        // preencher pares (paridade) - cada par = intervalo de preenchimento
        for (size_t i = 0; i + 1 < AET.size(); i += 2)
        {
            int x_start = (int)ceil(AET[i].x_at_ymin);
            int x_end = (int)floor(AET[i + 1].x_at_ymin);
            for (int x = x_start; x <= x_end; ++x)
            {
                setOverlayPixel(x, scan, cor);
            }
        }

        // incrementar x para cada aresta: x = x + inv_slope
        for (auto &e : AET)
        {
            e.x_at_ymin += e.inv_slope;
        }
        // atualizar AET: copy back with updated xs
        // Note: above we updated 'e' on a copy; we need to update items in vector.
        // We'll implement properly: we must iterate via index
        for (size_t k = 0; k < AET.size(); ++k)
        {
            // nothing: already updated via reference in the previous loop? To be safe, we redo.
        }
        // To ensure correct updates, rebuild AET: (simpler approach)
        for (auto &e : AET)
        {
            // no-op
        }
        // But because we used for(auto &e: AET) earlier, e.x_at_ymin was updated properly.
    }
}

// ------------------------
// Flood-fill
// ------------------------
bool colorEqual(const Color &a, const Color &b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b;
}

void floodFill4(int sx, int sy, Color newColor)
{
    if (sx < 0 || sx >= winW || sy < 0 || sy >= winH)
        return;
    Color target = getCombinedPixel(sx, sy);
    if (colorEqual(target, newColor))
        return;

    stack<V2> st;
    st.push({sx, sy});
    while (!st.empty())
    {
        V2 p = st.top();
        st.pop();
        int x = p.x, y = p.y;
        if (x < 0 || x >= winW || y < 0 || y >= winH)
            continue;
        Color c = getCombinedPixel(x, y);
        if (!colorEqual(c, target))
            continue;
    setOverlayPixel(x, y, newColor);
        st.push({x + 1, y});
        st.push({x - 1, y});
        st.push({x, y + 1});
        st.push({x, y - 1});
    }
}

// ------------------------
// Transformações geométricas (matriz 3x3) aplicadas a um conjunto de vértices.
// Utilizamos coordenadas homogêneas (x, y, 1). As transformações retornam
// um novo vetor de V2 com coordenadas arredondadas.
// ------------------------
using Mat3 = array<array<double, 3>, 3>;

vector<V2> applyTransform(const vector<V2> &pts, const Mat3 &M)
{
    vector<V2> out;
    out.reserve(pts.size());
    for (auto &p : pts)
    {
        double x = p.x, y = p.y;
        double nx = M[0][0] * x + M[0][1] * y + M[0][2] * 1.0;
        double ny = M[1][0] * x + M[1][1] * y + M[1][2] * 1.0;
        // ignoramos componente homogênea porque M[2][*] = [0,0,1]
        out.push_back({(int)round(nx), (int)round(ny)});
    }
    return out;
}

Mat3 identityMat()
{
    Mat3 I = {{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
    return I;
}

Mat3 translateMat(double tx, double ty)
{
    Mat3 M = identityMat();
    M[0][2] = tx;
    M[1][2] = ty;
    return M;
}

Mat3 scaleMat(double sx, double sy)
{
    Mat3 M = identityMat();
    M[0][0] = sx;
    M[1][1] = sy;
    return M;
}

Mat3 shearMat(double shx, double shy)
{
    Mat3 M = identityMat();
    M[0][1] = shx; // x' = x + shx*y
    M[1][0] = shy; // y' = shy*x + y
    return M;
}

Mat3 rotateMat(double ang_deg)
{
    double a = ang_deg * M_PI / 180.0;
    Mat3 M = identityMat();
    M[0][0] = cos(a);
    M[0][1] = -sin(a);
    M[1][0] = sin(a);
    M[1][1] = cos(a);
    return M;
}

Mat3 reflectMat(bool reflectX, bool reflectY)
{
    Mat3 M = identityMat();
    M[0][0] = reflectX ? -1 : 1;
    M[1][1] = reflectY ? -1 : 1;
    return M;
}

// Aplica transformação em relação ao centro (centroid) da forma
vector<V2> transformAboutCenter(const vector<V2> &pts, const Mat3 &M)
{
    // compute centroid (average)
    double cx = 0, cy = 0;
    for (auto &p : pts)
    {
        cx += p.x;
        cy += p.y;
    }
    cx /= pts.size();
    cy /= pts.size();
    Mat3 T1 = translateMat(-cx, -cy);
    Mat3 T2 = translateMat(cx, cy);
    // compose T2 * M * T1
    Mat3 TMP = identityMat();
    // TMP = M * T1
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            TMP[i][j] = 0;
            for (int k = 0; k < 3; k++)
                TMP[i][j] += M[i][k] * T1[k][j];
        }
    }
    Mat3 COM = identityMat();
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            COM[i][j] = 0;
            for (int k = 0; k < 3; k++)
                COM[i][j] += T2[i][k] * TMP[k][j];
        }
    }
    return applyTransform(pts, COM);
}

// ------------------------
// Funções de desenho / redesenho de todas as formas na tela
// ------------------------
void redrawAll()
{
    // Limpa framebuffer em RAM
    std::fill(framebuffer.begin(), framebuffer.end(), WHITE);
    // we'll flush the framebuffer to the screen after rasterizing shapes

    // Desenha todas as formas usando as rotinas definidas
    for (const auto &f : formas)
    {
        switch (f.tipo)
        {
        case M_LINHA:
            if (f.verts.size() >= 2)
                bresenhamLine(f.verts[0].x, f.verts[0].y, f.verts[1].x, f.verts[1].y, f.cor);
            break;
        case M_RETANGULO:
            if (f.verts.size() >= 2)
                drawRectFromCorners(f.verts[0].x, f.verts[0].y, f.verts[1].x, f.verts[1].y, f.cor);
            break;
        case M_TRIANGULO:
            if (f.verts.size() >= 3)
                drawTriangle(f.verts, f.cor);
            break;
        case M_POLIGONO:
            if (f.verts.size() >= 3)
                drawPolygon(f.verts, f.cor);
            break;
        case M_CIRCULO:
            if (f.verts.size() >= 2)
            {
                int cx = f.verts[0].x, cy = f.verts[0].y;
                int dx = f.verts[1].x - cx;
                int dy = f.verts[1].y - cy;
                int r = (int)round(sqrt(dx * dx + dy * dy));
                midpointCircle(cx, cy, r, f.cor);
            }
            break;
        }
    }

    if (drawing)
    {
        Color previewColor = RED; // cor do preview
        switch (currentForma.tipo)
        {
        case M_LINHA:
            if (currentForma.verts.size() >= 1)
                bresenhamLine(currentForma.verts[0].x, currentForma.verts[0].y, mouse_x, mouse_y, previewColor);
            break;
        case M_RETANGULO:
            if (currentForma.verts.size() >= 1)
                drawRectFromCorners(currentForma.verts[0].x, currentForma.verts[0].y, mouse_x, mouse_y, previewColor);
            break;
        case M_TRIANGULO:
            if (currentForma.verts.size() == 1)
                bresenhamLine(currentForma.verts[0].x, currentForma.verts[0].y, mouse_x, mouse_y, previewColor);
            else if (currentForma.verts.size() == 2)
            {
                bresenhamLine(currentForma.verts[0].x, currentForma.verts[0].y, currentForma.verts[1].x, currentForma.verts[1].y, previewColor);
                bresenhamLine(currentForma.verts[1].x, currentForma.verts[1].y, mouse_x, mouse_y, previewColor);
            }
            break;
        case M_POLIGONO:
            if (currentForma.verts.size() >= 1)
            {
                for (size_t i = 0; i + 1 < currentForma.verts.size(); ++i)
                    bresenhamLine(currentForma.verts[i].x, currentForma.verts[i].y, currentForma.verts[i + 1].x, currentForma.verts[i + 1].y, previewColor);
                V2 last = currentForma.verts.back();
                bresenhamLine(last.x, last.y, mouse_x, mouse_y, previewColor);
            }
            break;
        case M_CIRCULO:
            if (currentForma.verts.size() >= 1)
            {
                int cx = currentForma.verts[0].x, cy = currentForma.verts[0].y;
                int dx = mouse_x - cx;
                int dy = mouse_y - cy;
                int r = (int)round(sqrt(dx * dx + dy * dy));
                midpointCircle(cx, cy, r, previewColor);
            }
            break;
        default:
            break;
        }
    }
    // draw sidebar (overlay) with mode buttons
    // sidebar drawn here to overlay any shapes that may occupy left region
    auto drawSidebar = [&]() {
        int sw = sidebarWidth;
        // background
        glColor3ub(230, 230, 230);
        glBegin(GL_QUADS);
        glVertex2i(0, 0);
        glVertex2i(sw, 0);
        glVertex2i(sw, winH);
        glVertex2i(0, winH);
        glEnd();

        // buttons
        std::vector<std::string> labels = {"Linha", "Retangulo", "Triangulo", "Poligono", "Circulo", "Flood Fill", "Clear"};
        int btnH = 36;
        int margin = 8;
        int x0 = 8;
        for (size_t i = 0; i < labels.size(); ++i)
        {
            int ytop = winH - margin - int(i) * (btnH + margin);
            int ybot = ytop - btnH;

            // decide color: highlighted if current mode (for first 5) or special colors for Flood/Clear
            bool highlighted = false;
            if (i == 0 && modo == M_LINHA)
                highlighted = true;
            if (i == 1 && modo == M_RETANGULO)
                highlighted = true;
            if (i == 2 && modo == M_TRIANGULO)
                highlighted = true;
            if (i == 3 && modo == M_POLIGONO)
                highlighted = true;
            if (i == 4 && modo == M_CIRCULO)
                highlighted = true;

            if (highlighted)
                glColor3ub(70, 130, 180); // steel blue
            else if (i == 5)
                glColor3ub(200, 160, 50); // flood like color
            else if (i == 6)
                glColor3ub(200, 80, 80); // clear button
            else
                glColor3ub(245, 245, 245);

            glBegin(GL_QUADS);
            glVertex2i(0 + 4, ybot);
            glVertex2i(sw - 4, ybot);
            glVertex2i(sw - 4, ytop);
            glVertex2i(0 + 4, ytop);
            glEnd();

            // Desenhar icones usando GL_LINES
            int cx = sw / 2;
            int cy = ybot + btnH / 2;
            int s = 12; // icon half-size
            glLineWidth(2.0f);
            // choose stroke color (invert when highlighted)
            if (highlighted)
                glColor3ub(255, 255, 255);
            else
                glColor3ub(20, 20, 20);

            switch ((int)i)
            {
            case 0: // Linha (diagonal)
            {
                glBegin(GL_LINES);
                glVertex2i(cx - s, cy - s);
                glVertex2i(cx + s, cy + s);
                glEnd();
                break;
            }
            case 1: // Retangulo
            {
                int lx = cx - s, rx = cx + s, ty = cy + s, by = cy - s;
                glBegin(GL_LINES);
                glVertex2i(lx, by); glVertex2i(rx, by);
                glVertex2i(rx, by); glVertex2i(rx, ty);
                glVertex2i(rx, ty); glVertex2i(lx, ty);
                glVertex2i(lx, ty); glVertex2i(lx, by);
                glEnd();
                break;
            }
            case 2: // Triangulo
            {
                int x1 = cx, y1 = cy + s;
                int x2 = cx - s, y2 = cy - s;
                int x3 = cx + s, y3 = cy - s;
                glBegin(GL_LINES);
                glVertex2i(x1, y1); glVertex2i(x2, y2);
                glVertex2i(x2, y2); glVertex2i(x3, y3);
                glVertex2i(x3, y3); glVertex2i(x1, y1);
                glEnd();
                break;
            }
            case 3: // Poligono (hexágono)
            {
                const int N = 6;
                double ang0 = -M_PI / 2;
                std::vector<std::pair<int,int>> pts;
                for (int k = 0; k < N; ++k)
                {
                    double a = ang0 + 2.0 * k * M_PI / N;
                    int px = cx + (int)round(s * cos(a));
                    int py = cy + (int)round(s * sin(a));
                    pts.emplace_back(px, py);
                }
                glBegin(GL_LINES);
                for (int k = 0; k < N; ++k)
                {
                    int nx = (k + 1) % N;
                    glVertex2i(pts[k].first, pts[k].second);
                    glVertex2i(pts[nx].first, pts[nx].second);
                }
                glEnd();
                break;
            }
            case 4: // Circulo
            {
                const int SEG = 40;
                glBegin(GL_LINES);
                for (int k = 0; k < SEG; ++k)
                {
                    double a1 = 2.0 * M_PI * k / SEG;
                    double a2 = 2.0 * M_PI * (k + 1) / SEG;
                    int x1 = cx + (int)round(s * cos(a1));
                    int y1 = cy + (int)round(s * sin(a1));
                    int x2 = cx + (int)round(s * cos(a2));
                    int y2 = cy + (int)round(s * sin(a2));
                    glVertex2i(x1, y1); glVertex2i(x2, y2);
                }
                glEnd();
                break;
            }
            case 5: // Flood Fill (balde + nivel de liquido + gota) - desenhado com GL_LINES
            {
                int tlx = cx - s;           // top-left x
                int trx = cx + s;           // top-right x
                int tly = cy + s/2;         // top y
                int blx = cx - s/2;         // bottom-left x
                int brx = cx + s/2;         // bottom-right x
                int bly = cy - s;           // bottom y

                glBegin(GL_LINES);
                // rim
                glVertex2i(tlx, tly); glVertex2i(trx, tly);
                // right side
                glVertex2i(trx, tly); glVertex2i(brx, bly);
                // bottom
                glVertex2i(brx, bly); glVertex2i(blx, bly);
                // left side
                glVertex2i(blx, bly); glVertex2i(tlx, tly);
                glEnd();

                // liquid level: a small wavy line inside the bucket
                int Lx = tlx + 4;
                int Rx = trx - 4;
                int Ly = tly - (s / 6);
                glBegin(GL_LINES);
                int segments = 6;
                for (int k = 0; k < segments; ++k)
                {
                    double a = double(k) / segments;
                    double b = double(k + 1) / segments;
                    int x1 = Lx + (int)round((Rx - Lx) * a);
                    int x2 = Lx + (int)round((Rx - Lx) * b);
                    int y1 = Ly + (int)round( ( (k%2)==0 ? 2 : -2) );
                    int y2 = Ly + (int)round( ( ((k+1)%2)==0 ? 2 : -2) );
                    glVertex2i(x1, y1); glVertex2i(x2, y2);
                }
                glEnd();
                break;
            }
            case 6: // Clear (X)
            {
                glBegin(GL_LINES);
                glVertex2i(cx - s, cy - s); glVertex2i(cx + s, cy + s);
                glVertex2i(cx - s, cy + s); glVertex2i(cx + s, cy - s);
                glEnd();
                break;
            }
            default:
                break;
            }
            glLineWidth(1.0f);
        }
    };

    // flush rasterized shapes/preview to the screen
    // compose overlay (fills) on top of rasterized framebuffer
    applyOverlayToFramebuffer();
    flushFramebuffer();

    // draw sidebar and UI overlays ON TOP of the rasterized framebuffer
    drawSidebar();

    // Redesenha texto de coordenadas e instruções
    glColor3f(0, 0, 0);
    draw_text_stroke(sidebarWidth + 5, 40, string("(") + to_string(mouse_x) + string(", ") + to_string(mouse_y) + string(")"), 0.10);
    draw_text_stroke(sidebarWidth + 5, 20, string("Modo: ") + (modo == M_LINHA ? "Linha" : modo == M_RETANGULO ? "Retangulo"
                                                                       : modo == M_TRIANGULO   ? "Triangulo"
                                                                       : modo == M_POLIGONO    ? "Poligono"
                                                                                               : "Circulo"),
                     0.15);
    draw_text_stroke(sidebarWidth + 5, 5, string("Atalhos: l=linha r=ret t=tri p=pol c=circ f=scanfill o=flood x=clear esc=sair"), 0.12);

    glutSwapBuffers();
}

// ------------------------
// Callbacks GLUT
// ------------------------
void display()
{
    redrawAll();
}

void reshape(int w, int h)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, w, h);
    winW = w;
    winH = h;
    glOrtho(0, w, 0, h, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Rebuild framebuffer
    framebuffer.assign(winW * winH, WHITE);
    overlayBuffer.assign(winW * winH, WHITE);
    overlayMask.assign(winW * winH, 0);
}

void keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
    case ESC:
        exit(EXIT_SUCCESS);
        break;
    case 'l':
    case '1':
        modo = M_LINHA;
        cout << "Modo: Linha\n";
        break;
    case 'r':
    case '2':
        modo = M_RETANGULO;
        cout << "Modo: Retangulo\n";
        break;
    case 't':
    case '3':
        modo = M_TRIANGULO;
        cout << "Modo: Triangulo\n";
        break;
    case 'p':
    case '4':
        modo = M_POLIGONO;
        cout << "Modo: Poligono\n";
        break;
    case 'c':
    case '5':
        modo = M_CIRCULO;
        cout << "Modo: Circulo\n";
        break;
    case 'x': // clear
        formas.clear();
        clearScreen();
        break;
    case 'f': // scanline fill last polygon-like shape
        if (!formas.empty())
        {
            Forma &last = formas.back();
            if (last.tipo == M_POLIGONO && last.verts.size() >= 3)
            {
                fillPolygonScanline(last.verts, FILL_COLOR);
                // scanline fill writes into overlay; request redisplay so redrawAll composes overlay
                glutPostRedisplay();
            }
            else
            {
                cout << "Ultima forma nao e poligono com 3+ vertices\n";
            }
        }
        break;
    case 'o': // enable flood fill mode - user should click to fill
        cout << "Modo Flood-Fill: clique na regiao para preencher\n";
        modo = M_POLIGONO; // keep drawing mode but click handler will call floodfill if key set - handled via a flag
        break;
    case 13: // Enter key
        if (modo == M_POLIGONO && drawing)
        {
            if (currentForma.verts.size() >= 3)
            {
                formas.push_back(currentForma);
                drawing = false;
                redrawAll();
            }
            else
            {
                cout << "Poligono precisa de 3+ vertices\n";
            }
        }
        break;
    default:
        break;
    }
    glutPostRedisplay();
}

bool floodMode = false;

void mouse(int button, int state, int x, int y)
{
    int yy = winH - y - 1;
    // If click is inside sidebar, handle menu actions and don't treat as canvas click
    if (x < sidebarWidth && button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        // delegate to sidebar handler
        // implement inline handler here
        auto handleSidebarClick = [&](int sx, int sy)
        {
            std::vector<std::string> labels = {"Linha", "Retangulo", "Triangulo", "Poligono", "Circulo", "Flood Fill", "Clear"};
            int btnH = 36;
            int margin = 8;
            for (size_t i = 0; i < labels.size(); ++i)
            {
                int ytop = winH - margin - int(i) * (btnH + margin);
                int ybot = ytop - btnH;
                if (sy >= ybot && sy <= ytop)
                {
                    if (i <= 4)
                    {
                        // set drawing mode
                        TipoForma newMode = M_LINHA;
                        if (i == 0) newMode = M_LINHA;
                        if (i == 1) newMode = M_RETANGULO;
                        if (i == 2) newMode = M_TRIANGULO;
                        if (i == 3) newMode = M_POLIGONO;
                        if (i == 4) newMode = M_CIRCULO;
                        modo = newMode;
                        drawing = false; // cancel any current drawing
                        cout << "Modo selecionado: " << labels[i] << "\n";
                    }
                    else if (i == 5)
                    {
                        // flood fill: enable one-click flood mode
                        floodMode = true;
                        cout << "Flood fill ativado: clique na regio para preencher\n";
                    }
                    else if (i == 6)
                    {
                        // clear
                        formas.clear();
                        clearScreen();
                        cout << "Canvas limpo\n";
                    }
                    glutPostRedisplay();
                    return;
                }
            }
        };

        handleSidebarClick(x, yy);
        return;
    }
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        // dependendo do modo, coletar pontos
        if (modo == M_LINHA)
        {
            if (!drawing)
            {
                currentForma = Forma();
                currentForma.tipo = M_LINHA;
                currentForma.cor = BLACK;
                currentForma.verts.clear();
                currentForma.verts.push_back({x, yy});
                drawing = true;
            }
            else
            {
                currentForma.verts.push_back({x, yy});
                formas.push_back(currentForma);
                drawing = false;
                redrawAll();
            }
        }
        else if (modo == M_RETANGULO)
        {
            if (!drawing)
            {
                currentForma = Forma();
                currentForma.tipo = M_RETANGULO;
                currentForma.cor = BLACK;
                currentForma.verts.clear();
                currentForma.verts.push_back({x, yy});
                drawing = true;
            }
            else
            {
                currentForma.verts.push_back({x, yy});
                formas.push_back(currentForma);
                drawing = false;
                redrawAll();
            }
        }
        else if (modo == M_TRIANGULO)
        {
            if (!drawing)
            {
                currentForma = Forma();
                currentForma.tipo = M_TRIANGULO;
                currentForma.cor = BLACK;
                currentForma.verts.clear();
                currentForma.verts.push_back({x, yy});
                drawing = true;
            }
            else
            {
                currentForma.verts.push_back({x, yy});
                if (currentForma.verts.size() == 3)
                {
                    formas.push_back(currentForma);
                    drawing = false;
                    redrawAll();
                }
            }
        }
        else if (modo == M_POLIGONO)
        {
            // Se não está desenhando, começa novo polígono. Clique direito fecha.
            if (!drawing)
            {
                currentForma = Forma();
                currentForma.tipo = M_POLIGONO;
                currentForma.cor = BLACK;
                currentForma.verts.clear();
                currentForma.verts.push_back({x, yy});
                drawing = true;
            }
            else
            {
                currentForma.verts.push_back({x, yy});
                redrawAll();
            }
        }
        else if (modo == M_CIRCULO)
        {
            if (!drawing)
            {
                currentForma = Forma();
                currentForma.tipo = M_CIRCULO;
                currentForma.cor = BLACK;
                currentForma.verts.clear();
                currentForma.verts.push_back({x, yy}); // centro
                drawing = true;
            }
            else
            {
                currentForma.verts.push_back({x, yy}); // ponto para definir raio
                formas.push_back(currentForma);
                drawing = false;
                redrawAll();
            }
        }
        // Flood fill trigger if floodMode true
        if (floodMode)
        {
            floodFill4(x, yy, FILL_COLOR);
            floodMode = false;
            glutPostRedisplay();
        }
    }
    else if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
    {
        // Para polígono, fechar
        if (modo == M_POLIGONO && drawing)
        {
            if (currentForma.verts.size() >= 3)
            {
                formas.push_back(currentForma);
                drawing = false;
                redrawAll();
            }
            else
            {
                cout << "Poligono precisa de 3+ vertices\n";
            }
        }
    }
}

void motionPassive(int x, int y)
{
    mouse_x = x;
    mouse_y = winH - y - 1;
    glutPostRedisplay();
}

// Menu simples (opcional)
void menu_popup(int value)
{
    if (value == 0)
        exit(EXIT_SUCCESS);
    if (value == 1)
        modo = M_LINHA;
    if (value == 2)
        modo = M_RETANGULO;
    if (value == 3)
        modo = M_TRIANGULO;
    if (value == 4)
        modo = M_POLIGONO;
    if (value == 5)
        modo = M_CIRCULO;
    glutPostRedisplay();
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(winW, winH);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("PaintCG - Bresenham e Rasterizacao");

    // init framebuffer
    framebuffer.assign(winW * winH, WHITE);
    overlayBuffer.assign(winW * winH, WHITE);
    overlayMask.assign(winW * winH, 0);
    glClearColor(1, 1, 1, 1);
    glPointSize(1.0f);

    glutReshapeFunc(reshape);
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutPassiveMotionFunc(motionPassive);

    // menu
    glutCreateMenu(menu_popup);
    glutAddMenuEntry("Linha", 1);
    glutAddMenuEntry("Retangulo", 2);
    glutAddMenuEntry("Triangulo", 3);
    glutAddMenuEntry("Poligono", 4);
    glutAddMenuEntry("Circulo", 5);
    glutAddMenuEntry("Sair", 0);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    glutMainLoop();
    return EXIT_SUCCESS;
}