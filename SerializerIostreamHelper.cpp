// Copyright (c) Kobe Vrijsen 2022
// Licensed under the EUPL-1.2-or-later

#include "ConstexprSerializerBuffer.h"
#include "SerializerIostreamHelper.h"

static_assert(
	[]
	{

		using serializer_helper::Layout;

		struct MyStruct {
			char a = 'a';
			bool b = true;
			double c = 2.47;
			int d = 7;
		};

		using TestLayout = Layout<int, std::string, std::array<int, 3>, std::vector<MyStruct>>;

		Serializer<256> io{};

		// Write
		{

			//static_assert(JL_BINARY::detail::parse_kind<std::array<int, 3>>() != JL_BINARY::detail::eKind::invalid);

			TestLayout::Write(io, 13, "Hello", { 1,2,3 }, { MyStruct{}, MyStruct{.a = 'b', .c = 3.1415 } });

		}

		// Read
		{
			int i{};
			std::string str{};
			std::array<int, 3> arr{};
			std::vector<MyStruct> vec(size_t(2));

			TestLayout::Read(io, i, str, arr, vec);

			return (int)(i + str.size() + arr[0] + vec.size()); // 13 + 5 + 1 + 2 = 21
		}

		return 0;
	}
	(),
	"Test layout and various serializable types"
);
