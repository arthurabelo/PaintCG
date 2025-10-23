#include "clipping.h"
#include <cmath>
#include <algorithm>

static int computeOutCode(int x, int y, int xmin, int ymin, int xmax, int ymax){
    int code = INSIDE;
    if(x < xmin) code |= LEFT;
    else if(x > xmax) code |= RIGHT;
    if(y < ymin) code |= BOTTOM;
    else if(y > ymax) code |= TOP;
    return code;
}

bool cohenSutherlandClip(int &x0, int &y0, int &x1, int &y1, int xmin, int ymin, int xmax, int ymax){
    int out0 = computeOutCode(x0,y0,xmin,ymin,xmax,ymax);
    int out1 = computeOutCode(x1,y1,xmin,ymin,xmax,ymax);
    bool accept = false;
    while(true){
        if((out0 | out1) == 0){ // ambos dentro
            accept = true; break;
        } else if(out0 & out1){ // ambas fora no mesmo lado
            break;
        } else {
            // escolher ponto fora
            int outcodeOut = out0 ? out0 : out1;
            double x, y;
            if(outcodeOut & TOP){
                x = x0 + (double)(x1 - x0) * (ymax - y0) / (double)(y1 - y0);
                y = ymax;
            } else if(outcodeOut & BOTTOM){
                x = x0 + (double)(x1 - x0) * (ymin - y0) / (double)(y1 - y0);
                y = ymin;
            } else if(outcodeOut & RIGHT){
                y = y0 + (double)(y1 - y0) * (xmax - x0) / (double)(x1 - x0);
                x = xmax;
            } else { // LEFT
                y = y0 + (double)(y1 - y0) * (xmin - x0) / (double)(x1 - x0);
                x = xmin;
            }
            if(outcodeOut == out0){
                x0 = (int)std::round(x); y0 = (int)std::round(y);
                out0 = computeOutCode(x0,y0,xmin,ymin,xmax,ymax);
            } else {
                x1 = (int)std::round(x); y1 = (int)std::round(y);
                out1 = computeOutCode(x1,y1,xmin,ymin,xmax,ymax);
            }
        }
    }
    return accept;
}

// Helper: segment intersection between p1-p2 and p3-p4. Returns true if intersect and sets intersection point
static bool segSegIntersection(const IPoint &p1,const IPoint &p2,const IPoint &p3,const IPoint &p4, IPoint &out){
    double x1=p1.first, y1=p1.second;
    double x2=p2.first, y2=p2.second;
    double x3=p3.first, y3=p3.second;
    double x4=p4.first, y4=p4.second;
    double denom = (y4 - y3)*(x2 - x1) - (x4 - x3)*(y2 - y1);
    if(fabs(denom) < 1e-9) return false; // paralelas ou colineares
    double ua = ((x4 - x3)*(y1 - y3) - (y4 - y3)*(x1 - x3)) / denom;
    double ub = ((x2 - x1)*(y1 - y3) - (y2 - y1)*(x1 - x3)) / denom;
    if(ua >= 0.0 && ua <= 1.0 && ub >= 0.0 && ub <= 1.0){
        double xi = x1 + ua*(x2 - x1);
        double yi = y1 + ua*(y2 - y1);
        out.first = (int)std::round(xi);
        out.second = (int)std::round(yi);
        return true;
    }
    return false;
}

bool bruteForceClipSegment(const IPoint &a, const IPoint &b, const std::vector<IPoint> &poly, IPoint &outA, IPoint &outB){
    // Intersect segment with polygon edges; collect intersection points and also check inside/outside
    std::vector<IPoint> intersections;
    for(size_t i=0;i<poly.size();++i){
        IPoint p1 = poly[i];
        IPoint p2 = poly[(i+1)%poly.size()];
        IPoint ip;
        if(segSegIntersection(a,b,p1,p2,ip)) intersections.push_back(ip);
    }
    // If both endpoints inside polygon, keep them
    auto pointInPoly = [&](const IPoint &p)->bool{
        bool c=false; size_t n=poly.size();
        for(size_t i=0,j=n-1;i<n;j=i++){
            if( ((poly[i].second>p.second) != (poly[j].second>p.second)) &&
                (p.first < (poly[j].first-poly[i].first) * (p.second-poly[i].second) / (double)(poly[j].second-poly[i].second) + poly[i].first) )
                c = !c;
        }
        return c;
    };
    bool a_in = pointInPoly(a);
    bool b_in = pointInPoly(b);
    if(a_in && b_in){ outA = a; outB = b; return true; }
    if(intersections.empty()) return false;
    // sort unique intersections by parameter t along AB
    std::vector<std::pair<double,IPoint>> hits;
    double dx = b.first - a.first, dy = b.second - a.second;
    for(auto &ip: intersections){
        double t = (fabs(dx) > fabs(dy)) ? ((ip.first - a.first)/ (double)dx) : ((ip.second - a.second)/(double)dy);
        hits.push_back({t, ip});
    }
    std::sort(hits.begin(), hits.end(), [](auto &A, auto &B){ return A.first < B.first; });
    // determine clipped segment between entering and exiting intersections and possibly endpoint inside
    IPoint start, end; bool have=false;
    if(a_in){ start = a; have=true; }
    for(auto &h: hits){
        if(!have){ start = h.second; have=true; }
        else { end = h.second; // produce one segment and return
            outA = start; outB = end; return true;
        }
    }
    if(have && b_in){ outA = start; outB = b; return true; }
    return false;
}

bool cyrusBeckClip(const IPoint &a, const IPoint &b, const std::vector<IPoint> &convexPoly, IPoint &outA, IPoint &outB){
    // Parametric line P(t) = a + t*(b-a), t in [0,1]
    // For each edge i of convex polygon with outward normal n_i and point p_i on edge, compute t_i = (n_i dot (p_i - a)) / (n_i dot (b-a))
    // Maintain tE (enter) = 0 and tL (leave) = 1; update accordingly. If tE > tL reject.
    int n = (int)convexPoly.size();
    if(n < 3) return false;
    double tE = 0.0, tL = 1.0;
    double dx = b.first - a.first; double dy = b.second - a.second;
    for(int i=0;i<n;++i){
        IPoint p0 = convexPoly[i];
        IPoint p1 = convexPoly[(i+1)%n];
        // edge vector
        double ex = p1.first - p0.first;
        double ey = p1.second - p0.second;
        // outward normal for CCW polygon: n = (ey, -ex) (try to compute consistent normal)
        double nx = ey; double ny = -ex;
        double wx = a.first - p0.first; double wy = a.second - p0.second;
        double num = nx * (-wx) + ny * (-wy); // n dot (p_i - a) = - n dot (a - p_i)
        double den = nx * dx + ny * dy; // n dot (b-a)
        if(fabs(den) < 1e-9){
            if(num < 0) return false; // line parallel and outside
            else continue; // parallel and inside, no constraint
        }
        double t = num / den;
        if(den < 0){ // potential entering
            if(t > tE) tE = t;
        } else { // potential leaving
            if(t < tL) tL = t;
        }
        if(tE > tL) return false;
    }
    // compute clipped points
    double ax = a.first, ay = a.second;
    double bx = b.first, by = b.second;
    double cx = ax + tE * (bx - ax);
    double cy = ay + tE * (by - ay);
    double dxp = ax + tL * (bx - ax);
    double dyp = ay + tL * (by - ay);
    outA.first = (int)std::round(cx); outA.second = (int)std::round(cy);
    outB.first = (int)std::round(dxp); outB.second = (int)std::round(dyp);
    return true;
}
