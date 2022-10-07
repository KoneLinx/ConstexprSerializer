// Copyright (c) Kobe Vrijsen 2022
// Licensed under the EUPL-1.2-or-later

#pragma once

#include <fstream>
#include <string>
#include <vector>
#include <array>

namespace dev
{

	namespace detail
	{
		template <typename Object>
		struct is_parsable;

		template <typename Object, typename Return>
		using enable_if_parsable_t = std::enable_if_t<is_parsable<Object>::value, Return>;
	}

	//
	// Read
	//
	template <typename Object, typename Stream>
#if defined(__cpp_lib_bit_cast)
	constexpr
#endif
	auto read(Stream& stream, Object& object) -> detail::enable_if_parsable_t<Object, bool>;
	
	//
	// Write
	//
	template <typename Object, typename Stream>
#if defined(__cpp_lib_bit_cast)
	constexpr
#endif
	auto write(Stream& stream, Object const& object) -> detail::enable_if_parsable_t<Object, bool>;

	//
	// Layout
	//
	template<typename ... Objects>
	struct Layout
	{
		template <typename Stream>
#if defined(__cpp_lib_bit_cast)
		constexpr
#endif
		static bool Read(Stream& is, Objects & ... objects)
		{
			return (read(is, objects) && ...);
		}

		template <typename Stream>
#if defined(__cpp_lib_bit_cast)
		constexpr
#endif
		static bool Write(Stream& os, Objects const& ... objects)
		{
			return (write(os, objects) && ...);
		}
	};



// ---Implementation---

#pragma region detail

namespace detail
{

	constexpr bool result_fail = false;
	constexpr bool result_success = true;

	template <typename ... T>
	bool failed(std::basic_ios<T...> const& stream)
	{
		constexpr auto state = std::ios_base::badbit | std::ios_base::failbit;
		return bool(stream.rdstate() & state);
	}

	constexpr bool   READ = false;
	constexpr bool   WRITE = true;

	enum class eKind
	{
		invalid = 0,
		trivial,
		itterable,
		pointer
	};

	template <typename, typename = void>
	constexpr static bool is_iterable_v = false;

	template <typename, typename = void>
	constexpr static bool is_contiguous_container_v = false;

	template <typename Object>
	constexpr auto parse_kind() noexcept
	{
		if constexpr (std::is_trivially_copyable_v<Object>)
		{
			return eKind::trivial;
		}
		else if constexpr (is_iterable_v<Object>) // is_container_v
		{
			return eKind::itterable;
		}
		else if constexpr (std::is_pointer_v<Object>)
		{
			return eKind::pointer;
		}
		else return eKind::invalid;
	}

	template <typename Object>
	struct is_parsable : std::bool_constant<parse_kind<Object>() != eKind::invalid> {};

	template <bool W, typename Pod, typename Stream>
#if defined(__cpp_lib_bit_cast)
	constexpr
#endif
	bool parse_pod(Stream& stream, Pod&& data)
	{
		std::array<char, sizeof(Pod)> buffer{};

		if constexpr (W)
		{
#if defined(__cpp_lib_bit_cast)
			buffer = std::bit_cast<decltype(buffer)>(data);
#else
			std::memcpy(std::data(buffer), std::addressof(data), sizeof(Pod));
#endif
			stream.write(std::data(buffer), std::size(buffer));
		}
		else // R
		{
			stream.read(std::data(buffer), std::size(buffer));
#if defined(__cpp_lib_bit_cast)
			data = std::bit_cast<std::remove_reference_t<Pod>>(buffer);
#else
			std::memcpy(std::addressof(data), std::data(buffer), sizeof(Pod));
#endif
		}

#if defined(__cpp_lib_bit_cast)
		if (std::is_constant_evaluated())
			return result_success; // todo: fix?
		else
#endif
			return /*failed(stream)
				? result_fail
				:*/ result_success;

	}

	// Array

	template <bool W, typename Pod, typename Stream>
#if defined(__cpp_lib_bit_cast)
	constexpr
#endif
	bool parse_array(Stream& stream, Pod* const data, ptrdiff_t const count)
	{
		if (count < 0)
			return result_fail;
		else
			if (count == 0)
				return result_success;
			else
				if constexpr (W)
				{
					if (!parse_pod<WRITE>(stream, std::make_unsigned_t<ptrdiff_t>(count)))
						return result_fail;
#if defined(__cpp_lib_bit_cast)
					for (ptrdiff_t i = 0; i < count; ++i)
						parse_pod<WRITE>(stream, data[i]);
#else
					// Undefined behaviour!!!
					stream.write((char const* const)(data), count * sizeof(Pod));
#endif
				}
				else // R
				{
#if defined(__cpp_lib_bit_cast)
					for (ptrdiff_t i = 0; i < count; ++i)
						parse_pod<READ>(stream, data[i]);
#else
					// Undefined behaviour!!!
					stream.read((char* const)(data), count * sizeof(Pod));
#endif
				}

#if defined(__cpp_lib_bit_cast)
		if (std::is_constant_evaluated())
			return result_success; // todo: fix?
		else
#endif
			return /*failed(stream)
			? result_fail
			:*/ result_success;
	}

	// Containers

	template <typename Cont>
	constexpr static bool is_iterable_v<Cont, std::void_t<decltype(std::begin(std::declval<Cont&>()))>> = true;

	template <typename Cont>
	constexpr static bool is_contiguous_container_v<Cont, std::void_t<decltype(std::data(std::declval<Cont&>()))>> = is_iterable_v<Cont>;

	template <bool W, typename Cont, typename Stream>
#if defined(__cpp_lib_bit_cast)
	constexpr
#endif
	bool parse_container(Stream& stream, Cont& cont)
	{
		using T = std::decay_t<decltype(*begin(cont))>;
		if constexpr (std::is_trivially_copyable_v<T> && is_contiguous_container_v<Cont>)
		{
			if constexpr (W)
			{
				return parse_array<WRITE>(stream, data(cont), size(cont));
			}
			else // R
			{
				decltype(size(cont)) count{};
				if (!parse_pod<READ>(stream, count))
					return result_fail;

				cont.resize(count);
				return parse_array<READ>(stream, data(cont), count);
			}
		}
		else
		{
			if constexpr (W)
			{
				parse_pod<WRITE>(stream, size(cont));
				for (auto const& el : cont)
					if (!write(stream, el))
						return result_fail;
				return result_success;
			}
			else // R
			{
				decltype(size(cont)) count{};
				if (!parse_pod<READ>(stream, count))
					return result_fail;
				if (count == 0)
					return result_success;

				auto inserter = std::inserter(cont, end(cont));
				do
				{
					T object{};
					if (!read(stream, object))
						return result_fail;
					inserter = std::move(object);
				} while (--count);

				return result_success;
			}
		}
	}

	// Any

	template <bool W, typename Object, typename Stream>
#if defined(__cpp_lib_bit_cast)
	constexpr
#endif
	auto parse_any(Stream& stream, Object& object) -> enable_if_parsable_t<Object, bool>
	{
		if constexpr (constexpr auto kind = parse_kind<Object>(); kind == eKind::trivial)
		{
			return parse_pod<W>(stream, object);
		}
		else if constexpr (kind == eKind::itterable)
		{
			return parse_container<W>(stream, object);
		}
		else if constexpr (kind == eKind::pointer)
		{
			if constexpr (W)	
				return write(stream, object);
			else				
				return read(stream, object);
		}
		else return result_fail;
	}

}

	template <typename Object, typename Stream>
#if defined(__cpp_lib_bit_cast)
	constexpr
#endif
	auto read(Stream& stream, Object& object) -> detail::enable_if_parsable_t<Object, bool>
	{
		return detail::parse_any<detail::READ>(stream, object);
	}

	template <typename Object, typename Stream>
#if defined(__cpp_lib_bit_cast)
	constexpr
#endif
	auto write(Stream& stream, Object const& object) -> detail::enable_if_parsable_t<Object, bool>
	{
		return detail::parse_any<detail::WRITE>(stream, object);
	}


#pragma endregion

}