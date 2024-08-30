# CMake 最低版本号要求
cmake_minimum_required (VERSION 3.11)

include(FetchContent)

FetchContent_Declare(
    cryptopp-cmake
    GIT_REPOSITORY https://mirror.ghproxy.com/https://github.com/abdes/cryptopp-cmake.git
    GIT_TAG CRYPTOPP_8_9_0
)
FetchContent_MakeAvailable(cryptopp-cmake)

FetchContent_Declare(
    CLI11
    GIT_REPOSITORY https://mirror.ghproxy.com/https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.4.2
)
FetchContent_MakeAvailable(CLI11)

include_directories(${CLI11_SOURCE_DIR}/include)

# 项目信息
project(IncrementDiff)

set(CMAKE_CXX_STANDARD 20 REQUIRED)

add_subdirectory(src)
