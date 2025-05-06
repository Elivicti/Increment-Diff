#include "Commands.hpp"

HashCommand::HashCommand(CLI::App* app_)
	: CliSubcommand{ app_ }
{
	app->add_option("-o,--output", output, "output file, if not specified, output to stdout");

	app->add_option("directory", directory, "path to directory that needs to compute hash")
		->check(CLI::ExistingDirectory)
		->required(true);

	app->add_option("-c,--compare", compare_hash, "previous hash to compare")
		->check(CLI::ExistingFile);
}

std::set<FileNode> HashCommand::get_file_nodes(
	const std::filesystem::path& directory,
	std::optional<std::filesystem::path> compare_hash
)
{
	std::set<FileNode> files;
	if (compare_hash)
	{
		if (!std::filesystem::is_regular_file(compare_hash.value()))
			throw std::invalid_argument{ "" };

		std::fstream fs{ compare_hash.value(), std::ios::in };
		std::string hash, status, path;
		while (fs >> hash >> status)
		{
			fs.ignore(1); // ignore space
			std::getline(fs, path); // path may contain spaces

			files.emplace(directory / path, hash);
		}
	}

	for (auto& entry : std::filesystem::recursive_directory_iterator{ directory })
	{
		if (entry.is_directory())
			continue;
		auto [it, success] = files.emplace(entry.path(), Sha256{});
	}
	return files;
}


void HashCommand::operator()()
{
	namespace stdfs = std::filesystem;

	stdfs::path dir{ directory };
	std::set<FileNode> files = get_file_nodes(
		directory,
		compare_hash.empty()
			? std::nullopt
			: std::make_optional(compare_hash)
	);

	if (output.empty())
	{
		for (auto& file : files)
		{
			util::print("{}\n", file.to_string(dir));
		}
		return;
	}

	std::fstream fs{ output, std::ios::out | std::ios::trunc };
	for (auto& file : files)
	{
		util::print(fs, "{}\n", file.to_string(dir));
	}
}


void MakeIncrementDiff::operator()()
{
	namespace stdfs = std::filesystem;
	stdfs::path input_dir{ directory };
	stdfs::path output_dir{ output };

	std::vector<stdfs::path> modified_files;
	std::vector<std::string> deleted_files;

	if (!compare_hash.empty())
	{
		auto files = HashCommand::get_file_nodes(input_dir, compare_hash);
		for (auto& file : files)
		{
			auto status = file.status();
			if (status == FileNode::NotChanged)
				continue;

			if (status == FileNode::Deleted)
			{
				deleted_files.emplace_back(file.file_path().generic_string());
				continue;
			}
			modified_files.emplace_back(file.file_path());
		}
	}
	else if (!hashfile.empty())
	{
		std::fstream fs{ hashfile, std::ios::in };
		std::string hash, path;
		char status;
		while (fs >> hash >> status)
		{
			fs.ignore(1); // ignore space
			std::getline(fs, path); // path may contain spaces
			if (status == FileNode::NotChanged)
				continue;

			if (status == FileNode::Modified)
			{
				modified_files.emplace_back(input_dir / stdfs::path{ path });
				continue;
			}

			// status == FileNode::Marks[FileNode::Deleted]
			deleted_files.emplace_back(path);
		}
	}

	if (stdfs::exists(output_dir) && !stdfs::is_empty(output_dir))
	{
		if (!force)
			throw std::runtime_error{ "Output directory is not empty, use -f to force overwrite." };

		if (!quiet)
			util::print("Output directory is not empty, force to overwrite it.\n");
		stdfs::remove_all(output_dir);
	}

	stdfs::create_directories(output_dir);
	for (auto& file : modified_files)
	{
		// util::print("file path: {}\n", file.generic_string());
		stdfs::path target_dest{ output_dir / stdfs::relative(file, input_dir) };
		if (!quiet)
			util::print("Copying: {} -> {}\n", file.string(), target_dest.string());
		if (stdfs::path target_parent = target_dest.parent_path(); !stdfs::exists(target_parent))
			stdfs::create_directories(target_parent);
		stdfs::copy_file(file, target_dest);
	}

	if (deleted_files.empty())
		return;

	auto get_parent = [](const stdfs::path& dir) {
		stdfs::path parent = dir.parent_path();
		if (!dir.has_filename())
			parent = parent.parent_path();
		return parent.empty() ? stdfs::current_path() : parent;
	};
	auto script_maker = ScriptMaker.at(script_type);
	stdfs::path script_path{ get_parent(directory) / util::format("{}.{}", script_name, script_maker->extension()) };

	if (!quiet)
	{
		util::print("Generating {} for following files:\n", script_path.string());
		for (auto& i : deleted_files)
			util::print("  {}\n", i);
	}

	std::fstream script{
		script_path,
		std::ios::out | std::ios::trunc
	};
	script << script_maker->generate(deleted_files);

}
