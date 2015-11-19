#include "terra/blocktype.h"

namespace Terra {

void BlockTypeHolder::loadBaseTypes() {
    this->addBlockType(new Terra::AirType());
    this->addBlockType(new Terra::BedRockType());
}

void BlockTypeHolder::addBlockType(Terra::BlockType* type) {
    if (this->types.count(type->name)) {
        WARN("addBlockType replacing type %s", type->name.c_str());
    }

    // Give this blocktype an ID which will be used for networking blocks
    this->id_inc++;
    type->id = this->id_inc;

    this->cache[type->id] = type;
    this->types[type->name] = type;
}

void BlockTypeHolder::rmvBlockType(std::string type) {
    this->types.erase(type);
}

bool BlockTypeHolder::haveType(std::string type) {
    return (this->types.count(type) == 1);
}

BlockType* BlockTypeHolder::getBlockType(std::string type) {
    return this->types[type];
}

}
