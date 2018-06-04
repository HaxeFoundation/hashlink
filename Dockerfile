FROM debian:stretch

RUN apt-get update && apt-get install -y --no-install-recommends \
        cmake \
        make \
        gcc \
        libz-dev \
        zlib1g-dev \
        libpng-dev \
        libsdl2-dev \
        libvorbis-dev \
        libalut-dev \
        libmbedtls-dev \
        libturbojpeg0-dev \
        libuv1-dev \
        libopenal-dev \
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
ENV ARCH=64