cmake_minimum_required(VERSION 3.30)
project(rep64)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)

include_directories(../libdragon/include)
include_directories(../tiny3d/src)

include_directories(src)

add_executable(rep64
        src/main.cpp
        src/text.cpp
        src/miMemory.cpp
        src/demos/Repeat.cpp
        src/demos/Sync.cpp
        src/rdp/rdp.h
        src/rdp/rdp.cpp
        src/demos/VI.cpp
        src/demos/VIPong.cpp
        src/demoList.h
        src/demos/HiddenBits.cpp
        src/demos/HiddenBits.cpp
        src/demos/RDPTestMode.cpp)

set_property(TARGET rep64 PROPERTY CXX_STANDARD 23)