cmake_minimum_required(VERSION 3.10)
project(TimeStampedLateralQueue)

set(CMAKE_CXX_STANDARD 17)

set(SOURCES
    main.cc
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast")

add_executable(TimeStampedLateralQueue ${SOURCES})