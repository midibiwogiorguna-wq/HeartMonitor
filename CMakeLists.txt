
cmake_minimum_required(VERSION 3.21)

project(HeartMonitor VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${PROJECT_NAME} SHARED
    Src/main.cpp
)

setup_geode_mod(${PROJECT_NAME})
