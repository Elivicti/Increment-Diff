

#include <fmt/core.h>
#include <fmt/ranges.h>

#include <CLI/CLI.hpp>

#include <array>
#include <filesystem>
#include <fstream>


#include "../include/Commands.hpp"

int main(int argc, const char** argv)
{
	CLI::App app{ "description" };

	HashCommand hash{ app.add_subcommand("hash", "generate hash") };
	MakeIncrementDiff makediff{ app.add_subcommand("make", "make diff from hash") };

	app.require_subcommand(1);

	// std::string output;
	// app.add_option("-o,--output", output, "description");

	CLI11_PARSE(app, argc, argv);

	return 0;
}

