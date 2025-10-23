// Interactive Paint-like editor using existing rasterizer and shape modules
#include <GL/glut.h>
#include <vector>
#include <memory>
#include <string>
#include <cstdio>
#include <algorithm> // Para std::find

#include "rasterizer.h"
#include "shapes.h"
#include "fill.h"
#include "transforms.h"
#include "clipping.h"

int winW =800, winH =600;
std::vector<Color> framebuffer;

// Tool modes
enum ToolMode { TOOL_LINE =1, TOOL_RECT, TOOL_TRI, TOOL_POLY, TOOL_CIRCLE, TOOL_FILL, TOOL_FLOOD, TOOL_CLIP };
ToolMode currentTool = TOOL_LINE;

// Line algorithms
enum LineAlg { ALG_IMMEDIATE=1, ALG_INCREMENTAL, ALG_BRESENHAM };
LineAlg currentLineAlg = ALG_BRESENHAM;

// Clipping subtools
enum ClipAlg { CLIP_COHEN=1, CLIP_BRUTE, CLIP_CYRUS };
ClipAlg currentClipAlg = CLIP_COHEN;

// Estrutura de botão para menu gráfico
struct Button {
 int x, y, w, h;
 std::string label;
 ToolMode tool;
};

// Lista de botões para cada funcionalidade
std::vector<Button> buttons = {
 {10,10,90,30, "Linha", TOOL_LINE},
 {110,10,90,30, "Retângulo", TOOL_RECT},
 {210,10,90,30, "Triângulo", TOOL_TRI},
 {310,10,90,30, "Polígono", TOOL_POLY},
 {410,10,110,30, "Circunferência", TOOL_CIRCLE},
 {530,10,90,30, "Preencher", TOOL_FILL},
 {630,10,90,30, "FloodFill", TOOL_FLOOD},
 {730,10,70,30, "Recorte", TOOL_CLIP}
};

// Interaction state
bool click1 = false;
int x1c=0,y1c=0;
int mouseX=0, mouseY=0;

// Shapes storage
std::vector<std::shared_ptr<Shape>> shapes;

// Current polygon being drawn
std::vector<IPoint> tempPoly;

// Função para desenhar um botão
void drawButton(const Button& btn, bool selected) {
 glColor3f(selected ?0.7f :0.9f,0.9f,0.9f);
 glBegin(GL_QUADS);
 glVertex2i(btn.x, btn.y);
 glVertex2i(btn.x + btn.w, btn.y);
 glVertex2i(btn.x + btn.w, btn.y + btn.h);
 glVertex2i(btn.x, btn.y + btn.h);
 glEnd();
 glColor3f(0,0,0);
 glRasterPos2i(btn.x +5, btn.y +20);
 for (char c : btn.label) {
 glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
 }
}

void setFramebufferAndGL(){
 framebuffer.assign(winW * winH, Color(255,255,255,255));
 setFrameBufferPointer(framebuffer.data(), winW, winH);
 glPointSize(1.0f);
 glMatrixMode(GL_PROJECTION);
 glLoadIdentity();
 gluOrtho2D(0, winW,0, winH);
 glClearColor(1.0f,1.0f,1.0f,1.0f); // Fundo branco
}

void redrawAll(){
 // Clear OpenGL buffer
 glClear(GL_COLOR_BUFFER_BIT);
 // Desenhar botões do menu
 for (const auto& btn : buttons) {
 drawButton(btn, currentTool == btn.tool);
 }
 // Draw background via framebuffer
 // We will draw directly using putPixel/GL draws already performed by shape draw functions.
 // Repaint all shapes by redrawing into framebuffer and GL
 // Clear framebuffer white
 for(size_t i=0;i<framebuffer.size();++i) framebuffer[i]=Color(255,255,255,255);
 setFrameBufferPointer(framebuffer.data(), winW, winH);
 // Draw saved shapes
 for(auto &s: shapes) s->draw(Color(0,0,0,255));
 // If polygon in progress, draw preview
 if(!tempPoly.empty()){
 for(size_t i=0;i+1<tempPoly.size();++i) drawLineBresenham(tempPoly[i].first,tempPoly[i].second,tempPoly[i+1].first,tempPoly[i+1].second, Color(0,0,0,255));
 // preview from last to mouse
 auto p = tempPoly.back(); drawLineBresenham(p.first,p.second, mouseX, mouseY, Color(0,0,0,255));
 }
 // Finally push pixels to screen
 glDrawPixels(winW, winH, GL_RGBA, GL_UNSIGNED_BYTE, framebuffer.data());
 glutSwapBuffers();
}

void display(){ redrawAll(); }

void reshape(int w,int h){
 winW = w; winH = h;
 glViewport(0,0,w,h);
 setFramebufferAndGL();
 glutPostRedisplay();
}

void keyboard(unsigned char key,int x,int y){
 switch(key){
 case 27: exit(EXIT_SUCCESS); break; // ESC
 case 'l': currentTool = TOOL_LINE; break;
 case 'r': currentTool = TOOL_RECT; break;
 case 'p': currentTool = TOOL_POLY; break;
 case 'c': currentTool = TOOL_CIRCLE; break;
 case 'f': currentTool = TOOL_FILL; break;
 case 'x': currentTool = TOOL_FLOOD; break;
 case 'm': currentTool = TOOL_CLIP; break;
 case '1': currentLineAlg = ALG_IMMEDIATE; break;
 case '2': currentLineAlg = ALG_INCREMENTAL; break;
 case '3': currentLineAlg = ALG_BRESENHAM; break;
 case 'C': currentClipAlg = CLIP_COHEN; break;
 case 'B': currentClipAlg = CLIP_BRUTE; break;
 case 'Y': currentClipAlg = CLIP_CYRUS; break;
 }
 glutPostRedisplay();
}

void mouse(int button,int state,int x,int y){
 int yy = winH - y -1;
 mouseX = x; mouseY = yy;
 // Verifica se clicou em algum botão do menu
 if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN){
 for (const auto& btn : buttons) {
 if (x >= btn.x && x <= btn.x + btn.w && yy >= btn.y && yy <= btn.y + btn.h) {
 currentTool = btn.tool;
 glutPostRedisplay();
 return;
 }
 }
 }
 if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN){
 if(currentTool == TOOL_LINE){
 if(!click1){ click1=true; x1c=x; y1c=yy; }
 else { // finish line
 // draw and store
 shapes.push_back(std::make_shared<LineShape>(IPoint{x1c,y1c}, IPoint{x,yy}));
 shapes.back()->draw(Color(0,0,0,255));
 click1=false; tempPoly.clear();
 }
 } else if(currentTool == TOOL_RECT){
 if(!click1){ click1=true; x1c=x; y1c=yy; }
 else {
 // Desenha retângulo usando Bresenham
 shapes.push_back(std::make_shared<PolygonShape>(std::vector<IPoint>{
 {x1c, y1c}, {x, y1c}, {x, yy}, {x1c, yy}
 }));
 shapes.back()->draw(Color(0,0,0,255));
 click1=false; tempPoly.clear();
 }
 } else if(currentTool == TOOL_TRI){
 tempPoly.emplace_back(x,yy);
 if(tempPoly.size() ==3){
 shapes.push_back(std::make_shared<PolygonShape>(tempPoly));
 shapes.back()->draw(Color(0,0,0,255));
 tempPoly.clear();
 }
 } else if(currentTool == TOOL_POLY){
 tempPoly.emplace_back(x,yy);
 // Finaliza polígono com botão direito
 } else if(currentTool == TOOL_CIRCLE){
 if(!click1){ click1=true; x1c=x; y1c=yy; }
 else { int dx = x - x1c; int dy = yy - y1c; int r = (int)std::round(std::sqrt((double)dx*dx + (double)dy*dy));
 shapes.push_back(std::make_shared<CircleShape>(IPoint{x1c,y1c}, r));
 shapes.back()->draw(Color(0,0,0,255)); click1=false; }
 } else if(currentTool == TOOL_FILL){
 // Fill last polygon if any
 if(!shapes.empty()){
 auto s = shapes.back(); s->fill(Color(200,200,255,255));
 }
 } else if(currentTool == TOOL_FLOOD){
 floodFill4(x, yy, Color(100,200,100,255));
 } else if(currentTool == TOOL_CLIP){
 // simple demo: clip last line against window using selected clip algorithm
 if(!shapes.empty()){
 // find last line shape
 for(int i=(int)shapes.size()-1;i>=0;--i){
 LineShape *ln = dynamic_cast<LineShape*>(shapes[i].get());
 if(ln){
 IPoint a = ln->a; IPoint b = ln->b; IPoint oa, ob;
 bool ok=false;
 if(currentClipAlg == CLIP_COHEN){
 int x0=a.first,y0=a.second,x1=b.first,y1=b.second;
 ok = cohenSutherlandClip(x0,y0,x1,y1,0,0, winW-1, winH-1);
 if(ok){ oa = {x0,y0}; ob = {x1,y1}; }
 } else if(currentClipAlg == CLIP_BRUTE){
 std::vector<IPoint> rect = {{0,0},{winW-1,0},{winW-1,winH-1},{0,winH-1}};
 ok = bruteForceClipSegment(a,b,rect,oa,ob);
 } else {
 std::vector<IPoint> rect = {{0,0},{winW-1,0},{winW-1,winH-1},{0,winH-1}};
 ok = cyrusBeckClip(a,b,rect,oa,ob);
 }
 if(ok){
 // replace shape with clipped line
 shapes.erase(shapes.begin()+i);
 shapes.push_back(std::make_shared<LineShape>(oa,ob));
 }
 break;
 }
 }
 }
 }
 glutPostRedisplay();
 }
 if(button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN){
 // finish polygon on right click
 if(currentTool == TOOL_POLY && tempPoly.size()>=3){
 shapes.push_back(std::make_shared<PolygonShape>(tempPoly));
 shapes.back()->draw(Color(0,0,0,255));
 tempPoly.clear();
 glutPostRedisplay();
 }
 }
}

void mousePassiveMotion(int x,int y){ mouseX=x; mouseY = winH - y - 1; glutPostRedisplay(); }

// Popup menu callbacks
void menuClip(int value){
 if(value==1) currentClipAlg = CLIP_COHEN;
 else if(value==2) currentClipAlg = CLIP_BRUTE;
 else if(value==3) currentClipAlg = CLIP_CYRUS;
}
void menuMain(int value){
 if(value==0) exit(EXIT_SUCCESS);
}

int main(int argc,char** argv){
 glutInit(&argc, argv);
 glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
 glutInitWindowSize(winW, winH);
 glutCreateWindow("PaintCG - Interactive Editor");
 setFramebufferAndGL();

 // Simple demo content
 drawLineBresenham(50,50,300,200, Color(0,0,0,255));
 drawCircleBresenham(200,200,40, Color(0,0,255,255));
 std::vector<IPoint> demo = {{400,100},{550,120},{520,250},{380,220}};
 PolygonShape pg(demo); pg.draw(Color(0,0,0,255)); pg.fill(Color(200,200,0,255));

 // Callbacks
 glutDisplayFunc(display);
 glutReshapeFunc(reshape);
 glutKeyboardFunc(keyboard);
 glutMouseFunc(mouse);
 glutPassiveMotionFunc(mousePassiveMotion);

 // Create popup menus
 int clipMenu = glutCreateMenu(menuClip);
 glutAddMenuEntry("Cohen-Sutherland",1);
 glutAddMenuEntry("Brute Force",2);
 glutAddMenuEntry("Cyrus-Beck",3);

 glutCreateMenu(menuMain);
 glutAddMenuEntry("Sair",0);
 glutAddSubMenu("Recortar", clipMenu);
 glutAttachMenu(GLUT_RIGHT_BUTTON);

 glutMainLoop();
 return 0;
}