cmake_minimum_required (VERSION 3.21)

if (USE_FMT)
	find_package(fmt QUIET)
endif()

# 指定生成目标
add_executable(
	IncrementDiff
		main.cpp
		Commands.cpp
)

if (fmt_FOUND)
	message("=> Using fmt as formatting library")
	set(FMT_LINK_LIB fmt::fmt)
	target_compile_definitions(
		IncrementDiff
			PUBLIC FMT_LIB
	)
else()
	if (USE_FMT)
		message("=> fmt not found, fallback to std")
	else()
		message("=> Using std::format")
	endif()
endif()

target_link_libraries(
	IncrementDiff
		${FMT_LINK_LIB}
		picosha2
		CLI11::CLI11
)
