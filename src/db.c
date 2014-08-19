#pragma once

#include "db.h"

DB::DB(std::string path) {
    int rc = sqlite3_open(path.c_str(), &db);
    if (rc) {
        throw Exception("Failed to open database file: " + rc);
    }
}

DB::~DB() {
    sqlite3_close(db);
}

// Should be called on startup, makes sure SQLite is configured correctly
void init_db_module() {
    sqlite3_config(SQLITE_CONFIG_SERIALIZED);
}