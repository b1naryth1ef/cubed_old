#pragma once

#include "global.h"

class Point;

class Point {
    public:
        double x, y, z;

        Point(double x, double y, double z);
        Point(int x, int y, int z);
        Point(const rapidjson::Value &v);
        Point() {};

        Point *copy() {
            return new Point(x, y, z);
        }

        std::string debug() {
            char *x;
            sprintf(x, "Point<%F, %F, %F>", x, y, z);
            std::string result = std::string(x);
            free(x);
            return result;
        }
};

struct pointHashFunc {
    size_t operator()(const Point &k) const{
        size_t h1 = std::hash<double>()(k.x);
        size_t h2 = std::hash<double>()(k.y);
        size_t h3 = std::hash<double>()(k.z);
        return (h1 ^ (h2 << 1)) ^ h3;
    }
};

struct pointEqualsFunc {
  bool operator()( const Point& lhs, const Point& rhs ) const {
    return (lhs.x == rhs.x) && (lhs.y == rhs.y) && (lhs.z == rhs.z);
  }
};

typedef std::vector<Point*> PointPV;
typedef std::vector<Point> PointV;


