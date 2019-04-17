cmake_minimum_required(VERSION 3.0.2)

project(protobuf)

set(version 2.6.1)
set(srcdir ${CMAKE_CURRENT_SOURCE_DIR}/../src)
set(protobuf_include_dir ../src)
set(CMAKE_AUTOMOC OFF)

file(GLOB_RECURSE proto_h ${CMAKE_CURRENT_SOURCE_DIR}/../src/google/protobuf/*.h)
file(GLOB_RECURSE proto_src ${CMAKE_CURRENT_SOURCE_DIR}/../src/google/protobuf/*.cc)

include_directories(${srcdir})

if(MSVC)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../vsprojects)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS=1 /wd4244 /wd4267 /wd4018 /wd4355 /wd4800 /wd4251 /wd4996 /wd4146 /wd4305)
else()
    add_definitions(-DHAVE_CONFIG_H)
    add_definitions(-Wno-deprecated)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config_linux.h ${CMAKE_CURRENT_BINARY_DIR}/config.h)
    include_directories(${CMAKE_CURRENT_BINARY_DIR})
endif()

add_library(Protobuf STATIC ${proto_src} ${proto_h})



project(protoc)

file(GLOB_RECURSE comp_h ${CMAKE_CURRENT_SOURCE_DIR}/../src/google/protobuf/compiler/*.h)
file(GLOB_RECURSE comp_src ${CMAKE_CURRENT_SOURCE_DIR}/../src/google/protobuf/compiler/*.cc)

if(MSVC)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../vsprojects)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS=1 /wd4244 /wd4267 /wd4018 /wd4355 /wd4800 /wd4251 /wd4996 /wd4146 /wd4305)
else()
    add_definitions(-DHAVE_CONFIG_H)
    add_definitions(-Wno-deprecated)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config_linux.h ${CMAKE_CURRENT_BINARY_DIR}/config.h)
    include_directories(${CMAKE_CURRENT_BINARY_DIR})
endif()

include_directories(${srcdir})

link_libraries(Protobuf)

add_executable(Protoc ${comp_src} ${comp_h})
