cmake_minimum_required(VERSION 3.10)
project(TimeStampedLateralQueue)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    main.cpp benchmark_tester.cpp
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast")
add_compile_options(-Wno-interference-size -Wno-volatile)

add_executable(TSLQ ${SOURCES})