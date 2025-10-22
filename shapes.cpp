#include "shapes.h"
#include "rasterizer.h"
#include "fill.h"
#include <cmath>

void LineShape::draw(const Color &c){
    drawLineBresenham(a.first, a.second, b.first, b.second, c);
}
void LineShape::transform(const std::vector<Point>& (*tf)(const std::vector<Point>&)){
    std::vector<Point> in = {{(float)a.first, (float)a.second}, {(float)b.first, (float)b.second}};
    std::vector<Point> out = tf(in);
    a.first = (int)std::round(out[0].first); a.second = (int)std::round(out[0].second);
    b.first = (int)std::round(out[1].first); b.second = (int)std::round(out[1].second);
}

void PolygonShape::draw(const Color &c){
    if(pts.size() < 2) return;
    for(size_t i=0;i<pts.size();++i){
        auto p1 = pts[i];
        auto p2 = pts[(i+1)%pts.size()];
        drawLineBresenham(p1.first,p1.second,p2.first,p2.second,c);
    }
}
void PolygonShape::fill(const Color &c){
    std::vector<std::pair<int,int>> poly(pts.begin(), pts.end());
    fillPolygonScanline(poly, c);
}
void PolygonShape::transform(const std::vector<Point>& (*tf)(const std::vector<Point>&)){
    std::vector<Point> in;
    for(auto &p: pts) in.emplace_back((float)p.first, (float)p.second);
    auto out = tf(in);
    for(size_t i=0;i<pts.size();++i){
        pts[i].first = (int)std::round(out[i].first);
        pts[i].second = (int)std::round(out[i].second);
    }
}

void CircleShape::draw(const Color &c){
    drawCircleBresenham(center.first, center.second, radius, c);
}
void CircleShape::transform(const std::vector<Point>& (*tf)(const std::vector<Point>&)){
    std::vector<Point> in = {{(float)center.first,(float)center.second}};
    auto out = tf(in);
    center.first = (int)std::round(out[0].first);
    center.second = (int)std::round(out[0].second);
    // scaling radius: se tf contém escala uniforme na volta do centro (0,0) -> perdemos info.
    // Para suportar escala: estimar novo raio aplicando escala ao ponto (center+radius, center) e computar distância.
}