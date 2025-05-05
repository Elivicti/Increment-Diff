#pragma once

#include <cryptopp/files.h>
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <cryptopp/channels.h>

#include <filesystem>

#include "util.hpp"

struct Sha1Hash : std::array<CryptoPP::byte, 20>
{
	constexpr Sha1Hash() = default;
	constexpr Sha1Hash(const Sha1Hash& hash)
	{
		std::copy(hash.begin(), hash.end(), this->begin());
	}
	Sha1Hash(const std::string& str)
	{
		from_string(str);
	}
	Sha1Hash(const std::filesystem::path& path)
	{
		compute(path);
	}
	
	Sha1Hash& compute(const std::filesystem::path& path)
	{
		CryptoPP::SHA1 sha1;

		CryptoPP::HashFilter filter(sha1, new CryptoPP::ArraySink(this->data(), this->size()));

		CryptoPP::ChannelSwitch cs;
		cs.AddDefaultRoute(filter);

		CryptoPP::FileSource(path.string().data(), true, new CryptoPP::Redirector(cs));

		return *this;
	}

	Sha1Hash& from_string(const std::string& str)
	{
		CryptoPP::StringSource{str, true, new CryptoPP::HexDecoder{
			new CryptoPP::ArraySink{ this->data(), this->size() }
		}};
		return *this;
	}

	std::string to_string() const;

	constexpr bool valid() const
	{
		return !std::all_of(this->begin(), this->end(), [](const value_type& byte) { return byte == 0; });
	}
	constexpr void invalidate()
	{
		std::fill(this->begin(), this->end(), 0);
	}

public:
	bool operator==(const Sha1Hash& other) const
	{
		return std::equal(this->begin(), this->end(), other.begin());
	}
	bool operator<(const Sha1Hash& other) const
	{
		return std::lexicographical_compare(this->begin(), this->end(), other.begin(), other.end());
	}
	bool operator>(const Sha1Hash& other) const
	{
		return other < *this;
	}
	bool operator<=(const Sha1Hash& other) const
	{
		return !(*this > other);
	}
	bool operator>=(const Sha1Hash& other) const
	{
		return !(*this < other);
	}
};

class FileNode
{
public:
	enum StatusFlag
	{
		NotChanged, // default
		Modified,   // new file or modified old file
		Deleted     //
	};

public:
	FileNode(const std::filesystem::path& path_, StatusFlag flag_ = NotChanged)
		: path{ path_ }, hash{}, flag{ flag_ } {}

	FileNode(const std::string& path_, StatusFlag flag_ = NotChanged)
		: path{ path_ }, hash{}, flag{ flag_ } {}

	static constexpr char Marks[] = "=*-";
	const Sha1Hash& file_hash() const
	{
		return hash;
	}
	void compute_hash() const
	{
		hash.compute(path);
	}
	void compute_hash(const std::filesystem::path& parent) const
	{
		hash.compute(parent / path);
	}
	void set_hash(const Sha1Hash& hash) const { this->hash = hash; }

	bool compare_hash(const Sha1Hash& hash) const { return this->hash == hash; }
	bool compare_hash(const std::string& hash) const
	{
		return compare_hash(Sha1Hash{ hash });
	}

	void set_status(StatusFlag flag) const { this->flag = flag; }
	void set_status(char mark) const
	{
		switch (mark)
		{
		case Marks[NotChanged]:
			this->flag = NotChanged;
			break;
		case Marks[Modified]:
			this->flag = Modified;
			break;
		case Marks[Deleted]:
			this->flag = Deleted;
			break;
		default:
			throw std::invalid_argument{ "Invalid mark." };
		}
	}
	StatusFlag status_flag() const { return flag; }
	char status_mark() const { return Marks[flag]; }

	std::string to_string() const;

	const std::filesystem::path& file_path() const { return path; }
private:
	std::filesystem::path path;
	mutable Sha1Hash hash;
	mutable StatusFlag flag;

public:
	bool operator<(const FileNode& other) const
	{
		return path < other.path;
	}
};

template <typename Char>
struct FMT_NS::formatter<Sha1Hash, Char>
{
private:
	using value_type = Sha1Hash::value_type;
	FMT_NS::formatter<std::remove_cvref_t<value_type>, Char> value_formatter_;

public:
	template <typename ParseContext>
	constexpr auto parse(ParseContext &ctx)
	{
		return value_formatter_.parse(ctx);
	}

	template <typename FormatContext>
	auto format(const Sha1Hash& value, FormatContext &ctx) const
		-> decltype(ctx.out())
	{
		auto it  = value.begin();
		auto end = value.end();
		auto out = ctx.out();
		while (it != end)
		{
			out = value_formatter_.format(*it, ctx);
			++it;
			ctx.advance_to(out);
		}
		return out;
	}
};


inline std::string Sha1Hash::to_string() const
{
	return util::format("{:02X}", *this);
}

inline std::string FileNode::to_string() const
{
	return util::format("{:02X} {} {}"
		, hash
		, Marks[flag]
		, path.string()
	);
}