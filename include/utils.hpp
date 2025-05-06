#pragma once

#if defined(FMT_LIB)

#include <fmt/ostream.h>
namespace util
{
	using fmt::format;
	using fmt::print;
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
		::util::print(std::cout, fmt, std::forward<Args>(args)...);
	}
}
#define FMT_NS std
#endif

namespace util
{
	template<typename ExcepT, typename ...Args>
	ExcepT make_exception(FMT_NS::format_string<Args...> fmt, Args&&... args)
	{
		return ExcepT{ ::util::format(fmt, std::forward<Args>(args)...) };
	}
}