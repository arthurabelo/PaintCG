#include "transforms.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

std::vector<Point> translate(const std::vector<Point>& pts, float dx, float dy){
    std::vector<Point> out; out.reserve(pts.size());
    for(auto &p: pts) out.emplace_back(p.first + dx, p.second + dy);
    return out;
}

std::vector<Point> scale(const std::vector<Point>& pts, float sx, float sy, float cx, float cy){
    std::vector<Point> out; out.reserve(pts.size());
    for(auto &p: pts){
        float x = (p.first - cx) * sx + cx;
        float y = (p.second - cy) * sy + cy;
        out.emplace_back(x,y);
    }
    return out;
}

std::vector<Point> rotate(const std::vector<Point>& pts, float angle_deg, float cx, float cy){
    float a = angle_deg * static_cast<float>(M_PI) / 180.0f;
    float ca = cos(a), sa = sin(a);
    std::vector<Point> out; out.reserve(pts.size());
    for(auto &p: pts){
        float x = p.first - cx;
        float y = p.second - cy;
        float xr = x * ca - y * sa;
        float yr = x * sa + y * ca;
        out.emplace_back(xr + cx, yr + cy);
    }
    return out;
}

std::vector<Point> shear(const std::vector<Point>& pts, float shx, float shy, float cx, float cy){
    std::vector<Point> out; out.reserve(pts.size());
    for(auto &p: pts){
        float x = p.first - cx;
        float y = p.second - cy;
        float xs = x + shx * y;
        float ys = shy * x + y;
        out.emplace_back(xs + cx, ys + cy);
    }
    return out;
}

std::vector<Point> reflectX(const std::vector<Point>& pts, float cx){
    std::vector<Point> out; out.reserve(pts.size());
    for(auto &p: pts){
        float x = 2*cx - p.first;
        out.emplace_back(x, p.second);
    }
    return out;
}

std::vector<Point> reflectY(const std::vector<Point>& pts, float cy){
    std::vector<Point> out; out.reserve(pts.size());
    for(auto &p: pts){
        float y = 2*cy - p.second;
        out.emplace_back(p.first, y);
    }
    return out;
}