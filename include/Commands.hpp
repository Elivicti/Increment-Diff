#pragma once

#include <CLI/CLI.hpp>

#include "utils.hpp"
#include "FileNode.hpp"

struct CliSubcommand
{
	CLI::App* app;

	CliSubcommand(CLI::App* app_)
		: app{ app_ }
	{
		app->callback([this]() { this->operator()(); });
	}
	virtual ~CliSubcommand() = default;

	CLI::App* operator->()
	{ return app; }

	virtual void operator()() = 0;
};


struct HashCommand : public CliSubcommand
{
	std::string output;
	std::string directory;
	std::string compare_hash;

	HashCommand(CLI::App* app_)
		: CliSubcommand{ app_ }
	{
		app->add_option("-o,--output", output, "output file, if not specified, output to stdout");

		app->add_option("directory", directory, "path to directory that needs to compute hash")
			->check(CLI::ExistingDirectory)
			->required(true);
		
		app->add_option("-c,--compare", compare_hash, "previous hash to compare")
			->check(CLI::ExistingFile);
	}

	static std::set<FileNode> compute_directory_hash(const std::filesystem::path& dir)
	{
		std::set<FileNode> files;
		for (auto& entry : std::filesystem::recursive_directory_iterator{ dir })
		{
			if (entry.is_directory())
				continue;
			std::filesystem::path relative_path{ std::filesystem::relative(entry.path(), dir) };
			auto [it, success] = files.emplace(relative_path, FileNode::Modified);
			it->compute_hash(dir);
		}
		return files;
	}

	static std::set<FileNode>& compare_with_existing_hash(std::set<FileNode>& files, const std::string& hashfile)
	{
		using iterator = std::remove_cvref_t<decltype(files)>::iterator;
		std::fstream hashfs{ hashfile, std::ios::in };
		std::string hash, status, path;
		while (hashfs >> hash >> status)
		{
			hashfs.ignore(1); // ignore space
			std::getline(hashfs, path); // path may contain spaces

			iterator it = files.find(path);
			if (it == files.end())
			{
				auto [fileit, success] = files.emplace(path, FileNode::Deleted);
				fileit->set_hash(hash);
				continue;
			}
			if (it->compare_hash(hash))
				it->set_status(FileNode::NotChanged);
		}
		return files;
	}

	virtual void operator()() override
	{
		namespace stdfs = std::filesystem;

		stdfs::path dir{ directory };
		std::set<FileNode> files = compute_directory_hash(dir);

		if (!compare_hash.empty())
		{
			compare_with_existing_hash(files, compare_hash);
		}

		if (output.empty())
		{
			for (auto& file : files)
			{
				util::print("{}\n", file.to_string());
			}
			return;
		}

		std::fstream fs{ output, std::ios::out | std::ios::trunc };
		for (auto& file : files)
		{
			util::print(fs, "{}\n", file.to_string());
		}
	}
};

struct DirectoryOrNonexistingPathValidator : public CLI::Validator
{
	DirectoryOrNonexistingPathValidator()
		: CLI::Validator{"DIR|PATH(non-existing)"}
	{
		namespace stdfs = std::filesystem;
		func_ = [](std::string& filename) {
			stdfs::path path{ filename };

			if (!stdfs::exists(path))
				return std::string{};

			if (stdfs::is_directory(path))
				return std::string{};
			
			return util::format("{} exists and is not a directory.", filename);
		};
	}
};

struct ScriptTypeValidator : public CLI::Validator
{
	constexpr static const char* BASH = "bash";
	constexpr static const char* BAT = "bat";
	constexpr static const char* PSH = "psh";


	ScriptTypeValidator()
		: CLI::Validator{ util::format("CHOICE({},{},{})", BASH, BAT, PSH) }
	{
		func_ = [](std::string& type) -> std::string {
			if (type == BASH || type == BAT || type == PSH)
				return std::string{};
			return util::format("unkown script type, expecting \"{}\", \"{}\" or \"{}\", but \"{}\" provided."
				, BASH, BAT, PSH, type);
		};
	}
};

struct ScriptFactory
{
	virtual ~ScriptFactory() = default;
	virtual std::string generate(const std::vector<std::string>& deleted_files) const = 0;
	virtual constexpr std::string extension() const = 0;

	using shared_ptr = std::shared_ptr<ScriptFactory>;

	template<typename ScriptType>
	static shared_ptr make() { return std::make_shared<ScriptType>(); }
};
struct BashScript : public ScriptFactory
{
	virtual std::string generate(const std::vector<std::string>& deleted_files) const override
	{
		std::string script{"#!/bin/bash\ndir=\"$1\"\n"};
		
		for (auto& i : deleted_files)
			script += util::format("rm \"$dir/{0}\" && echo Deleted: \"$dir/{0}\"\n", i);

		return script;
	}
	virtual constexpr std::string extension() const override { return "sh"; }
};
struct BatScript : public ScriptFactory
{
	virtual std::string generate(const std::vector<std::string>& deleted_files) const override
	{
		std::string script{"@echo off\nset dir=%1\n"};

		for (auto i : deleted_files)
		{
			std::transform(i.begin(), i.end(), i.begin(), [](char c) {
				if (c == '/')
					return '\\';
				return c;
			});
			script += util::format("del %dir%\\{0} && echo Deleted: %dir%\\{0}\n", i);
		}
		return script;
	}
	virtual constexpr std::string extension() const override { return "bat"; }
};
struct PshScript : public ScriptFactory
{
	virtual std::string generate(const std::vector<std::string>& deleted_files) const override
	{
		std::string script{"Param( [string]$dir )\n"};

		for (auto& i : deleted_files)
			script += util::format("Remove-Item \"$dir/{0}\" && echo Deleted: \"$dir/{0}\"\n", i);
		return script;
	}
	virtual constexpr std::string extension() const override { return "ps1"; }
};


struct MakeIncrementDiff : public CliSubcommand
{
	std::string output;
	std::string hashfile;
	std::string compare_hash;
	std::string directory;
	std::string script_name;
	std::string script_type;
	bool force;
	bool quiet;

	inline static const std::map<std::string, ScriptFactory::shared_ptr> ScriptMaker {
		{ ScriptTypeValidator::BASH, ScriptFactory::make<BashScript>() },
		{ ScriptTypeValidator::BAT,  ScriptFactory::make<BatScript>()  },
		{ ScriptTypeValidator::PSH,  ScriptFactory::make<PshScript>()  }
	};

	MakeIncrementDiff(CLI::App* app_)
		: CliSubcommand{ app_ }
	{
		app->add_option("-o,--output", output, "output directory")
			->check(DirectoryOrNonexistingPathValidator{})
			->required(true);

		app->add_option("directory", directory, "path")
			->check(CLI::ExistingDirectory)
			->required(true);
		
		auto group = app->add_option_group("Compare Type", "");

		group->add_option("-H,--hash", hashfile, "read from file for hash diff messages")
			->check(CLI::ExistingFile);
		group->add_option("-c,--compare", compare_hash, "compute directory's hash and compare with this file")
			->check(CLI::ExistingFile);
		group->require_option(1);

		app->add_option("-s,--script", script_name, "specify generated shell script name, with no suffix")
			->default_val("clean");
		app->add_option("-t,--script-type", script_type, "specify which type of shell script will be generated")
			->check(ScriptTypeValidator{})
			->default_val<std::string>(
			#if defined(_WIN32)
				ScriptTypeValidator::PSH
			#elif defined(__unix__) || defined(__APPLE__)
				ScriptTypeValidator::BASH
			#endif
			);
		app->add_flag("-f,--force", force, "if output directory is not empty, force to overwrite it")
			->default_val(false);
		app->add_flag("-q,--quiet", quiet, "suppress output")
			->default_val(false);
	}

	virtual void operator()() override
	{
		namespace stdfs = std::filesystem;
		stdfs::path input_dir{ directory };
		stdfs::path output_dir{ output };

		std::vector<stdfs::path> modified_files;
		std::vector<std::string> deleted_files;

		if (!compare_hash.empty())
		{
			auto files = HashCommand::compute_directory_hash(input_dir);
			HashCommand::compare_with_existing_hash(files, compare_hash);
			for (auto& file : files)
			{
				auto status = file.status_flag();
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
		if (!hashfile.empty())
		{
			std::fstream fs{ hashfile, std::ios::in };
			std::string hash, path;
			char status;
			while (fs >> hash >> status)
			{
				fs.ignore(1); // ignore space
				std::getline(fs, path); // path may contain spaces
				if (status == FileNode::Marks[FileNode::NotChanged])
					continue;
				
				if (status == FileNode::Marks[FileNode::Modified])
				{
					modified_files.emplace_back(stdfs::path{ path });
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
			stdfs::path target_dest{ output_dir / file };
			stdfs::path file_path  { input_dir  / file };
			if (!quiet)
				util::print("Copying: {} -> {}\n", file_path.string(), target_dest.string());
			if (stdfs::path target_parent = target_dest.parent_path(); !stdfs::exists(target_parent))
				stdfs::create_directories(target_parent);
			stdfs::copy_file(file_path, target_dest);
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
};