#ifndef PRINT_H
#define PRINT_H

//This header is for compatibility between g++ and MSVC compilers.

#include <iostream>
#include <format>

#ifndef __cpp_lib_print
#define print(fmt, ...) cout << std::format(fmt, __VA_ARGS__)
#define println(fmt, ...) cout << (std::format(fmt, __VA_ARGS__) + "\n")
#else
#include <print>
#endif

#endif