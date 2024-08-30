#pragma once

#include <cryptopp/files.h>
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <cryptopp/channels.h>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <filesystem>

class FileNode
{
public:
	using HashDigest = std::array<CryptoPP::byte, 20>;
	enum StatusFlag
	{
		NotChanged, // default
		Modified,   // new file or modified old file
		Deleted     //
	};

public:
	FileNode(const HashDigest& hash_, const std::filesystem::path& path_, StatusFlag flag_ = NotChanged)
		: path{ path_ }, hash{ hash_ }, flag{ flag_ } {}

	FileNode(const std::string& hash_, const std::string& path_, StatusFlag flag_ = NotChanged)
		: path{ path_ }
		, hash{ ConvertHexString(hash_) }
		, flag{ flag_ }
	{}
	

	static constexpr char Marks[] = "=*-";
	std::string toString() const
	{
		return fmt::format("{:02X} {} {}"
			, fmt::join(hash, "")
			, Marks[flag]
			, path.string()
		);
	}
	const HashDigest& getHash() const { return hash; }

	bool compareHash(const  HashDigest& hash) const { return this->hash == hash; }
	bool compareHash(const std::string& hash) const
	{
		return compareHash(ConvertHexString(hash));
	}

	static HashDigest ConvertHexString(const std::string& str)
	{
		HashDigest array;
		CryptoPP::StringSource{str, true, new CryptoPP::HexDecoder{
			new CryptoPP::ArraySink{ array.data(), array.size() }
		}};
		return array;
	}
	static HashDigest GetFileSha1(const std::filesystem::path& path)
	{
		HashDigest digest;
		CryptoPP::SHA1 sha1;

		CryptoPP::HashFilter filter(sha1, new CryptoPP::ArraySink(digest.data(), digest.size()));

		CryptoPP::ChannelSwitch cs;
		cs.AddDefaultRoute(filter);

		CryptoPP::FileSource(path.string().data(), true, new CryptoPP::Redirector(cs));
		return digest;
	}
	
	mutable StatusFlag flag;
private:
	std::filesystem::path path;
	HashDigest hash;

public:
	bool operator<(const FileNode& other) const
	{
		return path < other.path;
	}
};