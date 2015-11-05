#pragma once

#include "global.h"

class Point;

class Point {
    public:
        float x, y, z;

        Point() {};
        Point(float x, float y, float z);
        Point(int x, int y, int z);
        Point(const rapidjson::Value &v);
        Point(const Point&);

        std::string debug() {
            char buff[512];
            sprintf(buff, "Point<%f, %f, %f>", x, y, z);
            std::string result = std::string(buff);
            return result;
        }
};

struct pointHashFunc {
    size_t operator()(const Point &k) const{
        size_t h1 = std::hash<float>()(k.x);
        size_t h2 = std::hash<float>()(k.y);
        size_t h3 = std::hash<float>()(k.z);
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

class BoundingBox {
    public:
        Point min, max;

        BoundingBox(Point, Point);

        bool contains(Point);
        bool contains(BoundingBox);
        bool intersects(BoundingBox);
};


