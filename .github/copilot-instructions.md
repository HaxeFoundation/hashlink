# HashLink – Copilot Agent Instructions

## Project Overview

HashLink is a virtual machine for the [Haxe](https://haxe.org) programming language. It executes `.hl` bytecode files produced by the Haxe compiler. This repository contains:

- The HashLink VM (`hl` binary) and runtime library (`libhl.so`)
- Optional native extension libraries (`fmt.hdll`, `sdl.hdll`, `ssl.hdll`, `openal.hdll`, `uv.hdll`, `mysql.hdll`, `sqlite.hdll`, `heaps.hdll`)
- Source code in `src/` and library implementations in `libs/`
- Test programs in `other/tests/`
- CMake build support alongside the classic `Makefile`

## Building HashLink on Linux

### Install system dependencies

```bash
sudo apt-get update -y
sudo apt-get install --no-install-recommends -y \
  libmbedtls-dev \
  libopenal-dev \
  libpng-dev \
  libsdl2-dev \
  libturbojpeg-dev \
  libuv1-dev \
  libvorbis-dev \
  libsqlite3-dev
```

### Build and install

```bash
make
sudo make install
sudo ldconfig
```

This installs the `hl` binary to `/usr/local/bin` and `libhl.so` plus the `.hdll` extension libraries to `/usr/local/lib`.

## Compiling and Running a Haxe Program

Haxe source files are compiled to HashLink bytecode with the Haxe compiler, then executed with `hl`:

```bash
# Compile a Haxe program to HashLink bytecode
haxe --hl program.hl -cp <source-dir> -m <MainClass>

# Run the bytecode
hl program.hl
```

Example using the test program included in the repository:

```bash
haxe --hl hello.hl -cp other/tests -m HelloWorld
hl hello.hl
```

## Running Tests

The main test in the build workflow:

```bash
haxe -hl hello.hl -cp other/tests -main HelloWorld -D interp
hl hello.hl
```

To test the C output path:

```bash
haxe -hl src/_main.c -cp other/tests -main HelloWorld
make hlc
./hlc
```

## Key Source Files and Directories

| Path | Description |
|------|-------------|
| `src/` | VM core: JIT compiler, GC, module loader, debugger |
| `src/hl.h` | Main public header for embedding HashLink |
| `libs/` | Native extension libraries (fmt, sdl, ssl, openal, uv, mysql, sqlite, heaps) |
| `include/` | Vendored third-party headers (pcre2, mbedtls, sdl, etc.) |
| `other/tests/` | Haxe test programs |
| `other/haxelib/` | HashLink haxelib package sources |
| `Makefile` | Primary build file for Linux/macOS |
| `CMakeLists.txt` | CMake build file (cross-platform) |

## Code Style and Conventions

- The VM and libraries are written in **C11** (`-std=c11`).
- Use `${CC}` for C compilation; `${CXX}` for C++ (only in the `heaps` library).
- Follow the existing pattern of adding new native functions via `HL_PRIM` macros defined in `src/hl.h`.
- New native libraries should be placed under `libs/<name>/` and expose a `DEFINE_PRIM` table.
