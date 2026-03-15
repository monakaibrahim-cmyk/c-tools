# c-tools

[![Build](https://github.com/monakaibrahim-cmyk/c-tools/actions/workflows/build.yml/badge.svg)](https://github.com/monakaibrahim-cmyk/c-tools/actions/workflows/build.yml)

A collection of Windows command-line utility tools.

## Tools

- **clear** - Clears the terminal screen
- **tree** - Displays directory structure in a tree format
- **touch** - Creates empty files or updates timestamps
- **grep** - Pattern matching utility (coming soon)

## Building

### Prerequisites

- CMake 3.10 or higher
- MinGW-w64 cross-compilation toolchain

### Build with CMake and MinGW

To build using the MinGW cross-compilation toolchain:

```bash
cmake -DCMAKE_TOOLCHAIN_FILE=../mingw-toolchain.cmake -DCMAKE_BUILD_TYPE=Debug -B build
make -C build
```

This will:
1. Configure the project using the MinGW toolchain file
2. Build in Debug mode
3. Create the build directory and compile all tools

### Build Options

| Option | Description | Values |
|--------|-------------|--------|
| `CMAKE_BUILD_TYPE` | Build type | `Debug`, `Release` |
| `CMAKE_TOOLCHAIN_FILE` | Cross-compilation toolchain | Path to toolchain file |

### Build on Windows (Native)

For native builds on Windows with MSVC or MinGW:

```bash
cmake -DCMAKE_BUILD_TYPE=Release -B build
cmake --build build
```

## CI/CD

This project uses GitHub Actions for continuous integration. The workflow builds the project using MinGW in Debug mode.

See [`.github/workflows/build.yml`](.github/workflows/build.yml) for details.
