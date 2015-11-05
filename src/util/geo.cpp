#pragma once

#include "util/geo.h"

Point::Point(float x, float y, float z) {
    this->x = x;
    this->y = y;
    this->z = z;
}

Point::Point(int x, int y, int z) {
    this->x = x;
    this->y = y;
    this->z = z;
}

Point::Point(const rapidjson::Value &v) {
    x = v[rapidjson::SizeType(0)].GetInt();
    y = v[rapidjson::SizeType(1)].GetInt();
    z = v[rapidjson::SizeType(2)].GetInt();
}


Point::Point(const Point& p) {
    this->x = p.x;
    this->y = p.y;
    this->z = p.z;
}


BoundingBox::BoundingBox(Point min, Point max) {
    this->min = min;
    this->max = max;
}

bool BoundingBox::intersects(BoundingBox other) {
    return (
        ((other.min.x <= max.x) && (other.max.x >= min.x)) &&
        ((other.min.y <= max.y) && (other.max.y >= min.y)) &&
        ((other.min.z <= max.z) && (other.max.z >= min.z))
    );
}

bool BoundingBox::contains(Point p) {
    return (
       ((p.x >= min.x) && (p.x <= max.x)) &&
       ((p.y >= min.y) && (p.y <= max.y)) &&
       ((p.z >= min.z) && (p.z <= max.z))
    );
}

bool BoundingBox::contains(BoundingBox other) {
    return (this->contains(other.min) && this->contains(other.max));
}
