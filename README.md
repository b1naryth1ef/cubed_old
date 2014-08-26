

## Worlds
A world in cubed is represented by a folder containing two items. The first is a world-defintion file specifiying a few basics about the world, like the world name and version. The second is a sqlite database containing all the data required for the world to be loaded. Worlds can be loaded at anytime, either during startup or while the cubed server is running.

### World Definition File
The world definition file contains the following three fields:

1. "name", a lexical name for the world
2. "version" an integer version of the world relating to the CUBED_WORLD_FORMAT_VERSION
3. "origin", a start point of the world (AKA spawn point)


### World Database File
The world DB holds three major tables relating to the world

1. "blocks" a table of (x, y, z) and type for each block
2. "blocktypes" a table of block type ID's to block type names, used for managing mod loading/unloading when blocktypes are involved, and for faster operations on the blocks db
3. "blockdata" a table of block data (to be designed/specified)


# Networking Loop
- Accept clients
- Dump clients (ServiceClient) into a vector inside Service
- epoll on all the ServiceClient fd's, read data as Packet into a queue
- Server iterates over the queue and parses messages one by one, special code to transalate to JSON if needed?

# Threads
- Server Main Loop
- Networking Thread
- Tick Update Thread


# Requirements
- https://github.com/miloyip/rapidjson



## B1nz todos
- Design/Implement some kind of non horrific cvar system. Dump to json, have read/write permissions, defaults, all that q3 sex without the uglyness of the files.
- Implement client/server running inside build.py/main.cpp
- Start working on plugin implementation

## Lua API Questions
- How can I define a import path of sorts? luaL_requiref??