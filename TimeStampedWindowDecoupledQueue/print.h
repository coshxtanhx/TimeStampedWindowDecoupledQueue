#ifndef PRINT_H
#define PRINT_H

// This header is for compatibility between g++ and MSVC compilers.

#include <iostream>
#include <format>

#ifndef __cpp_lib_print
#define print cout << std::format
#else
#include <print>
#endif

#endif