#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <exception>
#include <map>

#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <stdint.h>
#include <sys/file.h>

// Json
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/filereadstream.h"

// Sqlite3
#include <sqlite3.h> 

#include "log.h"

#define MULTI_LINE_STRING(a) #a

static Log LOG("cubed.log", true);
static int CUBED_VERSION = 1;
