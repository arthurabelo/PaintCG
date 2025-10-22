#pragma once
#include "rasterizer.h"
#include "transforms.h"
#include <vector>
#include <utility>

using IPoint = std::pair<int,int>;
using PointF = std::pair<float,float>;

struct Shape {
    virtual void draw(const Color &c) = 0;
    virtual void fill(const Color &c) = 0;
    virtual void transform(const std::vector<Point>& (*tf)(const std::vector<Point>&)) = 0;
    virtual ~Shape() = default;
};

struct LineShape : public Shape {
    IPoint a,b;
    LineShape(IPoint a_, IPoint b_):a(a_),b(b_){}
    void draw(const Color &c) override;
    void fill(const Color &c) override {}
    void transform(const std::vector<Point>& (*tf)(const std::vector<Point>&)) override;
};

struct PolygonShape : public Shape {
    std::vector<IPoint> pts;
    PolygonShape(const std::vector<IPoint>& p):pts(p){}
    void draw(const Color &c) override;
    void fill(const Color &c) override;
    void transform(const std::vector<Point>& (*tf)(const std::vector<Point>&)) override;
};

struct CircleShape : public Shape {
    IPoint center;
    int radius;
    CircleShape(IPoint c_, int r_):center(c_),radius(r_){}
    void draw(const Color &c) override;
    void fill(const Color &c) override {} // especificado: não preencher via scanline
    void transform(const std::vector<Point>& (*tf)(const std::vector<Point>&)) override;
};