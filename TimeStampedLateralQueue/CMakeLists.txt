cmake_minimum_required(VERSION 3.10)
project(TimeStampedLateralQueue)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    main.cpp
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast")

add_executable(TimeStampedLateralQueue ${SOURCES})