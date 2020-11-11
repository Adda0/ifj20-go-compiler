# IFJ20 compiler – a Go-inspired language compiler

A compiler created for a [term project](http://www.fit.vutbr.cz/study/courses/IFJ/public/project/) in the [IFJ course](https://www.fit.vut.cz/study/course/IFJ/.en).

## Building

The compiler project is written in C. The tests are written in C++, using [GoogleTest](https://github.com/google/googletest).

Both the project and the tests may be built using CMake. The target compiler is GCC and G++, version 9+. 
MSVC support is experimental and not guaranteed. 

A Makefile for GNU Make is included that encapsulates the typical actions performed on the project. 
You can run one of the following targets:
- `make all` or `make compiler` to build the compiler,
- `make test` to run the compiler tests,
- `make doc` to generate the documentation (_TODO_).

The non-test targets in the Makefile may be used without CMake. The `test` target, however, triggers a CMake build. 
This approach has been chosen to ensure compliance with the assignment, while keeping the build process as simple as possible.

## Authors
#### Pracovní skupina Sluníčka
František Nečas (xnecas27) \
David Chocholatý (xchoch08) \
Ondřej Ondryáš (xondry02)

## License
IFJ-proj: An IFJ20 language compiler

MIT License \
Copyright (C) 2020 František Nečas, David Chocholatý, Ondřej Ondryáš \
See [LICENSE](https://github.com/Adda0/ifj20-go-compiler/blob/master/LICENSE)
