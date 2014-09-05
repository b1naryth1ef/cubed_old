#pragma once

#include "geo.h"

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