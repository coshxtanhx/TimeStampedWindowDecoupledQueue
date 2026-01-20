#ifndef PRINT_H
#define PRINT_H

// This header is for compatibility with systems where std::print is not implemented.

#ifdef __cpp_lib_print
#include <print>
#endif
#include <iostream>
#include <format>

namespace compat {
	template<class... Args>
	void Print(std::format_string<Args...> fmt, Args&&... args)
	{
#ifdef __cpp_lib_print
		std::print(fmt, std::forward<Args>(args)...);
#else
		std::cout << std::format(fmt, std::forward<Args>(args)...);
#endif
	}
}

#endif