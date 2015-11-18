#pragma once

#include "util/geo.h"

Point::Point(double x, double y, double z) {
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


ProtoNet::IPoint* Point::to_proto() {
    ProtoNet::IPoint* point = new ProtoNet::IPoint;
    point->set_x(x);
    point->set_y(y);
    point->set_z(z);
    return point;
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

ProtoNet::IBoundingBox* BoundingBox::to_proto() {
    ProtoNet::IBoundingBox* box = new ProtoNet::IBoundingBox;
    box->set_allocated_min(this->min.to_proto());
    box->set_allocated_max(this->max.to_proto());
    return box;
}

uint64_t BoundingBox::size() {
    return ((max.x - min.x) *
            (max.y - min.y) *
            (max.z - min.z));
}

