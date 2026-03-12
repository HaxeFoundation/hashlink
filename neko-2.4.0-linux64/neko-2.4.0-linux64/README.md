![NekoVM](https://cloud.githubusercontent.com/assets/576184/14234981/10528a0e-f9f1-11e5-8922-894569b2feea.png)

[![Build Status](https://dev.azure.com/HaxeFoundation/GitHubPublic/_apis/build/status/HaxeFoundation.neko?branchName=master)](https://dev.azure.com/HaxeFoundation/GitHubPublic/_build/latest?definitionId=2&branchName=master)

# Deprecated as of 2021-09-09

**Neko is not actively maintained anymore.**

We keep it compatible with existing Haxe standard library and Haxe language features. But don't expect any new features in Neko itself and don't expect implementation of any new Haxe standard library API.

# Neko Virtual Machine

See https://nekovm.org/

## Snapshot Builds

Compiled binaries can be found in the "artifacts" link in the summary section of each [Azure Pipelines build](https://dev.azure.com/HaxeFoundation/GitHubPublic/_build?definitionId=2&_a=summary&repositoryFilter=2&branchFilter=14&statusFilter=succeeded).

For macOS, Neko snapshot of the latest master branch can be built using [homebrew](https://brew.sh/) in a single command: `brew install neko --HEAD`. It will install required dependencies, build, and install Neko to the system. The binaries can be found at `brew --prefix neko`. Use `brew reinstall neko --HEAD` to upgrade in the future.

Ubuntu users can use the [Haxe Foundation snapshots PPA](https://launchpad.net/~haxe/+archive/ubuntu/snapshots) to install a Neko package built from the latest master branch. To do so, run the commands as follows:
```
sudo add-apt-repository ppa:haxe/snapshots -y
sudo apt-get update
sudo apt-get install neko -y
```

Users of other Linux/FreeBSD distributions should build Neko from source. See below for additional instructions.

## Build instruction

Neko can be built using CMake (version 3.x is recommended) and one of the C compilers listed as follows:

 * Windows: Visual Studio 2010 / 2013 / 2015 / 2017
 * Mac: XCode (with its "Command line tools")
 * Linux: gcc (can be obtained by installing the "build-essential" Debian/Ubuntu package)

Neko needs to link with various third-party libraries, which are summarized as follows:

| library / tool                          | OS          | Debian/Ubuntu package                                     |
|-----------------------------------------|-------------|-----------------------------------------------------------|
| Boehm GC                                | all         | libgc-dev                                                 |
| OpenSSL                                 | all         | libssl-dev                                                |
| pcre2                                   | all         | libpcre2-dev                                              |
| zlib                                    | all         | zlib1g-dev                                                |
| Apache 2.2 / 2.4, with apr and apr-util | all         | apache2-dev                                               |
| MariaDB / MySQL (Connector/C)           | all         | libmariadb-client-lgpl-dev-compat (or libmysqlclient-dev) |
| SQLite                                  | all         | libsqlite3-dev                                            |
| mbed TLS                                | all         | libmbedtls-dev                                            |
| GTK+3                                   | Linux       | libgtk-3-dev                                              |

On Windows, CMake will automatically download and build the libraries in the build folder during the build process. However, you need to install [Perl](https://www.activestate.com/activeperl) manually because OpenSSL needs it for configuration. On Mac/Linux, you should install the libraries manually to your system before building Neko, or use the `STATIC_DEPS` CMake option, which will be explained in [CMake options](#cmake-options).

### Building on Mac/Linux

```shell
# make a build directory, and change to it
mkdir build
cd build

# run cmake
cmake ..

# let's build, the outputs can be located in the "bin" directory
make

# install it if you want
# default installation prefix is /usr/local
make install
```

### Building on Windows

Below is the instructions of building Neko in a Visual Studio command prompt.
You may use the CMake GUI and Visual Studio to build it instead.

```shell
# make a build directory, and change to it
mkdir build
cd build

# run cmake specifying the visual studio version you need
# Visual Studio 12 2013, Visual Studio 14 2015, Visual Studio 15 2017
# you can additionally specify platform via -A switch (x86, x64)
cmake -G "Visual Studio 12 2013" ..

# let's build, the outputs can be located in the "bin" directory
msbuild ALL_BUILD.vcxproj /p:Configuration=Release

# install it if you want
# default installation location is C:\HaxeToolkit\neko
msbuild INSTALL.vcxproj /p:Configuration=Release
```

### CMake options

A number of options can be used to customize the build. They can be specified in the CMake GUI, or passed to `cmake` in command line as follows:

```shell
cmake "-Doption=value" ..
```

#### NDLLs

Settings that allow to exclude libraries and their dependencies from the build; available on all platforms. By default all are `ON`:

- `WITH_REGEXP` - Build Perl-compatible regex support
- `WITH_UI` - Build GTK-3 UI support
- `WITH_SSL` - Build SSL support
- `WITH_MYSQL` - Build MySQL support
- `WITH_SQLITE` - Build Sqlite support
- `WITH_APACHE` - Build Apache modules

#### `STATIC_DEPS`

Default value: `all` for Windows, `none` otherwise

It defines the dependencies that should be linked statically. Can be `all`, `none`, or a list of library names (e.g. `BoehmGC;Zlib;OpenSSL;MariaDBConnector;pcre2;SQLite3;APR;APRutil;Apache;MbedTLS`).

CMake will automatically download and build the specified dependencies into the build folder. If a library is not present in this list, it should be installed manually, and it will be linked dynamically.

All third-party libraries, except GTK+3 (Linux) and BoehmGC on Windows, can be linked statically. We do not support statically linking GTK+3 due to the difficulty of building it and its own dependencies. Additionally, we do not support statically linking the BoehmGC library on Windows systems.

#### `RELOCATABLE`

Available on Mac/Linux. Default value: `ON`

Set RPATH to `$ORIGIN` (Linux) / `@executable_path` (Mac). It allows the resulting Neko VM executable to locate libraries (e.g. "libneko" and ndll files) in its local directory, such that the libraries need not be installed to "/usr/lib" or "/usr/local/lib".

#### `NEKO_JIT_DISABLE`

Default `OFF`.

Disable Neko JIT. By default, Neko JIT will be enabled for platforms it supports. Setting this to `ON` disable JIT for all platforms.

#### `NEKO_JIT_DEBUG`

Default `OFF`.

Debug Neko JIT.

#### `RUN_LDCONFIG`

Available on Linux. Default value: `ON`

Whether to run `ldconfig` automatically after `make install`. It is for refreshing the shared library cache such that "libneko" can be located correctly by the Neko VM.
