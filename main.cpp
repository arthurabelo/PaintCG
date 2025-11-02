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
enum TipoForma { M_LINHA = 1, M_TRIANGULO, M_RETANGULO, M_POLIGONO, M_CIRCULO };

// Cor (RGB)
struct Color { unsigned char r,g,b; };
const Color WHITE = {255,255,255};
const Color BLACK = {0,0,0};
const Color RED = {255,0,0};
const Color GREEN = {0,255,0};
const Color BLUE = {0,0,255};
const Color FILL_COLOR = {200,200,20}; // cor padrão de preenchimento

// Vertice inteiro
struct V2 { int x,y; };

// Forma geométrica (lista de vértices)
struct Forma {
    TipoForma tipo;
    vector<V2> verts; // Para linhas: 2 verts; tri: 3; ret: 2 (sup-esq, inf-dir); pol: n>=4; circ: 2 (centro, ponto raio)
    Color cor = BLACK;
};

// Dimensões janela / mouse
int winW = 800, winH = 600;
int mouse_x = 0, mouse_y = 0;

bool click1 = false; // verifica se foi realizado o primeiro clique do mouse (exemplo)
int m_x = 0, m_y = 0; // coordenadas do mouse (exemplo)
int x_1 = 0, y_1 = 0, x_2 = 0, y_2 = 0; // cliques 1 e 2 (exemplo)
int width = winW, height = winH; // largura/altura - alias visual para o exemplo

// Estado do aplicativo
vector<Forma> formas;
Forma currentForma;
bool drawing = false; // se estamos no meio de desenhar
TipoForma modo = M_LINHA;

// Framebuffer auxiliar (armazenar cor de cada pixel)
vector<Color> framebuffer; // tamanho winW * winH, row-major

// Funções utilitárias de pixel / framebuffer
inline int idx(int x, int y) { return y * winW + x; }

void setPixelBuffer(int x, int y, Color c){
    if(x < 0 || x >= winW || y < 0 || y >= winH) return;
    framebuffer[idx(x,y)] = c;
}

Color getPixelBuffer(int x, int y){
    if(x < 0 || x >= winW || y < 0 || y >= winH) return WHITE;
    return framebuffer[idx(x,y)];
}

// Desenha pixel usando GL_POINTS
void drawPixelGL(int x, int y, Color c){
    // Atualiza buffer auxiliar
    setPixelBuffer(x,y,c);

    glColor3ub(c.r, c.g, c.b);
    glBegin(GL_POINTS);
        glVertex2i(x, y);
    glEnd();
}

// Limpa tela (framebuffer + OpenGL)
void clearScreen(){
    // Limpa framebuffer em RAM
    std::fill(framebuffer.begin(), framebuffer.end(), WHITE);

    glClear(GL_COLOR_BUFFER_BIT);
    // Preenche todos os pixels brancos com GL_POINTS para manter consistência,
    // mas isso é opcional — iremos redesenhar formas a partir do buffer.
    for(int y = 0; y < winH; ++y){
        for(int x = 0; x < winW; ++x){
            // usa GL_POINTS
            glColor3ub(WHITE.r, WHITE.g, WHITE.b);
            glBegin(GL_POINTS);
                glVertex2i(x,y);
            glEnd();
        }
    }
    glutSwapBuffers();
}

// ------------------------
// Bresenham (linha)
// ------------------------

struct OctantFlags {
    bool steep;
    int x0, y0, x1, y1;
    int ystep;
};

OctantFlags computeOctantFlags(int x0, int y0, int x1, int y1){
    OctantFlags f;
    f.steep = (abs(y1 - y0) > abs(x1 - x0));
    if(f.steep){
        swap(x0, y0);
        swap(x1, y1);
    }
    if(x0 > x1){
        swap(x0, x1);
        swap(y0, y1);
    }
    f.x0 = x0; f.y0 = y0; f.x1 = x1; f.y1 = y1;
    f.ystep = (y0 < y1) ? 1 : -1;
    return f;
}

// Plot um ponto transformando de volta para o sistema original (inverso da redução)
void plotFromTransformed(int x, int y, const OctantFlags &f, Color cor){
    int px = x, py = y;
    if(f.steep) swap(px, py);
    // No computeOctantFlags garantimos que x0 <= x1; porém se houve swap para ordenar, y may need step inversion
    // O algoritmo principal já usa ystep para mover em direção correta.
    drawPixelGL(px, py, cor);
}

// Bresenham geral (utilizando redução ao "primeiro octante" via steep + swap)
void bresenhamLine(int x0, int y0, int x1, int y1, Color cor){
    // Special case: degenerate
    if(x0 == x1 && y0 == y1){
        drawPixelGL(x0, y0, cor);
        return;
    }

    OctantFlags f = computeOctantFlags(x0, y0, x1, y1);
    int dx = f.x1 - f.x0;
    int dy = abs(f.y1 - f.y0);
    int err = dx / 2;
    int y = f.y0;
    for(int x = f.x0; x <= f.x1; ++x){
        // map (x,y) back and draw
        int px = x, py = y;
        if(f.steep) swap(px, py);
        drawPixelGL(px, py, cor);

        err -= dy;
        if(err < 0){
            y += f.ystep;
            err += dx;
        }
    }
}

// ------------------------
// Círculo: algoritmo de midpoint (variante de Bresenham para círculos)
// Recebe centro (cx,cy) e raio r
// ------------------------
void plotCirclePoints(int cx, int cy, int x, int y, Color cor){
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

void midpointCircle(int cx, int cy, int r, Color cor){
    int x = 0, y = r;
    int d = 1 - r;
    plotCirclePoints(cx, cy, x, y, cor);
    while(x < y){
        if(d < 0){
            d += 2*x + 3;
            x++;
        }else{
            d += 2*(x - y) + 5;
            x++; y--;
        }
        plotCirclePoints(cx, cy, x, y, cor);
    }
}

// ------------------------
// Retângulo: desenha 4 arestas com Bresenham
// Recebe canto superior esquerdo (x1,y1) e canto inferior direito (x2,y2)
// ------------------------
void drawRectFromCorners(int x1, int y1, int x2, int y2, Color cor){
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
void drawTriangle(const vector<V2> &pts, Color cor){
    if(pts.size() < 3) return;
    bresenhamLine(pts[0].x, pts[0].y, pts[1].x, pts[1].y, cor);
    bresenhamLine(pts[1].x, pts[1].y, pts[2].x, pts[2].y, cor);
    bresenhamLine(pts[2].x, pts[2].y, pts[0].x, pts[0].y, cor);
}

// ------------------------
// Polígono: desenha arestas consecutivas e fecha
// ------------------------
void drawPolygon(const vector<V2> &pts, Color cor){
    if(pts.size() < 2) return;
    for(size_t i = 0; i < pts.size()-1; ++i){
        bresenhamLine(pts[i].x, pts[i].y, pts[i+1].x, pts[i+1].y, cor);
    }
    // fechar
    bresenhamLine(pts.back().x, pts.back().y, pts.front().x, pts.front().y, cor);
}

// ------------------------
// Scanline Fill para polígonos simples (não necessariamente convexos)
// Entrada: vetor de vértices em ordem (horária/anti-horária)
// ------------------------
struct Edge {
    int ymax;
    double x_at_ymin;
    double inv_slope;
};

void fillPolygonScanline(const vector<V2> &verts, Color cor){
    if(verts.size() < 3) return;
    // Encontrar ymin e ymax (em y inteiro)
    int ymin = verts[0].y, ymax = verts[0].y;
    for(auto &v: verts){ ymin = min(ymin, v.y); ymax = max(ymax, v.y); }

    // Table de lista de arestas (bucket) por y
    int H = ymax - ymin + 1;
    vector<vector<Edge>> buckets(H);

    size_t n = verts.size();
    for(size_t i = 0; i < n; ++i){
        V2 v1 = verts[i];
        V2 v2 = verts[(i+1)%n];
        if(v1.y == v2.y) continue; // ignora arestas horizontais
        V2 ymin_v = v1.y < v2.y ? v1 : v2;
        V2 ymax_v = v1.y < v2.y ? v2 : v1;
        Edge e;
        e.ymax = ymax_v.y;
        e.x_at_ymin = ymin_v.x;
        e.inv_slope = double(ymax_v.x - ymin_v.x) / double(ymax_v.y - ymin_v.y); // dx/dy
        int index = ymin_v.y - ymin;
        if(index >= 0 && index < H) buckets[index].push_back(e);
    }

    vector<Edge> AET; // active edge table
    // scanline varre da ymin até ymax-1
    for(int scan = ymin; scan <= ymax; ++scan){
        int idx_bucket = scan - ymin;
        if(idx_bucket >= 0 && idx_bucket < H){
            // adiciona arestas iniciando neste scanline
            for(auto &e: buckets[idx_bucket]) AET.push_back(e);
        }
        // remove arestas cujo ymax == scan
        AET.erase(remove_if(AET.begin(), AET.end(), [scan](const Edge &e){ return e.ymax <= scan; }), AET.end());
        // ordenar AET por x_at_ymin
        sort(AET.begin(), AET.end(), [](const Edge &a, const Edge &b){ return a.x_at_ymin < b.x_at_ymin; });

        // preencher pares (paridade) - cada par = intervalo de preenchimento
        for(size_t i = 0; i+1 < AET.size(); i += 2){
            int x_start = (int)ceil(AET[i].x_at_ymin);
            int x_end = (int)floor(AET[i+1].x_at_ymin);
            for(int x = x_start; x <= x_end; ++x){
                drawPixelGL(x, scan, cor);
            }
        }

        // incrementar x para cada aresta: x = x + inv_slope
        for(auto &e : AET){
            e.x_at_ymin += e.inv_slope;
        }
        // atualizar AET: copy back with updated xs
        // Note: above we updated 'e' on a copy; we need to update items in vector.
        // We'll implement properly: we must iterate via index
        for(size_t k = 0; k < AET.size(); ++k){
            // nothing: already updated via reference in the previous loop? To be safe, we redo.
        }
        // To ensure correct updates, rebuild AET: (simpler approach)
        for(auto &e : AET){
            // no-op
        }
        // But because we used for(auto &e: AET) earlier, e.x_at_ymin was updated properly.
    }
}

// ------------------------
// Flood-fill
// ------------------------
bool colorEqual(const Color &a, const Color &b){
    return a.r==b.r && a.g==b.g && a.b==b.b;
}

void floodFill4(int sx, int sy, Color newColor){
    if(sx < 0 || sx >= winW || sy < 0 || sy >= winH) return;
    Color target = getPixelBuffer(sx, sy);
    if(colorEqual(target, newColor)) return;

    stack<V2> st;
    st.push({sx, sy});
    while(!st.empty()){
        V2 p = st.top(); st.pop();
        int x = p.x, y = p.y;
        if(x < 0 || x >= winW || y < 0 || y >= winH) continue;
        Color c = getPixelBuffer(x,y);
        if(!colorEqual(c, target)) continue;
        drawPixelGL(x,y,newColor);
        st.push({x+1,y});
        st.push({x-1,y});
        st.push({x,y+1});
        st.push({x,y-1});
    }
}

// ------------------------
// Transformações geométricas (matriz 3x3) aplicadas a um conjunto de vértices.
// Utilizamos coordenadas homogêneas (x, y, 1). As transformações retornam
// um novo vetor de V2 com coordenadas arredondadas.
// ------------------------
using Mat3 = array<array<double,3>,3>;

vector<V2> applyTransform(const vector<V2> &pts, const Mat3 &M){
    vector<V2> out;
    out.reserve(pts.size());
    for(auto &p: pts){
        double x = p.x, y = p.y;
        double nx = M[0][0]*x + M[0][1]*y + M[0][2]*1.0;
        double ny = M[1][0]*x + M[1][1]*y + M[1][2]*1.0;
        // ignoramos componente homogênea porque M[2][*] = [0,0,1]
        out.push_back({(int)round(nx), (int)round(ny)});
    }
    return out;
}

Mat3 identityMat(){
    Mat3 I = {{{1,0,0},{0,1,0},{0,0,1}}};
    return I;
}

Mat3 translateMat(double tx, double ty){
    Mat3 M = identityMat();
    M[0][2] = tx;
    M[1][2] = ty;
    return M;
}

Mat3 scaleMat(double sx, double sy){
    Mat3 M = identityMat();
    M[0][0] = sx;
    M[1][1] = sy;
    return M;
}

Mat3 shearMat(double shx, double shy){
    Mat3 M = identityMat();
    M[0][1] = shx; // x' = x + shx*y
    M[1][0] = shy; // y' = shy*x + y
    return M;
}

Mat3 rotateMat(double ang_deg){
    double a = ang_deg * M_PI / 180.0;
    Mat3 M = identityMat();
    M[0][0] = cos(a); M[0][1] = -sin(a);
    M[1][0] = sin(a); M[1][1] = cos(a);
    return M;
}

Mat3 reflectMat(bool reflectX, bool reflectY){
    Mat3 M = identityMat();
    M[0][0] = reflectX ? -1 : 1;
    M[1][1] = reflectY ? -1 : 1;
    return M;
}

// Aplica transformação em relação ao centro (centroid) da forma
vector<V2> transformAboutCenter(const vector<V2> &pts, const Mat3 &M){
    // compute centroid (average)
    double cx = 0, cy = 0;
    for(auto &p: pts){ cx += p.x; cy += p.y; }
    cx /= pts.size(); cy /= pts.size();
    Mat3 T1 = translateMat(-cx, -cy);
    Mat3 T2 = translateMat(cx, cy);
    // compose T2 * M * T1
    Mat3 TMP = identityMat();
    // TMP = M * T1
    for(int i=0;i<3;i++){
        for(int j=0;j<3;j++){
            TMP[i][j] = 0;
            for(int k=0;k<3;k++) TMP[i][j] += M[i][k] * T1[k][j];
        }
    }
    Mat3 COM = identityMat();
    for(int i=0;i<3;i++){
        for(int j=0;j<3;j++){
            COM[i][j] = 0;
            for(int k=0;k<3;k++) COM[i][j] += T2[i][k] * TMP[k][j];
        }
    }
    return applyTransform(pts, COM);
}

// ------------------------
// Funções de desenho / redesenho de todas as formas na tela
// ------------------------
void redrawAll(){
    // Limpa framebuffer em RAM
    std::fill(framebuffer.begin(), framebuffer.end(), WHITE);
    // Limpa tela
    glClear(GL_COLOR_BUFFER_BIT);

    // Desenha todas as formas usando as rotinas definidas
    for(const auto &f : formas){
        switch(f.tipo){
            case M_LINHA:
                if(f.verts.size() >= 2)
                    bresenhamLine(f.verts[0].x, f.verts[0].y, f.verts[1].x, f.verts[1].y, f.cor);
                break;
            case M_RETANGULO:
                if(f.verts.size() >= 2)
                    drawRectFromCorners(f.verts[0].x, f.verts[0].y, f.verts[1].x, f.verts[1].y, f.cor);
                break;
            case M_TRIANGULO:
                if(f.verts.size() >= 3)
                    drawTriangle(f.verts, f.cor);
                break;
            case M_POLIGONO:
                if(f.verts.size() >= 3)
                    drawPolygon(f.verts, f.cor);
                break;
            case M_CIRCULO:
                if(f.verts.size() >= 2){
                    int cx = f.verts[0].x, cy = f.verts[0].y;
                    int dx = f.verts[1].x - cx;
                    int dy = f.verts[1].y - cy;
                    int r = (int)round(sqrt(dx*dx + dy*dy));
                    midpointCircle(cx, cy, r, f.cor);
                }
                break;
        }
    }

    // Redesenha texto de coordenadas e instruções
    glColor3f(0,0,0);
    draw_text_stroke(5, 5, string("Modo: ") + (modo==M_LINHA?"Linha":modo==M_RETANGULO?"Retangulo":modo==M_TRIANGULO?"Triangulo":modo==M_POLIGONO?"Poligono":"Circulo"), 0.15);
    draw_text_stroke(5, 20, string("Clique esquerdo para adicionar vértices. Clique direito para fechar polígono."), 0.09);
    draw_text_stroke(5, 34, string("Teclas: l=linha r=ret t=tri p=pol c=circ f=scanfill o=flood x=clear esc=sair"), 0.08);

    glutSwapBuffers();
}

// ------------------------
// Callbacks GLUT
// ------------------------
void display(){
    redrawAll();
}

void reshape(int w, int h){
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0,0,w,h);
    winW = w; winH = h;
    glOrtho(0, w, 0, h, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Rebuild framebuffer
    framebuffer.assign(winW * winH, WHITE);
}

void keyboard(unsigned char key, int x, int y){
    switch(key){
        case ESC: exit(EXIT_SUCCESS); break;
        case 'l': case '1': modo = M_LINHA; cout << "Modo: Linha\n"; break;
        case 'r': case '2': modo = M_RETANGULO; cout << "Modo: Retangulo\n"; break;
        case 't': case '3': modo = M_TRIANGULO; cout << "Modo: Triangulo\n"; break;
        case 'p': case '4': modo = M_POLIGONO; cout << "Modo: Poligono\n"; break;
        case 'c': case '5': modo = M_CIRCULO; cout << "Modo: Circulo\n"; break;
        case 'x': // clear
            formas.clear();
            std::fill(framebuffer.begin(), framebuffer.end(), WHITE);
            glClear(GL_COLOR_BUFFER_BIT);
            glutSwapBuffers();
            break;
        case 'f': // scanline fill last polygon-like shape
            if(!formas.empty()){
                Forma &last = formas.back();
                if(last.tipo == M_POLIGONO && last.verts.size() >= 3){
                    fillPolygonScanline(last.verts, FILL_COLOR);
                    glutSwapBuffers();
                }else{
                    cout << "Ultima forma nao e poligono com 3+ vertices\n";
                }
            }
            break;
        case 'o': // enable flood fill mode - user should click to fill
            cout << "Modo Flood-Fill: clique na regiao para preencher\n";
            modo = M_POLIGONO; // keep drawing mode but click handler will call floodfill if key set - handled via a flag
            break;
        case 13: // Enter key
            if(modo == M_POLIGONO && drawing){
                if(currentForma.verts.size() >=3){
                    formas.push_back(currentForma);
                    drawing = false;
                    redrawAll();
                } else {
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

void mouse(int button, int state, int x, int y){
    int yy = winH - y - 1;
    if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN){
        // dependendo do modo, coletar pontos
        if(modo == M_LINHA){
            if(!drawing){
                currentForma = Forma(); currentForma.tipo = M_LINHA; currentForma.cor = BLACK;
                currentForma.verts.clear();
                currentForma.verts.push_back({x, yy});
                drawing = true;
            }else{
                currentForma.verts.push_back({x, yy});
                formas.push_back(currentForma);
                drawing = false;
                redrawAll();
            }
        } else if(modo == M_RETANGULO){
            if(!drawing){
                currentForma = Forma(); currentForma.tipo = M_RETANGULO; currentForma.cor = BLACK;
                currentForma.verts.clear();
                currentForma.verts.push_back({x, yy});
                drawing = true;
            }else{
                currentForma.verts.push_back({x, yy});
                formas.push_back(currentForma);
                drawing = false;
                redrawAll();
            }
        } else if(modo == M_TRIANGULO){
            if(!drawing){
                currentForma = Forma(); currentForma.tipo = M_TRIANGULO; currentForma.cor = BLACK;
                currentForma.verts.clear();
                currentForma.verts.push_back({x, yy});
                drawing = true;
            }else{
                currentForma.verts.push_back({x, yy});
                if(currentForma.verts.size() == 3){
                    formas.push_back(currentForma);
                    drawing = false;
                    redrawAll();
                }
            }
        } else if(modo == M_POLIGONO){
            // Se não está desenhando, começa novo polígono. Clique direito fecha.
            if(!drawing){
                currentForma = Forma(); currentForma.tipo = M_POLIGONO; currentForma.cor = BLACK;
                currentForma.verts.clear();
                currentForma.verts.push_back({x, yy});
                drawing = true;
            }else{
                currentForma.verts.push_back({x, yy});
                redrawAll();
            }
        } else if(modo == M_CIRCULO){
            if(!drawing){
                currentForma = Forma(); currentForma.tipo = M_CIRCULO; currentForma.cor = BLACK;
                currentForma.verts.clear();
                currentForma.verts.push_back({x, yy}); // centro
                drawing = true;
            }else{
                currentForma.verts.push_back({x, yy}); // ponto para definir raio
                formas.push_back(currentForma);
                drawing = false;
                redrawAll();
            }
        }
        // Flood fill trigger if floodMode true
        if(floodMode){
            floodFill4(x, yy, FILL_COLOR);
            floodMode = false;
            glutPostRedisplay();
        }
    } else if(button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN){
        // Para polígono, fechar
        if(modo == M_POLIGONO && drawing){
            if(currentForma.verts.size() >= 3){
                formas.push_back(currentForma);
                drawing = false;
                redrawAll();
            } else {
                cout << "Poligono precisa de 3+ vertices\n";
            }
        }
    }
}

void motionPassive(int x, int y){
    mouse_x = x; mouse_y = winH - y - 1;
    glutPostRedisplay();
}

// Menu simples (opcional)
void menu_popup(int value){
    if(value == 0) exit(EXIT_SUCCESS);
    if(value == 1) modo = M_LINHA;
    if(value == 2) modo = M_RETANGULO;
    if(value == 3) modo = M_TRIANGULO;
    if(value == 4) modo = M_POLIGONO;
    if(value == 5) modo = M_CIRCULO;
    glutPostRedisplay();
}

int main(int argc, char** argv){
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(winW, winH);
    glutInitWindowPosition(100,100);
    glutCreateWindow("PaintCG - Bresenham e Rasterizacao");

    // init framebuffer
    framebuffer.assign(winW * winH, WHITE);
    glClearColor(1,1,1,1);
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