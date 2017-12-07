# HashLink

[![TravisCI Build Status](https://travis-ci.org/HaxeFoundation/hashlink.svg?branch=master)](https://travis-ci.org/HaxeFoundation/hashlink)
[![AppVeyor Build Status](https://ci.appveyor.com/api/projects/status/github/HaxeFoundation/hashlink?branch=master&svg=true)](https://ci.appveyor.com/project/HaxeFoundation/hashlink)

HashLink is a virtual machine for Haxe http://hashlink.haxe.org


# Building on Linux/OSX

HashLink is distributed with some graphics libraries allowing to develop various applications, you can manually disable the libraries you want to compile in Makefile. Here's the dependencies that you install in order to compile all the libraries:

fmt: libpng-dev libturbojpeg-dev libvorbis-dev
openal: libopenal-dev
sdl: libsdl2-dev
ssl: libmbedtls-dev
uv: libuv1-dev

To install all dependencies on Ubuntu for example:

`sudo apt-get install libpng-dev libturbojpeg-dev libvorbis-dev libopenal-dev libsdl2-dev libmbedtls-dev libuv1-dev`

# Documentation

Read the [Documentation](https://github.com/HaxeFoundation/hashlink/wiki) on the HashLink wiki.
