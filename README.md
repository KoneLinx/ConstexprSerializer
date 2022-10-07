
# Compile time serializer & tests

# Testing

The entire serializer is has been writen in a constexpr correct way. Therefore we can run tests in a static assert. If the code would ever throw or give thre wrong result. The code will fail to compile.

This also means that the code is **free of any Undefined Behaviour**.
