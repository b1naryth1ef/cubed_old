project(cubed)
cmake_minimum_required (VERSION 3.2)

# Set git hash
execute_process(
  COMMAND git rev-parse HEAD
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  RESULT_VARIABLE GIT_EXECUTED
  OUTPUT_VARIABLE CUBED_GIT_HASH
)
string(STRIP ${CUBED_GIT_HASH} CUBED_GIT_HASH)
if (NOT (GIT_EXECUTED EQUAL 0))
  message(FATAL_ERROR "Could not identifiy git commit id")
endif()

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
target_link_libraries(cubed sqlite3 lua dl protobuf pthread sodium SDL2 GL curl proto rocksdb)
