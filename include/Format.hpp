#pragma once

#if defined(FMT_LIB)

#include <fmt/format.h>
namespace util
{
	using fmt::format;
}
#define FMT_NS fmt
#else

#include <format>
namespace util
{
	using std::format;
}
#define FMT_NS std
#endif


