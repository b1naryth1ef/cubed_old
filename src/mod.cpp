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
    result.dex = this;

    if (this->get(result.name) != nullptr) {
        ERROR("Mod with name %s already loaded!", result.name.c_str());
        throw Exception("Mod with same name already loaded!");
    }

    this->mods[result.name] = &result;
    result.load();

    fclose(fp);

    return true;
}

Mod *ModDex::get(std::string k) {
    if (this->mods.count(k)) {
        return this->mods[k];
    }

    return nullptr;
}

bool ModDex::eventPre(Event *e) {
    DEBUG("Would fire pre on event %s", e->type.c_str());
}

bool ModDex::eventPost(Event *e) {
    DEBUG("Would fire post on event %s", e->type.c_str());
}

void Mod::load() {
    int res;
    DEBUG("Attempting to load mod %s", this->name.c_str());

    Event *e = new Event("mod_load");
    e->data->setString("name", this->name);
    e->data->setInt("version", this->version);
    this->dex->eventPre(e);

    if (e->cancelled) {
        WARN("mod_load event for %s was cancelled, will not load.");
        return;
    }

    res = luaL_dofile(L, ioutil::join(this->path, "main.lua").c_str());
    if (res != 0) {
        ERROR("Failed to exceute file %s, %i!", ioutil::join(this->path, "main.lua").c_str(), res);
        return;
    }

    // Now we call the on_load method of the plugin, every plugin should have this
    lua_getglobal(L, "on_load");
    res = lua_pcall(L, 0, 0, 0);
    if (res != 0) {
        ERROR("Failed to call on_load for the mod %i!", res);
        return;
    }

    this->dex->eventPost(e);
    DEBUG("Mod %s has been loaded!", this->name.c_str());
}
