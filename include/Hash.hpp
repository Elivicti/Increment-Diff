#pragma once

#include "utils.hpp"

#include <filesystem>
#include <ranges>
#include <array>

#include <picosha2.h>

namespace alt
{
#ifdef __cpp_lib_ranges_chunk
	using std::views::chunk;
#else
	template<std::ranges::view V>
	class chunk_view : public std::ranges::view_interface<chunk_view<V>>
	{
		V base;
		std::size_t chunk_size;

		class iterator
		{
			using BaseIter = std::ranges::iterator_t<V>;
			BaseIter current;
			BaseIter end;
			std::size_t chunk_size;

		public:
			using iterator_category = std::input_iterator_tag;
			using value_type = std::ranges::subrange<BaseIter>;
			using difference_type = std::ranges::range_difference_t<V>;

			iterator() = default;

			iterator(BaseIter begin, BaseIter end, std::size_t chunk_size)
				: current{ begin }, end{ end }
				, chunk_size{ chunk_size } {}

			value_type operator*() const
			{
				auto next = current;
				std::advance(next, chunk_size);
				return { current, next };
			}

			iterator& operator++()
			{
				std::advance(current, chunk_size);
				return *this;
			}

			iterator operator++(int)
			{
				auto tmp = *this;
				this->operator++()();
				return tmp;
			}

			bool operator==(const iterator& other) const
			{ return current == other.current; }

			bool operator==(std::default_sentinel_t) const
			{ return current == end; }
		};

	public:
		chunk_view() = default;

		chunk_view(V base, std::size_t chunk_size)
			: base{ std::move(base) }, chunk_size{ chunk_size }
		{ assert(chunk_size > 0); }

		auto begin()
		{ return iterator{ std::ranges::begin(base), std::ranges::end(base), chunk_size }; }

		auto end()
		{ return std::default_sentinel; }

		auto begin() const requires std::ranges::range<const V>
		{ return iterator{ std::ranges::begin(base), std::ranges::end(base), chunk_size }; }

		auto end() const requires std::ranges::range<const V>
		{ return std::default_sentinel; }
	};

	struct chunk_fn
	{
		std::size_t chunk_size;

		template <std::ranges::viewable_range R>
		auto operator()(R&& r) const
		{ return chunk_view{ std::views::all(std::forward<R>(r)), chunk_size }; }

	};

	inline chunk_fn chunk(std::size_t chunk_size)
	{ return { chunk_size }; }

	template <std::ranges::viewable_range R>
	inline auto operator|(R&& r, const chunk_fn& adapter)
	{ return adapter(std::forward<R>(r)); }
#endif
}

enum class HashAlgorithm : std::uint32_t
{
	Sha256,
};

template<std::size_t N>
using byte_array = std::array<uint8_t, N>;


template<std::size_t N, typename CharT>
struct FMT_NS::formatter<byte_array<N>, CharT>
{
	using bytearray = byte_array<N>;
	using value_type = typename bytearray::value_type;

	FMT_NS::formatter<std::remove_cvref_t<value_type>, CharT> value_formatter;

	template <typename ParseContext>
	constexpr auto parse(ParseContext &ctx)
	{
		return value_formatter.parse(ctx);
	}

	template <typename FormatContext>
	auto format(const bytearray& value, FormatContext &ctx) const
		-> decltype(ctx.out())
	{
		auto it  = value.begin();
		auto end = value.end();
		auto out = ctx.out();
		while (it != end)
		{
			out = value_formatter.format(*it, ctx);
			++it;
			ctx.advance_to(out);
		}
		return out;
	}
};

template<HashAlgorithm ALG, std::size_t N>
struct Hash
{
	using bytearray = byte_array<N>;
	bytearray value;

	using value_type = typename bytearray::value_type;

	struct hex_format_tag{};
	inline static constexpr hex_format_tag hex_format{};

	Hash() : value{}  {}
	Hash(const Hash& other) = default;
	Hash(Hash&& other) = default;

	Hash(std::string_view hex, hex_format_tag tag)
		: Hash{ std::move(from_hexstring(hex)) } {}

	void reset()
	{ std::memset(value.data(), 0, N); }

	consteval HashAlgorithm algorithm() const
	{ return ALG; }

	static Hash from_hexstring(std::string_view hex)
	{
		Hash ret;
		const std::size_t size = hex.size();
		if ((size & 1) != 0)
			throw std::invalid_argument{ "size of hex string can not be odd number" };

		if (size > N * 2) // ensure not overflow
			hex = hex.substr(0, N * 2);

		auto bytes_view = hex
			| alt::chunk(2)
			| std::views::transform([](auto byte_chars) {
				uint8_t byte = 0;
				const char* ptr = &*byte_chars.begin();
				std::from_chars(ptr, ptr + 2, byte, 16);
				return byte;
			});
		std::ranges::copy(bytes_view, ret.value.begin());
		return ret;
	}

	constexpr bool is_null() const
	{
		constexpr auto is_zero = [](auto v) { return v == decltype(v){}; };
		return std::ranges::all_of(value, is_zero);
	}

	std::string to_hexstring() const
	{ return util::format("{:02X}", value); }

	operator std::string() const
	{ return this->to_hexstring(); }


	bool operator==(const Hash& other) const
	{ return std::equal(value.begin(), value.end(), other.value.begin()); }
	bool operator<(const Hash& other) const
	{
		return std::lexicographical_compare(
			value.begin(), value.end(),
			other.value.begin(), other.value.end()
		);
	}
	bool operator>(const Hash& other) const
	{ return other < *this; }
	bool operator<=(const Hash& other) const
	{ return !(*this > other); }
	bool operator>=(const Hash& other) const
	{ return !(*this < other); }
};

struct Sha256 : Hash<HashAlgorithm::Sha256, picosha2::k_digest_size>
{
	using Hash = Hash<HashAlgorithm::Sha256, picosha2::k_digest_size>;
	using Hash::Hash;

	void from_file(const std::filesystem::path& path)
	{
		if (std::filesystem::is_directory(path))
			throw std::invalid_argument{ "can not compute sha256 hash for directory" };

		this->reset();
		if (!std::filesystem::exists(path))
			return;
		std::ifstream ifs{ path, std::ios::in | std::ios::binary };
		picosha2::hash256(ifs, value.begin(), value.end());
	}

};
