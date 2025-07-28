set(MINIMP3_INCLUDE_DIR ${INCLUDES_BASE_DIR}/minimp3)
set(MIKKTSPACE_INCLUDE_DIR ${INCLUDES_BASE_DIR}/mikktspace)

add_library(fmt.hdll
    fmt.c
    sha1.c
    dxt.c
    mikkt.c
    ${MIKKTSPACE_INCLUDE_DIR}/mikktspace.c
)

if (WIN32 OR ANDROID)
    set(PNG_INCLUDE_DIRS ${INCLUDES_BASE_DIR}/png)
    set(VORBIS_INCLUDE_DIR ${INCLUDES_BASE_DIR}/vorbis)

    target_sources(fmt.hdll PRIVATE
        ${INCLUDES_BASE_DIR}/png/png.c
        ${INCLUDES_BASE_DIR}/png/pngerror.c
        ${INCLUDES_BASE_DIR}/png/pngget.c
        ${INCLUDES_BASE_DIR}/png/pngmem.c
        ${INCLUDES_BASE_DIR}/png/pngpread.c
        ${INCLUDES_BASE_DIR}/png/pngread.c
        ${INCLUDES_BASE_DIR}/png/pngrio.c
        ${INCLUDES_BASE_DIR}/png/pngrtran.c
        ${INCLUDES_BASE_DIR}/png/pngrutil.c
        ${INCLUDES_BASE_DIR}/png/pngset.c
        ${INCLUDES_BASE_DIR}/png/pngtrans.c
        ${INCLUDES_BASE_DIR}/png/pngwio.c
        ${INCLUDES_BASE_DIR}/png/pngwrite.c
        ${INCLUDES_BASE_DIR}/png/pngwtran.c
        ${INCLUDES_BASE_DIR}/png/pngwutil.c
        ${INCLUDES_BASE_DIR}/vorbis/bitrate.c
        ${INCLUDES_BASE_DIR}/vorbis/bitwise.c
        ${INCLUDES_BASE_DIR}/vorbis/block.c
        ${INCLUDES_BASE_DIR}/vorbis/codebook.c
        ${INCLUDES_BASE_DIR}/vorbis/envelope.c
        ${INCLUDES_BASE_DIR}/vorbis/floor0.c
        ${INCLUDES_BASE_DIR}/vorbis/floor1.c
        ${INCLUDES_BASE_DIR}/vorbis/framing.c
        ${INCLUDES_BASE_DIR}/vorbis/info.c
        ${INCLUDES_BASE_DIR}/vorbis/lookup.c
        ${INCLUDES_BASE_DIR}/vorbis/lpc.c
        ${INCLUDES_BASE_DIR}/vorbis/lsp.c
        ${INCLUDES_BASE_DIR}/vorbis/mapping0.c
        ${INCLUDES_BASE_DIR}/vorbis/mdct.c
        ${INCLUDES_BASE_DIR}/vorbis/psy.c
        ${INCLUDES_BASE_DIR}/vorbis/registry.c
        ${INCLUDES_BASE_DIR}/vorbis/res0.c
        ${INCLUDES_BASE_DIR}/vorbis/sharedbook.c
        ${INCLUDES_BASE_DIR}/vorbis/smallft.c
        ${INCLUDES_BASE_DIR}/vorbis/synthesis.c
        ${INCLUDES_BASE_DIR}/vorbis/vorbisfile.c
        ${INCLUDES_BASE_DIR}/vorbis/window.c
    )

    if (ANDROID)
        # TODO: enable
        target_compile_definitions(fmt.hdll PRIVATE PNG_ARM_NEON_OPT=0)
    endif()
endif()

if(WIN32)
    set(ZLIB_INCLUDE_DIRS ${INCLUDES_BASE_DIR}/zlib)

    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(TurboJPEG_INCLUDE_DIRS ${INCLUDES_BASE_DIR}/turbojpeg ${INCLUDES_BASE_DIR}/turbojpeg/x64)
        find_library(TurboJPEG_LIBRARIES simd PATHS ${INCLUDES_BASE_DIR}/turbojpeg/x64)
    else()
        set(TurboJPEG_INCLUDE_DIRS ${INCLUDES_BASE_DIR}/turbojpeg ${INCLUDES_BASE_DIR}/turbojpeg/x86)
        find_library(TurboJPEG_LIBRARIES simd PATHS ${INCLUDES_BASE_DIR}/turbojpeg/x86)
    endif()

    target_sources(fmt.hdll PRIVATE
        ${INCLUDES_BASE_DIR}/turbojpeg/jaricom.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jcapimin.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jcapistd.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jcarith.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jccoefct.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jccolor.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jcdctmgr.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jchuff.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jcinit.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jcmainct.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jcmarker.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jcmaster.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jcomapi.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jcparam.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jcphuff.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jcprepct.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jcsample.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jctrans.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jdapimin.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jdapistd.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jdarith.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jdatadst-tj.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jdatadst.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jdatasrc-tj.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jdatasrc.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jdcoefct.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jdcolor.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jddctmgr.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jdhuff.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jdinput.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jdmainct.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jdmarker.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jdmaster.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jdmerge.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jdphuff.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jdpostct.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jdsample.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jdtrans.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jerror.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jfdctflt.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jfdctfst.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jfdctint.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jidctflt.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jidctfst.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jidctint.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jidctred.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jmemmgr.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jmemnobs.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jquant1.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jquant2.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jsimd.c
        ${INCLUDES_BASE_DIR}/turbojpeg/jutils.c
        ${INCLUDES_BASE_DIR}/turbojpeg/transupp.c
        ${INCLUDES_BASE_DIR}/turbojpeg/turbojpeg.c
        ${INCLUDES_BASE_DIR}/zlib/adler32.c
        ${INCLUDES_BASE_DIR}/zlib/crc32.c
        ${INCLUDES_BASE_DIR}/zlib/deflate.c
        ${INCLUDES_BASE_DIR}/zlib/inffast.c
        ${INCLUDES_BASE_DIR}/zlib/inflate.c
        ${INCLUDES_BASE_DIR}/zlib/inftrees.c
        ${INCLUDES_BASE_DIR}/zlib/trees.c
        ${INCLUDES_BASE_DIR}/zlib/zutil.c
    )
elseif(ANDROID)
    find_package(ZLIB REQUIRED)

	ExternalProject_Add(turbojpeg-project
        URL https://github.com/libjpeg-turbo/libjpeg-turbo/releases/download/3.1.1/libjpeg-turbo-3.1.1.tar.gz
        URL_HASH SHA256=aadc97ea91f6ef078b0ae3a62bba69e008d9a7db19b34e4ac973b19b71b4217c
        CMAKE_ARGS
            -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
            -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
            -DANDROID_PLATFORM=${ANDROID_PLATFORM}
            -DANDROID_ABI=${CMAKE_ANDROID_ARCH_ABI}
            -DBUILD_SHARED_LIBS=OFF
        # INSTALL_BYPRODUCTS in CMake 3.26+
        BUILD_BYPRODUCTS <INSTALL_DIR>/${CMAKE_INSTALL_LIBDIR}/libturbojpeg.a
        DOWNLOAD_EXTRACT_TIMESTAMP true
    )

    ExternalProject_Get_Property(turbojpeg-project INSTALL_DIR)

    add_library(turbojpeg STATIC IMPORTED)
    set_target_properties(turbojpeg PROPERTIES IMPORTED_LOCATION ${INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR}/libturbojpeg.a)

    set(TurboJPEG_INCLUDE_DIRS ${INSTALL_DIR}/${CMAKE_INSTALL_INCLUDEDIR})
    set(TurboJPEG_LIBRARIES turbojpeg)

    add_dependencies(turbojpeg turbojpeg-project)
else()
    find_package(ZLIB REQUIRED)
    find_package(PNG REQUIRED)

    find_package(TurboJPEG QUIET)
    if(NOT TurboJPEG_FOUND)
        pkg_check_modules(TurboJPEG REQUIRED libjpeg)
    endif()

    find_package(OggVorbis QUIET)
    if(NOT OGGVORBIS_FOUND)
        pkg_check_modules(OGGVORBIS REQUIRED vorbis vorbisenc vorbisfile)
    endif()
endif()

set_as_hdll(fmt)

target_include_directories(fmt.hdll
    PRIVATE
    ${ZLIB_INCLUDE_DIRS}
    ${PNG_INCLUDE_DIRS}
    ${TurboJPEG_INCLUDE_DIRS}
    ${VORBIS_INCLUDE_DIR}
    ${MINIMP3_INCLUDE_DIR}
    ${MIKKTSPACE_INCLUDE_DIR}
)

target_link_libraries(fmt.hdll
    libhl
    ${ZLIB_LIBRARIES}
    ${PNG_LIBRARIES}
    ${TurboJPEG_LIBRARIES}
    ${OGGVORBIS_LIBRARIES}
)

target_compile_definitions(fmt.hdll
    PRIVATE
    ${PNG_DEFINITIONS}
)

install(
    TARGETS
        fmt.hdll
    DESTINATION ${HDLL_DESTINATION}
)
