#include "world.h"


WorldFile::WorldFile(std::string dir) {
    directory = dir;
}

WorldFile::~WorldFile() {
    close();
}

bool WorldFile::open() {
    fp = fopen(ioutil::join(this->directory, "world.json").c_str(), "r");

    if (!fp) {
        LOG.L("Failed to open world file!");
        throw Exception("Failed to open world file!");
    }

    // Create a lock for the file, which warns other instances of cubed to not write to
    //  this world file.
    int lock_result = flock(fileno(fp), LOCK_EX | LOCK_NB);
    if (lock_result != 0) {
        LOG.L("Error getting lock: %i", lock_result);

        throw Exception("Error occured getting lock for world file!");
    }

    // Read the entire json document
    char readBuffer[65536];
    FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    Document d;
    d.ParseStream(is);

    // Parse the name and version
    name = d["name"].GetString();
    version = d["version"].GetInt();
    LOG.L("Name and version: %s, %i", name.c_str(), version);

    // Check the version is valid
    if (version < SUPPORTED_WORLD_FILE_VERSION) {
        throw Exception("World file version is older, would need to convert it!");
    } else if (version > SUPPORTED_WORLD_FILE_VERSION) {
        throw Exception("World file version is newer than the supported version,\
            this is a fatal error!");
    }
}

bool WorldFile::close() {
    if (!fp) { return false; }

    // Unlock the file and close it
    LOG.L("Closing world file %s", name.c_str());
    flock(fileno(fp), LOCK_UN);
    fclose(fp);

    return true;
}
