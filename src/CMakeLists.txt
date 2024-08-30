# CMake 最低版本号要求
cmake_minimum_required (VERSION 3.11)


message("${CMAKE_SOURCE_DIR}")


find_package(fmt REQUIRED)

# 指定生成目标
add_executable(
	IncrementDiff
		main.cpp
)
target_link_libraries(
	IncrementDiff
		fmt::fmt
		cryptopp::cryptopp
)