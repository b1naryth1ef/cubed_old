#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <exception>
#include <map>
#include <mutex>
#include <thread>
#include <chrono>
#include <queue>
#include <functional>
#include <unordered_map>
#include <sstream>

#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <stdint.h>
#include <sys/file.h>
#include <signal.h>
#include <unistd.h>

// Json
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/prettywriter.h"
// #include "rapidjson/filestream.h"

// Sqlite3
#include <sqlite3.h>

#include "util/log.h"

#define MULTI_LINE_STRING(a) #a

static Log LOG;

static int CUBED_RELEASE_A = 0;
static int CUBED_RELEASE_B = 0;
static int CUBED_RELEASE_C = 4;

static int CUBED_VERSION = CUBED_RELEASE_C >> CUBED_RELEASE_B >> CUBED_RELEASE_A;

#define DEBUG(x, ...)  LOG.L("DEBUG", x, ## __VA_ARGS__)
#define INFO(x, ...)   LOG.L("INFO ", x, ## __VA_ARGS__)
#define WARN(x, ...)   LOG.L("WARN ", x, ## __VA_ARGS__)
#define ERROR(x, ...)  LOG.L("ERROR", x, ## __VA_ARGS__)
#define THREAD(...) THREAD_POOL.push_back(new std::thread(__VA_ARGS__))

#ifndef CUBED_GIT_HASH
    #define CUBED_GIT_HASH ""
#endif

static std::vector<std::thread *> THREAD_POOL;

class Client;
class Server;

static Server *s = nullptr;
static Client *c = nullptr;

#define MAX_UINT32 2147483647
