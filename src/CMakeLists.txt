# CMake 最低版本号要求
cmake_minimum_required (VERSION 3.11)

if (USE_FMT)
	# 指定这个变量的时候才尝试寻找fmt
	find_package(fmt)
endif()

# 指定生成目标
add_executable(
	IncrementDiff
		main.cpp
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
		cryptopp::cryptopp
)