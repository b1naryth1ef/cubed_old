# Cubed
Cubed is a slightly ambitious project to create an open-source Minecraft-like game that aims to be efficient, extremely modular and extend-able. It was built to help nuture and grow a modding community around Minecraft, and is inspired by projects like Forge and Bukkit. Cubed has the following major goals:

- To create a simple, extensive, fast, and solidified modding API in Lua, allowing for the creation of entirely new games and nurturing the exploration of ideas within the engine and game.
- To create a abstracted, intelligent and modern C++ game engine, without any obvious attachments to game logic or rules.
- To create and distribute a "vanilla mod" that resembles a near-match to the vanilla minecraft experience.

Cubed is in a sudo-state of development and experimentation right now, and welcomes contributors that want to get their feet wet in early stages of planning and implementation.

## Building/Running/Testing
Cubed aims for an extremely simple interface to compiling and running, and uses a custom python build script to achieve this. We currently only target Linux, with the goal of creating Mac/Windows versions at a later day. To get cubed running on your *nix like box, you can run the following commands from the source directory:

- `./build.py setup` (installs dependencies, headers, etc, only needs to be done once)
- `./build.py run [server|client]` builds and runs the server or client

You can also find compiled binaries for 64bit linux on the releases page, although these are mostly just snapshots of previous development versions. Cubed uses C++11, and thus you'll need a GCC compiler past 4.7 to build it properly.

## Compatibility
Cubed has no goals to remain compatible with any portions of the minecraft ecosphere, however at some point the goal is to implement a minecraft -> cubed world converter.

## Contributing
Read the source, understand the format, fork, edit, PR, profit. Eventually we will put together a style guide.

## Infrastructure

### Networking
Cubed uses the awesome NaCL library, wrapped with libsodium, for it's encryption and signing. Cubed was purposely designed to be secure, smart, and distributable. Cubed encrypts almost all (sans handshaking, and public-status) packets with Curve25519 public-key encryption. By default, a user is only represented by a username and public key.

Cubed allows the use of multiple, and third-party login servers, putting the weight of username validation/management on the community. The provided login-server implementation is available as [hackle](https://github.com/cubed/hackle).

#### Authentication

1. Client sends PacketHello to server, this is UNENCRYPTED and SIGNED payload which contains some generic information, and this clients public-key/username pair along with the current session ID from the login server.
2. Server contacts the login server with the public-key, username, and session ID. If the session server validates this information, the server sends back a PacketStart which is now ENCRYPTED and nounced! All further packets must be fully encrypted (signing is assumed from the encryption). If the login server denies the validity of the users content, the server will immediately send a PacketDisconnect with the valid error-code and message.

#### Packets

- `PacketStatus`, sent from the client to the server for general server listing
- `PacketHello`, sent from the client to the server for handshaking
- `PacketDisconnect`, bidirectional, signals the immediate end of this networking stream/session
- `PacketStart`, sent from server to client, start of nounce's, world/server info
- `PacketBlocks`, sent from server to client, contains positionally tagged blocks
- `PacketBlockRegion`, sent from the server to client, contains polygonal information and blocks in the polygon
- `PacketBlockUpdate`, sent from server to client, specifies an individual block change


### Worlds
A world in cubed is represented by a folder containing two items. The first is a world-definition file specifying a few basics about the world, like the world name and version. The second is a sqlite database containing all the data required for the world to be loaded. Worlds can be loaded at anytime, either during startup or while the cubed server is running. World's allow both sync and async block loading from disk allowing for fairly good performance on loading large chunks from the world, although there are still areas that need improvement. On disk, around 2 million blocks equates to 60 or so MB, and that takes about 50 seconds to load async.

#### World Definition File
The world definition file contains the following three fields:

1. "name", a lexical name for the world
2. "version" an integer version of the world relating to the CUBED_WORLD_FORMAT_VERSION
3. "origin", a start point of the world (AKA spawn point)

#### World Database File
The world DB holds three major tables relating to the world

1. "blocks" a table of (x, y, z) and type for each block
2. "blocktypes" a table of block type ID's to block type names, used for managing mod loading/unloading when blocktypes are involved, and for faster operations on the blocks db
3. "blockdata" a table of block data (to be designed/specified)

### Server
The server represents a base class that handles the runtime environment of a server. It handles loaded worlds, modules, and everything else in between. Servers operate at a constant tickrate that can be set at startup, and have a single primary loop which updates everything in the server (which operates at said tickrate).

# B1nz todos
- Implement the encryption protocol, authentication protocol, etc.
- Write a complete installation/building guide
- Write client gl context shit
- Implement loading cvars from JSON files.
- Design the "extra-data" system for blocks/entities/etc. What format? etc.
- Start working on plugin implementation
