// Exemplo minimal de integração (apenas esqueleto)
#include <GL/glut.h>
#include "rasterizer.h"
#include "shapes.h"
#include <vector>
#include <memory>

int winW = 800, winH = 600;
std::vector<Color> framebuffer;

void display(){
    glClear(GL_COLOR_BUFFER_BIT);
    // Exibir framebuffer rapidamente via glDrawPixels (ou via varredura com GL_POINTS)
    glDrawPixels(winW, winH, GL_RGBA, GL_UNSIGNED_BYTE, framebuffer.data());
    glutSwapBuffers();
}

void init(){
    framebuffer.assign(winW * winH, Color(255,255,255,255));
    setFrameBufferPointer(framebuffer.data(), winW, winH);
    glPointSize(1.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, winW, 0, winH);
}

int main(int argc, char** argv){
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
    glutInitWindowSize(winW, winH);
    glutCreateWindow("CGPaint - exemplo");
    init();

    // Exemplo: desenhando linha com Bresenham
    drawLineBresenham(100,100,400,300, Color(0,0,0,255));
    // Exemplo: círculo
    drawCircleBresenham(200,200,50, Color(255,0,0,255));
    // Exemplo: polígono e preenchimento
    std::vector<IPoint> poly = {{500,100},{650,150},{600,300},{450,250}};
    PolygonShape pg(poly);
    pg.draw(Color(0,0,0,255));
    pg.fill(Color(0,255,0,255));

    glutDisplayFunc(display);
    glutMainLoop();
    return 0;
}