cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
include(ExternalProject)

find_program(GIT git)
find_program(ANT ant)

if( NOT ANT )
  message( FATAL_ERROR "ant is required to build zookeeper module" )
endif()

project(zookeeper C)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Find Tarantool
set(TARANTOOL_FIND_REQUIRED ON)
find_package(Tarantool)

# Prepare zookeeper library
set(ZOOKEEPER_LIB_HOME "${PROJECT_SOURCE_DIR}/third_party/zookeeper")

# Get git version only if source directory has .git repository, this
# avoids git to search .git repository in parent
# directories.
#
if (EXISTS "${PROJECT_SOURCE_DIR}/.git" AND GIT)
    execute_process(COMMAND ${GIT} submodule update --init --recursive)
endif()

foreach(f generated/zookeeper.jute.h generated/zookeeper.jute.c)
    if(NOT EXISTS "${ZOOKEEPER_LIB_HOME}/src/c/${f}")
        execute_process(COMMAND ${ANT} compile_jute
                        WORKING_DIRECTORY ${ZOOKEEPER_LIB_HOME})
        break()
    endif()
endforeach()

add_subdirectory(${ZOOKEEPER_LIB_HOME}/src/c)

set(ZOOKEEPER_HOME "${ZOOKEEPER_LIB_HOME}")
set(Zookeeper_FOUND_REQURIRED ON)
find_package(Zookeeper)
message(STATUS "Zookeeper_INCLUDE_DIRS is ${Zookeeper_INCLUDE_DIRS}")

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
