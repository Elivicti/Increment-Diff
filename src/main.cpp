#include <CLI/CLI.hpp>

#include "Commands.hpp"

int main(int argc, const char** argv)
{
	CLI::App app{ "description" };

	HashCommand hash{ app.add_subcommand("hash", "generate hash") };
	MakeIncrementDiff makediff{ app.add_subcommand("make", "make diff from hash") };

	app.require_subcommand(1);

	try
	{
		app.parse(argc, argv);
	}
	catch(const CLI::ParseError& e)
	{
		return app.exit(e);
	}
	catch(const std::runtime_error& e)
	{
		util::print(std::cerr, "{}\n", e.what());
		util::print(std::cerr, "Run with --help for more information.\n");
		return 1;
	}
	return 0;
}

