// Copyright (c) Kobe Vrijsen 2022
// Licensed under the EUPL-1.2-or-later

#include <set>

#include "SerializerIostreamHelper.h"
#include "ConstexprSerializerBuffer.h"

// use layout without namespace calls all the time
using serializer_helper::Layout;

// An example of how to define a layout
using MyLayout = Layout<
	std::vector<std::string>,
	std::array<float, 12>,
	long
>;

// An example of a class we like to use
class WeirdObject
{
public:

	/*other code*/

	// reading/writing

	// Object write implementation
	friend bool write(std::ofstream& os, WeirdObject const& object)
	{
		return WeirdObject::Layout::Write(os, object.m_Names, object.m_Heights, object.m_Measurements);
	}

	// Object read implementaion
	friend bool read(std::ifstream& is, WeirdObject& object)
	{
		return WeirdObject::Layout::Read(is, object.m_Names, object.m_Heights, object.m_Measurements);
	}

	// Make the function freind so it's global and can be called by  Layout::write()  internally

private:

	// example data memebers
	std::vector<std::string> m_Names;
	std::array<float, 12>    m_Heights;
	long					 m_Measurements;

	// Object read/write layout
	using Layout = Layout<
		decltype(m_Names),
		decltype(m_Heights),
		decltype(m_Measurements)
	>;

};

int main()
{
;

	// loose objects
	std::vector<std::string> names{ "ann", "joseph", "catherine" };
	std::array<float, 12> heights{ 2, 3, 5, 7, 11, 13, 17, 23, 29, 31, 37, 43 };
	long measurements{ 1234 };

	// some of our weird class objects
	WeirdObject weird1{}, weird2{}, weird3{};

	{
		// File stream
		std::ofstream file{ "file.bin", std::ios::out | std::ios::binary };

		// Write a previously defined layout
		auto result = MyLayout::Write(file, names, heights, measurements);
		if (!result)
			return -1;

		// Or define one on the spot
		result = Layout<WeirdObject, WeirdObject, WeirdObject, std::string, int>::Write(file, weird1, weird2, weird3, "Some string, idk", 1009);
		if (!result)
			return -1;
	}

	{
		// Same for reading.

		// Just copy the layout

		std::ifstream file{ "file.bin", std::ios::in | std::ios::binary };

		auto result = MyLayout::Read(file, names, heights, measurements);
		if (!result)
			return -1;

		std::string str{};
		int i{};

		result = Layout<WeirdObject, WeirdObject, WeirdObject, std::string, int> ::Read(file, weird1, weird2, weird3, str, i);
		if (!result)
			return -1;
	}


	// Example that uses use std::set with std::string

	using SetLayout = Layout<
		long,
		std::set<std::string>
	>;

	{
		std::set<std::string> names{ "Ann", "Joseph", "Catherine" };

		std::ofstream file{ "set.bin", std::ios::out | std::ios::binary };
		SetLayout::Write(file, 0xDEADFACE, names);
	}

	{
		std::set<std::string> names{};
		long num{};

		std::ifstream file{ "set.bin", std::ios::in | std::ios::binary };
		SetLayout::Read(file, num, names);
	}

	return 0;

}
