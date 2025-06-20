#ifndef PRINT_H
#define PRINT_H

// This header is for compatibility with systems where std::print is not implemented.

#include <iostream>
#include <format>

#ifdef __cpp_lib_print
#include <print>
#else
#define print cout << std::format
#endif

#endif