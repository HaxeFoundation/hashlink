# HashLink

[![TravisCI Build Status](https://travis-ci.org/HaxeFoundation/hashlink.svg?branch=master)](https://travis-ci.org/HaxeFoundation/hashlink)
[![AppVeyor Build Status](https://ci.appveyor.com/api/projects/status/github/HaxeFoundation/hashlink?branch=master&svg=true)](https://ci.appveyor.com/project/HaxeFoundation/hashlink)

HashLink is a virtual machine for Haxe https://hashlink.haxe.org


## Building on Linux/OSX

HashLink is distributed with some graphics libraries allowing to develop various applications, you can manually disable the libraries you want to compile in Makefile. Here's the dependencies that you install in order to compile all the libraries:

fmt: libpng-dev libturbojpeg-dev libvorbis-dev
openal: libopenal-dev
sdl: libsdl2-dev
ssl: libmbedtls-dev
uv: libuv1-dev

To install all dependencies on the latest Ubuntu, for example:

`sudo apt-get install libpng16-dev libturbojpeg-dev libvorbis-dev libopenal-dev libsdl2-dev libmbedtls-dev libuv1-dev`

For 16.04, see [this note](https://github.com/HaxeFoundation/hashlink/issues/147).

**And on OSX:**

`brew install libpng jpeg-turbo libvorbis sdl2 mbedtls openal-soft libuv`

Once dependencies are installed you can simply call:

`make`

To install hashlink binaries on your system you can then call:

`make install`

## Building on Windows

Open `hl.sln` using Visual Studio Code and compile.

To build all of HashLink libraries it is required to download several additional distributions, read each library README file (in hashlink/libs/xxx/README.md) for additional information.

## Debugging

You can debug Haxe/HashLink applications by using the [Visual Studio Code Debugger](https://marketplace.visualstudio.com/items?itemName=HaxeFoundation.haxe-hl)

## Documentation

Read the [Documentation](https://github.com/HaxeFoundation/hashlink/wiki) on the HashLink wiki.
