#pragma once

#include "mod.h"

using namespace rapidjson;

bool ModDex::loadFromPath(std::string dir) {
    DEBUG("Loading mod from path %s", dir.c_str());
    Mod result;

    FILE *fp = fopen(ioutil::join(dir, "mod.json").c_str(), "r");

    if (!fp) {
        ERROR("Invalid mod directory, mod.json does not exist!");
        throw Exception("Invalid Mod: no mod.json exists!");
    }

    char readBuffer[65536];
    FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    Document d;

    try {
        d.ParseStream(is);
    } catch (std::string e) {
        ERROR("Error occured parsing the mod.json file: %s", e.c_str());
        throw Exception("Error occured parsing the mod.json file!");
    }

    result.name = d["name"].GetString();
    result.author = d["author"].GetString();
    result.version = d["version"].GetInt();
    result.path = dir;

    if (this->get(result.name) != nullptr) {
        ERROR("Mod with name %s already loaded!", result.name.c_str());
        throw Exception("Mod with same name already loaded!");
    }

    this->mods[result.name] = &result;
    result.compile();

    fclose(fp);

    return true;
}

Mod *ModDex::get(std::string k) {
    if (this->mods.count(k)) {
        return this->mods[k];
    }

    return nullptr;
}

void Mod::compile() {
    DEBUG("Attempting to compile mod %s", this->name.c_str());
    int res = luaL_loadfile(L, ioutil::join(this->path, "main.lua").c_str());
    if (res != 0) {
        ERROR("Failed to compile file %s!", ioutil::join(this->path, "main.lua").c_str());
        return;
    }

}