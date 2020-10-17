# IFJ20 compiler - Go language inspired compiler

A compiler created for a [term project](http://www.fit.vutbr.cz/study/courses/IFJ/public/project/) in an [IFJ course](https://www.fit.vut.cz/study/course/IFJ/.en).

## Project structure

The project consists of the following parts:
- `foo`: bar

The `src` directory also contains:
- Doxygen files to generate HTML documentation for the app and the library,
- a Makefile (see below),

## Building

The whole project is written in C, using GTest as its testing framework. You can either
use your IDE of choice (like Visual Studio or Clion) to build the project or the CLI may be used in the typical manner.

A Makefile for GNU Make is included that encapsulates the typical actions performed on the project. You can run one of the following targets:
- `make foo` to bar,
- `make test` to run the compiler tests,
- `make comp` to build the compiler,
- `make doc` to generate the documentation.

## Authors
#### Pracovní skupina Sluníčka
František Nečas (xnecas27) \
David Chocholatý (xchoch08)
Ondřej Ondryáš (xondry02) \

## License
IFJ-proj: An IFJ20 language compiler

MIT License \
Copyright (C) 2020 František Nečas, David Chocholatý, Ondřej Ondryáš \
See [LICENSE](https://github.com/Adda0/ifj20-go-compiler/blob/master/LICENSE)
