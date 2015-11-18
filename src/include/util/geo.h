#pragma once

#include "global.h"
#include "packet.pb.h"

class Point;

class Point {
    public:
        double x, y, z;

        Point() {};
        Point(double x, double y, double z);
        Point(int x, int y, int z);
        Point(const rapidjson::Value &v);
        Point(const Point&);

        std::string debug() {
            char buff[512];
            sprintf(buff, "Point<%f, %f, %f>", x, y, z);
            std::string result = std::string(buff);
            return result;
        }

        ProtoNet::IPoint* to_proto();
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

class BoundingBox {
    public:
        Point min, max;

        BoundingBox(Point, Point);

        bool contains(Point);
        bool contains(BoundingBox);
        bool intersects(BoundingBox);

        ProtoNet::IBoundingBox* to_proto();

        uint64_t size();

        uint64_t sizeX() {
            return pow((max.x - min.x), 2);
        }

        uint64_t sizeY() {
            return pow((max.y - min.y), 2);
        }

        uint64_t sizeZ() {
            return pow((max.z - min.z), 2);
        }
};



