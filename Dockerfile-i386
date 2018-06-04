FROM debian:stretch

RUN dpkg --add-architecture i386 && apt-get update && apt-get install -y --no-install-recommends \
        cmake \
        make \
        gcc-multilib \
        libz-dev:i386 \
        zlib1g-dev:i386 \
        libpng-dev:i386 \
        libsdl2-dev:i386 \
        libvorbis-dev:i386 \
        libalut-dev:i386 \
        libmbedtls-dev:i386 \
        libturbojpeg0-dev:i386 \
        libuv1-dev:i386 \
        libopenal-dev:i386 \
        neko \
        curl \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

ENV HAXE_STD_PATH=/usr/local/share/haxe/std
RUN set -ex \
    && mkdir /haxe \
    && cd /haxe \
    && curl -sSL https://build.haxe.org/builds/haxe/linux64/haxe_latest.tar.gz | tar -x -z --strip-components=1 -f - \
    && ln -s /haxe/haxe /haxe/haxelib /usr/local/bin/ \
    && mkdir -p `dirname "$HAXE_STD_PATH"` \
    && ln -s /haxe/std "$HAXE_STD_PATH" \
    && cd ..

RUN set -ex \
    && mkdir /haxelib \
    && haxelib setup /haxelib

RUN haxelib install hashlink

COPY . /hashlink
WORKDIR /hashlink
ENV ARCH=32