#pragma once

#if defined(FMT_LIB)

#include <fmt/ostream.h>
namespace util
{
	using fmt::format;
}
#define FMT_NS fmt
#else

#include <format>
#include <iostream>
namespace util
{
	using std::format;

	template <typename... Args>
	void print(std::ostream& os, std::format_string<Args...> fmt, Args&&... args)
	{
		std::format_to(std::ostreambuf_iterator{ os }, fmt, std::forward<Args>(args)...);
	}
	template <typename... Args>
	void print(std::format_string<Args...> fmt, Args&&... args)
	{
		print(std::cout, fmt, std::forward<Args>(args)...);
	}
}
#define FMT_NS std
#endif


