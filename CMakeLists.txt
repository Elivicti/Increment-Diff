cmake_minimum_required (VERSION 3.21)

include(FetchContent)

FetchContent_Declare(
	picosha2
	GIT_REPOSITORY git@github.com:okdshin/PicoSHA2
	GIT_TAG        v1.0.1
)
FetchContent_MakeAvailable(picosha2)

FetchContent_Declare(
    CLI11
    GIT_REPOSITORY git@github.com:CLIUtils/CLI11.git
    GIT_TAG v2.4.2
)
FetchContent_MakeAvailable(CLI11)

include_directories(${CLI11_SOURCE_DIR}/include)

project(IncrementDiff)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(USE_FMT "Determines if fmtlib is used to format strings" OFF)

include_directories(include)
add_subdirectory(src)


if (PROJECT_IS_TOP_LEVEL)
	# copy compile_commands.json to build
	execute_process(
		COMMAND ${CMAKE_COMMAND} -E copy
			${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json
			${CMAKE_EXPORT_COMPILE_COMMANDS_DIR}/compile_commands.json
	)
endif()
