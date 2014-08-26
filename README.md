

# Types

- Client
- Server

- Entity
    - Player

- Material

- Block
    - StaticBlock
    - DynamicBlock
    - LiveBlock (+entity)

- World
    - Realm
        - Region [(128 x 128 x 128) cube of block data]
            - Block  

# Serilization
Protobuf for both network and on-disk data


## World Format
Directory Structure:
    - world.json (contains name, version, etc)
    - data.db (contains block data/etc, should be hugenormous)
    - cache/ (contains block caches)

### world.json
Structure:
```
{
    "name": "my awesome world",
    "version": 1,
}
```

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