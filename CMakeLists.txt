project(cubed)
cmake_minimum_required (VERSION 3.2)

# C++11
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# Protobuf
aux_source_directory(proto/gen PB_SOURCES)
add_library(proto STATIC ${PB_SOURCES})
target_link_libraries(proto protobuf)
message(STATUS "${PROTOBUF_LIBRARY}")

# Main C++ files
include_directories(proto/gen)
include_directories(src/include/)
FILE(GLOB_RECURSE CORE_SOURCES src/*.cpp)
# aux_source_directory(src/ CORE_SOURCES)

message(STATUS "${CORE_SOURCES} ${PB_SOURCES}")
add_executable(cubed ${CORE_SOURCES})
target_link_libraries(cubed sqlite3 lua dl protobuf pthread sodium SDL2 GL curl terra proto)
