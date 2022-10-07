// Copyright (c) Kobe Vrijsen 2022
// Licensed under the EUPL-1.2-or-later

#include "ConstexprSerializerBuffer.h"

#include <string>
#include <vector>

static_assert(
	[]
	{
		Serializer<8> io{};
		io.write(13);
		return io.read<int>() == 13;
	}
	(), 
	"Integer serialization"
);

static_assert(
	[] {

		Serializer io(16); // do not call initialiser list as it will allocate a single size_t
		io.write(13l);
		io.write(27ll);
		return io.read<long>() == 13l
			&& io.read<long long>() == 27ll;
	}(),
	"dynamic buffer"
);

static_assert(
	[]
	{
		std::wstring wstr{ L"Hello world" };
		std::vector<short> nums(wstr.size(), {});

		Serializer{ wstr }.read(nums); // From string to vector
		Serializer{ nums }.read(wstr); // Vice-versa

		return wstr == L"Hello world";
	}
	(),
	"String serialization"
);

static_assert(
	[]
	{
		struct Data
		{
			bool b;
			char c;
			float f;

			constexpr bool operator == (Data const& other) const 
			{
				return b == other.b && c == other.c && f == other.f;
			}
		};

		using Container = std::vector<Data>;
		
		Serializer<64> io{};

		Container container_origin{
			{false, 'a', 2.47f },
			{true , 'b', 3.14f }
		};
		
		{
			io.write(std::ranges::size(container_origin));
			io.write(container_origin);
		}

		Container container_replica{};
		
		{
			std::ranges::generate_n(
				std::inserter(container_replica, std::ranges::end(container_replica)),
				io.read<Container::size_type>(),
				[&io] () 
				{
					return io.read<Container::value_type>();
				}
			);
		}

		return std::ranges::equal(container_origin, container_replica);
	}
	(),
	"vector and struct serialization"
);
