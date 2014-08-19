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

bool DB::create() {
    int res;

    for (auto& kv : tables) {
        res = sqlite3_exec(db, kv.second.c_str(), 0, 0, 0);

        if (res != SQLITE_OK) {
            LOG.L("Error creating table %s", kv.first.c_str());
            throw Exception("Error creating table!");
        }
    }
}

void DB::add_table(std::string name, std::string sql, bool create) {
    tables[name] = sql;

    if (create) {
        int res = sqlite3_exec(db, sql.c_str(), 0, 0, 0);

        if (res != SQLITE_OK) {
            LOG.L("Error creating table %s on add", name.c_str());
            throw Exception("Error creating table on add!");
        }
    }
}

bool DB::drop_all() {
    int res = sqlite3_exec(db,
        "PRAGMA writable_schema = 1;"
        "delete from sqlite_master where type = 'table';"
        "PRAGMA writable_schema = 0;"
        "VACUUM;"
        "PRAGMA INTEGRITY_CHECK;", 0, 0, 0);

    if (res != SQLITE_OK) {
        LOG.L("Error dropping all tables!");
        throw Exception("Error dropping all tables!");
    }
}

// Should be called on startup, makes sure SQLite is configured correctly
void init_db_module() {
    sqlite3_config(SQLITE_CONFIG_SERIALIZED);
}