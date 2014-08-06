

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
    - light.cache
    - regions/
        - $UUID.reg (region file which contains a header of )

### world.json
Structure:
```
{
    "name": "my awesome world",
    "version": 1,
    "regions": {
        "$UUID": [x, y, z]
    }
}
```

### Region Files
[0,0,0], [0,1,0]
[1,0,0], [1,1,0]
...
[128,0,0],[128,1,0]
[0,0,1],[0,1,1]
...
[0,0,128],[0,1,128]
[1,0,1],[1,1,1]
...
[128,0,128],[128,1,128]


# Requirements
- https://github.com/miloyip/rapidjson

