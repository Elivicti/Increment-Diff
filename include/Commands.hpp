#pragma once

#include <CLI/CLI.hpp>

#include "Format.hpp"
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

struct EmptyDirectoryOrNonexistingPathValidator : public CLI::Validator
{
	EmptyDirectoryOrNonexistingPathValidator()
		: CLI::Validator{"DIR(empty)|PATH(non-existing)"}
	{
		namespace stdfs = std::filesystem;
		func_ = [](std::string& filename) {
			stdfs::path path{ filename };

			if (!stdfs::exists(path))
				return std::string{};

			if (stdfs::is_directory(path) && stdfs::is_empty(path))
				return std::string{};
			
			return util::format("{} exists and is not empty.", filename);
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
	std::string directory;
	std::string script_name;
	std::string script_type;

	inline static const std::map<std::string, ScriptFactory::shared_ptr> ScriptMaker {
		{ ScriptTypeValidator::BASH, ScriptFactory::make<BashScript>() },
		{ ScriptTypeValidator::BAT,  ScriptFactory::make<BatScript>()  },
		{ ScriptTypeValidator::PSH,  ScriptFactory::make<PshScript>()  }
	};

	MakeIncrementDiff(CLI::App* app_)
		: CliSubcommand{ app_ }
	{
		app->add_option("-o,--output", output, "output directory")
			->check(EmptyDirectoryOrNonexistingPathValidator{})
			->required(true);

		app->add_option("directory", directory, "path")
			->check(CLI::ExistingDirectory)
			->required(true);
		
		app->add_option("-H,--hash", hashfile, "diff file containing hash")
			->check(CLI::ExistingFile)
			->required(true);

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
	}

	virtual void operator()() override
	{
		namespace stdfs = std::filesystem;

		std::fstream fs{ hashfile, std::ios::in };
		stdfs::path dir{ directory };
		stdfs::path output_path{ output };

		stdfs::create_directories(output_path);

		std::vector<std::string> deleted_files;

		std::string hash, path;
		char status;
		while (fs >> hash >> status >> path)
		{
			if (status == FileNode::Marks[FileNode::NotChanged])
				continue;
			
			if (status == FileNode::Marks[FileNode::Modified])
			{
				stdfs::path file{ dir / path };
				stdfs::path target{ output_path / path };
				if (stdfs::path target_parent = target.parent_path(); !stdfs::exists(target_parent))
					stdfs::create_directories(target_parent);
				stdfs::copy_file(file, target);

				continue;
			}

			// status == FileNode::Marks[FileNode::Deleted]
			deleted_files.emplace_back(path);
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
		std::fstream script{
			get_parent(directory) / util::format("{}.{}", script_name, script_maker->extension()),
			std::ios::out | std::ios::trunc
		};
		script << script_maker->generate(deleted_files);
	}
};