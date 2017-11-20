cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

project(zookeeper C)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Find Tarantool
set(TARANTOOL_FIND_REQUIRED ON)
find_package(Tarantool)

# Find Zookeeper
find_package(Zookeeper)
message(STATUS "Zookeeper_INCLUDE_DIRS is ${Zookeeper_INCLUDE_DIRS}")
message(STATUS "Zookeeper_LIBRARIES is ${Zookeeper_LIBRARIES}")

# Proceeding

include_directories(${TARANTOOL_INCLUDE_DIRS})
include_directories(${Zookeeper_INCLUDE_DIRS})

# Set CFLAGS
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -O2")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fPIC -O2 -Wall -Wextra")

if (APPLE)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -undefined suppress -flat_namespace")
endif(APPLE)

# Build module
add_subdirectory(zookeeper)
