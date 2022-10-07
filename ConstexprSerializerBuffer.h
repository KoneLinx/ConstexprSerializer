// Copyright (c) Kobe Vrijsen 2022
// Licensed under the EUPL-1.2-or-later

#pragma once

#include <bit>
#include <span>
#include <array>
#include <ranges>
#include <vector>
#include <stdexcept>
#include <algorithm>


template <size_t EXTENT = std::dynamic_extent>
class Serializer;

namespace detail {

	// Base implementation
	class SerializerBase
	{

		// Byte array alias
		template <typename Val>
		using Bytes = std::array<std::byte, sizeof(Val)>;
	
	public:
	
		// Without buffer
		explicit constexpr SerializerBase() = default;
	

		// Copy / Move
		constexpr SerializerBase(SerializerBase const&) = default;
		constexpr SerializerBase& operator = (SerializerBase const&) = default;
	
		constexpr SerializerBase(SerializerBase&&) = default;
		constexpr SerializerBase& operator = (SerializerBase&&) = default;
	

		// With buffer
		constexpr SerializerBase(std::span<std::byte> buffer)
			: m_Buffer{ buffer }
			, m_Free{ m_Buffer.begin(), m_Buffer.end() }
			, m_ToRead{ m_Buffer.begin(), m_Buffer.begin() }
		{}
	
		// With buffer and default
		template <typename Val> requires std::is_trivially_copyable_v<Val> && (!std::is_pointer_v<Val>)
		constexpr SerializerBase(std::span<std::byte> buffer, std::initializer_list<Val> list)
			: SerializerBase{ buffer, std::span{ list } }
		{}
	
		// With buffer and default (range)
		template <std::ranges::sized_range Src, typename Val = std::ranges::range_value_t<Src>> requires std::is_trivially_copyable_v<Val>
		constexpr SerializerBase(std::span<std::byte> buffer, Src const& source)
			: SerializerBase{ buffer }
		{
			write(source);
		}


		// Write memory (iostream alike)
		constexpr SerializerBase& write(char const* src, std::streamsize count)
		{
			if (count > std::ssize(m_Free))
				throw std::runtime_error{ "Buffer overflow" };

			write(std::span{ src, src + count });

			return *this;
		}
		//
		// Read memory (iostream alike)
		constexpr SerializerBase& read(char* dest, std::streamsize count)
		{
			if (count > std::ssize(m_ToRead))
				throw std::runtime_error{ "Buffer holds too little data" };

			read(std::span{ dest, dest + count });

			return *this;
		}

	
		// Write value
		template <typename Val> requires std::is_trivially_copyable_v<Val>
		constexpr SerializerBase& write(Val const& value)
		{
			if (sizeof(Val) > m_Free.size())
				throw std::runtime_error{ "Buffer overflow" };

			auto const bytes = std::bit_cast<Bytes<Val>>(value); // reinterpret_cast
			
			m_Free = { 
				std::ranges::copy(bytes, m_Free.begin()).out,
				m_Free.end() 
			};

			m_ToRead = {
				// to_address because of debug iterator mismatch assert
				std::to_address(m_ToRead.begin()),
				std::to_address(m_Free  .begin())
			};
	
			return *this;
		}
		//
		// Read value
		template <typename Val> requires std::is_trivially_copyable_v<Val>
		constexpr Val read()
		{
			if (sizeof(Val) > std::size(m_ToRead))
				throw std::runtime_error{ "Buffer empty" };
	
			Bytes<Val> bytes{};
			m_ToRead = {
				std::ranges::copy_n(m_ToRead.begin(), bytes.size(), bytes.begin()).in,
				m_ToRead.end()
			};
			return std::bit_cast<Val>(bytes);
		}
		//
		// Read value (into memory)
		template <typename Val> requires std::is_trivially_copyable_v<Val>
		constexpr SerializerBase& read(Val& value)
		{
			value = read<Val>();
	
			return *this;
		}


		// Write values (ranges range)
		template <std::ranges::sized_range Source, typename Value = std::ranges::range_value_t<Source>> requires std::is_trivially_copyable_v<Value>
		constexpr SerializerBase& write(Source&& source)
		{
			if (std::ranges::size(source) * sizeof(Value) > m_Free.size())
				throw std::exception{ "Range too large, insufficient buffer size" };

			for (auto& value : source)
				write(value);

			return *this;
		}
		//
		// Read values (ranges range)
		template <std::ranges::sized_range Dst, typename Value = std::ranges::range_value_t<Dst>> requires std::is_trivially_copyable_v<Value>
		constexpr SerializerBase& read(Dst&& dest)
		{
			if (std::ranges::size(dest) * sizeof(Value) > m_ToRead.size())
				throw std::exception{ "Range too large, insufficient bytes queued" };

			for (auto& value : dest)
				read(value);

			return *this;
		}



		//
		// Thinking of deleting or restricting these is as there is no good way to determine the final size. the range also requires pre-allocated memory.
		// Hence [[depricated]] untill further notice
		// 
		// TODO fix this function
		// Read throws buffer overflow if the outpur range is too large.
		// Difficult to figure out by the user
		// 
		// Write values (range)
		template <std::ranges::input_range Src> requires std::is_trivially_copyable_v<std::ranges::range_value_t<Src>> && (!std::ranges::sized_range<Src>)
		[[deprecated]]
		constexpr SerializerBase& write(Src&& source)
		{
			for (auto const& value : source)
				write(value);

			return *this;
		}
		//
		// Read values (output range)
		template <std::ranges::range Dst, typename Value = std::ranges::range_value_t<Dst>> requires std::ranges::output_range<Dst, Value>&& std::is_trivially_copyable_v<Value> && (!std::ranges::sized_range<Dst>)
		[[deprecated]] 
		constexpr SerializerBase& read(Dst&& dest)
		{
			for (auto& value : dest)
				read(value);

			return *this;
		}


		// Reset buffer
		constexpr void reset_buffer(std::span<std::byte> buffer)
		{
			m_Free = m_Buffer = buffer;

			m_ToRead = std::span{ m_Buffer.begin(), size_t{} };
		}
	

		// Clear buffer
		constexpr void clear()
		{
			// Effectively mark as overwritable
			reset_buffer(m_Buffer);
		}

	
	private:
	
		std::span<std::byte> m_Buffer;

		std::span<std::byte> m_Free;
		std::span<std::byte> m_ToRead;
	
	};

}

template <size_t EXTENT>
class Serializer : private detail::SerializerBase
{
public:

	constexpr Serializer(Serializer const&) = default;
	constexpr Serializer(Serializer&&) = default;
	constexpr Serializer& operator = (Serializer const&) = default;
	constexpr Serializer& operator = (Serializer&&) = default;

	using SerializerBase::read;
	using SerializerBase::write;
	using SerializerBase::clear;

	// Empty
	constexpr Serializer()
	{
		reset_buffer(m_Array);
	}

	// With default
	template <typename Val> requires std::is_trivially_copyable_v<Val> && (!std::is_pointer_v<Val>)
		constexpr Serializer(std::initializer_list<Val> list)
		: Serializer{ std::span{ list } }
	{}

	// With default (range)
	template <std::ranges::sized_range Src, typename Val = std::ranges::range_value_t<Src>> requires std::is_trivially_copyable_v<Val>
	constexpr Serializer(Src const& source)
		: Serializer{}
	{
		write(source);
	}

private:

	// Buffer
	std::array<std::byte, EXTENT> m_Array{};

};

template <>
class Serializer<std::dynamic_extent> : private detail::SerializerBase
{
public:

	// Copy
	constexpr Serializer(Serializer const&) = delete;
	constexpr Serializer& operator = (Serializer const&) = delete;

	// Move
	constexpr Serializer(Serializer&&) = default;
	constexpr Serializer& operator = (Serializer&&) = default;

	using SerializerBase::read;
	using SerializerBase::write;
	using SerializerBase::clear;

	// A buffer of size 0 makes no sense
	constexpr Serializer() = delete;

	// With size
	constexpr Serializer(size_t size)
		: SerializerBase{/* no buffer */}, m_Vector(size)
	{
		m_Vector.resize(size);
		reset_buffer({ m_Vector.begin(), size });
	}

	// With size and default
	template <typename Val> requires std::is_trivially_copyable_v<Val> && (!std::is_pointer_v<Val>)
		constexpr Serializer(size_t size, std::initializer_list<Val> list)
		: Serializer{ size, std::span{ list } }
	{}

	// With default
	template <typename Val> requires std::is_trivially_copyable_v<Val> && (!std::is_pointer_v<Val>)
		constexpr Serializer(std::initializer_list<Val> list)
		: Serializer{ std::size(list) * sizeof(Val), std::span{list} }
	{}

	// With size and default (range)
	template <std::ranges::sized_range Src, typename Val = std::ranges::range_value_t<Src>> requires std::is_trivially_copyable_v<Val>
	constexpr Serializer(size_t size, Src const& source)
		: Serializer(size)
	{
		write(source);
	}

	// With default (range)
	template <std::ranges::sized_range Src, typename Val = std::ranges::range_value_t<Src>> requires std::is_trivially_copyable_v<Val>
	constexpr Serializer(Src const& source)
		: Serializer{ std::size(source) * sizeof(Val), source }
	{}

private:

	// Buffer
	std::vector<std::byte> m_Vector{};

};


template <typename T, size_t EXT>
Serializer(T(&)[EXT]) -> Serializer<EXT * sizeof(T)>;

template <typename T, size_t EXT>
Serializer(std::array<T, EXT>) -> Serializer<EXT * sizeof(T)>;

Serializer(size_t) -> Serializer<>;

Serializer(int) -> Serializer<>;
