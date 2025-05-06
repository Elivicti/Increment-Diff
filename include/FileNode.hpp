#pragma once

#include "Hash.hpp"

class FileNode
{
public:
	enum Status : char
	{
		NotSet     = 0,
		NotChanged = '=',
		Modified   = '*',
		Deleted    = '-',
	};
public:
	FileNode(const std::filesystem::path& path, const Sha256& old_hash)
		: path{ path }, old_hash{ old_hash }
	{
		if (std::filesystem::is_directory(path))
			throw std::invalid_argument{ "expecting file or non-existing path, got directory" };
	}
	FileNode(const std::filesystem::path& path, std::string_view old_hash_hex_str)
		: FileNode{ path, Sha256{ old_hash_hex_str, Sha256::hex_format } } {}

	void compute_hash()
	{
		new_hash.from_file(path);
	}

	Status status() const
	{
		if (new_hash.is_null())
			new_hash.from_file(path); // if path not exists, new_hash will be all zero

		// 0b0'0
		//   ^ ^
		// old new
		const uint8_t null_stat =
			(((uint8_t)!old_hash.is_null()) << 1) | ((uint8_t)!new_hash.is_null());

		switch (null_stat)
		{
		[[unlikely]] case 0b00:
			// means this file doesn't exist before, and doesn't exist now,
			// which seems impossible, may be happening when user modifies
			// folder while the program is running.
			throw std::runtime_error{
				util::format(
					"{} has all zero before and after",
					path.string()
				)
			};
		case 0b01:
			return Status::Modified;
		case 0b10:
			return Status::Deleted;
		}
		if (old_hash == new_hash)
			return Status::NotChanged;
		else
			return Status::Modified;
	}

	std::string to_string() const
	{
		return util::format(
			"{} {} {}",
			new_hash.to_hexstring(),
			(char)status(),
			path.generic_string()
		);
	}
	std::string to_string(const std::filesystem::path& relative_to) const
	{
		return util::format(
			"{} {} {}",
			new_hash.to_hexstring(),
			(char)status(),
			std::filesystem::relative(path, relative_to).generic_string()
		);
	}

	std::filesystem::path file_path() const { return path; }

private:
	std::filesystem::path path;
	const   Sha256 old_hash;
	mutable Sha256 new_hash;

public:
	bool operator<(const FileNode& other) const
	{
		return path < other.path;
	}
};