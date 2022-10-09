
# Compile time serializer & tests

Fully constexpr and verified by static assrts. Serializer requires at least C++20 to compile, The Serializer helper compatible with iostreams can be compiled using C++17.

```cpp
static_assert(
	[]
	{
		// Layout
		using MyLayout = Layout<std::wstring, std::vector<short>>;
		// Buffer
		Serializer io(128);
		// Write
		MyLayout::Write(io, L"ind", { 7, 13, 3, 31 });
		// Read
		auto const [key, val] = MyLayout::Read(io);
		// Correct?
		return key == L"ind" 
			&& std::ranges::equal(val, std::initializer_list{ 7, 13, 3, 31 });
	} 
	(),
	"Serializing a vector and wstring"
);
```

### Testing

The entire serializer is has been writen in a constexpr correct way. Therefore we can run tests in a static assert. If the code would ever throw or give thre wrong result. The code will fail to compile.

This also means that the code is **free of any Undefined Behaviour**.

Tests ares located in the .cpp files

### Serializer

A buffer allowing you to write to and read from.

`Serializer::read`  Read objects from the buffer

`Serializer::write` Write objects to the buffer

`Serializer::clear` Clears the buffer

To construct a fixed size buffer on the stack

`Serializer<1024>{}`

To construct with size not known on the heap

`Serializer(1024)`

*Initialiser list initialisation will not work*

A serializer object can be moved but not copied. The fixed size Serializer is trivially copyable if you do decide to deep copy it.
