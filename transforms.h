#pragma once
#include <vector>
#include <utility>
#include <cmath>

// Operamos com pares (x,y)
using Point = std::pair<float,float>;

// transformações geométricas — todas retornam novo vetor de pontos
std::vector<Point> translate(const std::vector<Point>& pts, float dx, float dy);
std::vector<Point> scale(const std::vector<Point>& pts, float sx, float sy, float cx=0.0f, float cy=0.0f);
std::vector<Point> rotate(const std::vector<Point>& pts, float angle_deg, float cx=0.0f, float cy=0.0f);
std::vector<Point> shear(const std::vector<Point>& pts, float shx, float shy, float cx=0.0f, float cy=0.0f);
std::vector<Point> reflectX(const std::vector<Point>& pts, float cx=0.0f);
std::vector<Point> reflectY(const std::vector<Point>& pts, float cy=0.0f);