cmake_minimum_required(VERSION 3.22.1)

project("mediainfolib_example_cmake")

find_package(mediainfolib REQUIRED CONFIG)

add_library(mediainfolib_example_bridge SHARED 
    mediainfolib_example_bridge.cpp
)

target_link_libraries(mediainfolib_example_bridge 
    mediainfolib::mediainfo
)
