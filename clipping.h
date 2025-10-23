#pragma once
#include <utility>
#include <vector>

using IPoint = std::pair<int,int>;

// Cohen-Sutherland outcodes
enum OutCode {
    INSIDE = 0, // 0000
    LEFT   = 1, // 0001
    RIGHT  = 2, // 0010
    BOTTOM = 4, // 0100
    TOP    = 8  // 1000
};

// Cohen-Sutherland clipping against axis-aligned rectangle (xmin,ymin)-(xmax,ymax)
bool cohenSutherlandClip(int &x0, int &y0, int &x1, int &y1, int xmin, int ymin, int xmax, int ymax);

// Brute-force clipping: clip a segment against a convex polygon by checking intersections
// polygon is a list of points in order (convex or not) representing the clipping window
bool bruteForceClipSegment(const IPoint &a, const IPoint &b, const std::vector<IPoint> &poly, IPoint &outA, IPoint &outB);

// Cyrus-Beck parametric clipping for convex polygon window
// Returns true if clipped segment exists; outA/outB are clipped endpoints
bool cyrusBeckClip(const IPoint &a, const IPoint &b, const std::vector<IPoint> &convexPoly, IPoint &outA, IPoint &outB);
