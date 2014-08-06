

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