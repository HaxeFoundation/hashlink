set(MINIMP3_INCLUDE_DIR ${INCLUDES_BASE_DIR}/minimp3)
set(MIKKTSPACE_INCLUDE_DIR ${INCLUDES_BASE_DIR}/mikktspace)

add_library(fmt.hdll
    fmt.c
    sha1.c
    dxt.c
    mikkt.c
    ${MIKKTSPACE_INCLUDE_DIR}/mikktspace.c
)

if (WIN32)
    set(PNG_INCLUDE_DIRS ${INCLUDES_BASE_DIR}/png)
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
        ${INCLUDES_BASE_DIR}/png/pngwutil.c)
endif()

if (WIN32)
    set(VORBIS_INCLUDE_DIR ${INCLUDES_BASE_DIR}/vorbis)

    target_sources(fmt.hdll PRIVATE
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
endif()

set(OPUSFILE_INCLUDE_DIR ${INCLUDES_BASE_DIR}/opusfile)
set(OPUS_SRC_DIR ${INCLUDES_BASE_DIR}/opus-src)
set(OPUS_INCLUDE_DIR ${OPUS_SRC_DIR}/include)

target_sources(fmt.hdll PRIVATE
    ${INCLUDES_BASE_DIR}/opusfile/opusfile.c
    ${INCLUDES_BASE_DIR}/opusfile/info.c
    ${INCLUDES_BASE_DIR}/opusfile/stream.c
    ${INCLUDES_BASE_DIR}/opusfile/internal.c
)
target_compile_definitions(fmt.hdll PRIVATE OP_HAVE_LRINTF)

if(WIN32)
    target_sources(fmt.hdll PRIVATE
        ${OPUS_SRC_DIR}/src/opus.c
        ${OPUS_SRC_DIR}/src/opus_decoder.c
        ${OPUS_SRC_DIR}/src/opus_encoder.c
        ${OPUS_SRC_DIR}/src/extensions.c
        ${OPUS_SRC_DIR}/src/opus_multistream.c
        ${OPUS_SRC_DIR}/src/opus_multistream_encoder.c
        ${OPUS_SRC_DIR}/src/opus_multistream_decoder.c
        ${OPUS_SRC_DIR}/src/repacketizer.c
        ${OPUS_SRC_DIR}/src/opus_projection_encoder.c
        ${OPUS_SRC_DIR}/src/opus_projection_decoder.c
        ${OPUS_SRC_DIR}/src/mapping_matrix.c
        ${OPUS_SRC_DIR}/src/analysis.c
        ${OPUS_SRC_DIR}/src/mlp.c
        ${OPUS_SRC_DIR}/src/mlp_data.c
        ${OPUS_SRC_DIR}/celt/bands.c
        ${OPUS_SRC_DIR}/celt/celt.c
        ${OPUS_SRC_DIR}/celt/celt_encoder.c
        ${OPUS_SRC_DIR}/celt/celt_decoder.c
        ${OPUS_SRC_DIR}/celt/cwrs.c
        ${OPUS_SRC_DIR}/celt/entcode.c
        ${OPUS_SRC_DIR}/celt/entdec.c
        ${OPUS_SRC_DIR}/celt/entenc.c
        ${OPUS_SRC_DIR}/celt/kiss_fft.c
        ${OPUS_SRC_DIR}/celt/laplace.c
        ${OPUS_SRC_DIR}/celt/mathops.c
        ${OPUS_SRC_DIR}/celt/mdct.c
        ${OPUS_SRC_DIR}/celt/modes.c
        ${OPUS_SRC_DIR}/celt/pitch.c
        ${OPUS_SRC_DIR}/celt/celt_lpc.c
        ${OPUS_SRC_DIR}/celt/quant_bands.c
        ${OPUS_SRC_DIR}/celt/rate.c
        ${OPUS_SRC_DIR}/celt/vq.c
        ${OPUS_SRC_DIR}/silk/CNG.c
        ${OPUS_SRC_DIR}/silk/code_signs.c
        ${OPUS_SRC_DIR}/silk/init_decoder.c
        ${OPUS_SRC_DIR}/silk/decode_core.c
        ${OPUS_SRC_DIR}/silk/decode_frame.c
        ${OPUS_SRC_DIR}/silk/decode_parameters.c
        ${OPUS_SRC_DIR}/silk/decode_indices.c
        ${OPUS_SRC_DIR}/silk/decode_pulses.c
        ${OPUS_SRC_DIR}/silk/decoder_set_fs.c
        ${OPUS_SRC_DIR}/silk/dec_API.c
        ${OPUS_SRC_DIR}/silk/enc_API.c
        ${OPUS_SRC_DIR}/silk/encode_indices.c
        ${OPUS_SRC_DIR}/silk/encode_pulses.c
        ${OPUS_SRC_DIR}/silk/gain_quant.c
        ${OPUS_SRC_DIR}/silk/interpolate.c
        ${OPUS_SRC_DIR}/silk/LP_variable_cutoff.c
        ${OPUS_SRC_DIR}/silk/NLSF_decode.c
        ${OPUS_SRC_DIR}/silk/NSQ.c
        ${OPUS_SRC_DIR}/silk/NSQ_del_dec.c
        ${OPUS_SRC_DIR}/silk/PLC.c
        ${OPUS_SRC_DIR}/silk/shell_coder.c
        ${OPUS_SRC_DIR}/silk/tables_gain.c
        ${OPUS_SRC_DIR}/silk/tables_LTP.c
        ${OPUS_SRC_DIR}/silk/tables_NLSF_CB_NB_MB.c
        ${OPUS_SRC_DIR}/silk/tables_NLSF_CB_WB.c
        ${OPUS_SRC_DIR}/silk/tables_other.c
        ${OPUS_SRC_DIR}/silk/tables_pitch_lag.c
        ${OPUS_SRC_DIR}/silk/tables_pulses_per_block.c
        ${OPUS_SRC_DIR}/silk/VAD.c
        ${OPUS_SRC_DIR}/silk/control_audio_bandwidth.c
        ${OPUS_SRC_DIR}/silk/quant_LTP_gains.c
        ${OPUS_SRC_DIR}/silk/VQ_WMat_EC.c
        ${OPUS_SRC_DIR}/silk/HP_variable_cutoff.c
        ${OPUS_SRC_DIR}/silk/NLSF_encode.c
        ${OPUS_SRC_DIR}/silk/NLSF_VQ.c
        ${OPUS_SRC_DIR}/silk/NLSF_unpack.c
        ${OPUS_SRC_DIR}/silk/NLSF_del_dec_quant.c
        ${OPUS_SRC_DIR}/silk/process_NLSFs.c
        ${OPUS_SRC_DIR}/silk/stereo_LR_to_MS.c
        ${OPUS_SRC_DIR}/silk/stereo_MS_to_LR.c
        ${OPUS_SRC_DIR}/silk/check_control_input.c
        ${OPUS_SRC_DIR}/silk/control_SNR.c
        ${OPUS_SRC_DIR}/silk/init_encoder.c
        ${OPUS_SRC_DIR}/silk/control_codec.c
        ${OPUS_SRC_DIR}/silk/A2NLSF.c
        ${OPUS_SRC_DIR}/silk/ana_filt_bank_1.c
        ${OPUS_SRC_DIR}/silk/biquad_alt.c
        ${OPUS_SRC_DIR}/silk/bwexpander_32.c
        ${OPUS_SRC_DIR}/silk/bwexpander.c
        ${OPUS_SRC_DIR}/silk/debug.c
        ${OPUS_SRC_DIR}/silk/decode_pitch.c
        ${OPUS_SRC_DIR}/silk/inner_prod_aligned.c
        ${OPUS_SRC_DIR}/silk/lin2log.c
        ${OPUS_SRC_DIR}/silk/log2lin.c
        ${OPUS_SRC_DIR}/silk/LPC_analysis_filter.c
        ${OPUS_SRC_DIR}/silk/LPC_inv_pred_gain.c
        ${OPUS_SRC_DIR}/silk/table_LSF_cos.c
        ${OPUS_SRC_DIR}/silk/NLSF2A.c
        ${OPUS_SRC_DIR}/silk/NLSF_stabilize.c
        ${OPUS_SRC_DIR}/silk/NLSF_VQ_weights_laroia.c
        ${OPUS_SRC_DIR}/silk/pitch_est_tables.c
        ${OPUS_SRC_DIR}/silk/resampler.c
        ${OPUS_SRC_DIR}/silk/resampler_down2_3.c
        ${OPUS_SRC_DIR}/silk/resampler_down2.c
        ${OPUS_SRC_DIR}/silk/resampler_private_AR2.c
        ${OPUS_SRC_DIR}/silk/resampler_private_down_FIR.c
        ${OPUS_SRC_DIR}/silk/resampler_private_IIR_FIR.c
        ${OPUS_SRC_DIR}/silk/resampler_private_up2_HQ.c
        ${OPUS_SRC_DIR}/silk/resampler_rom.c
        ${OPUS_SRC_DIR}/silk/sigm_Q15.c
        ${OPUS_SRC_DIR}/silk/sort.c
        ${OPUS_SRC_DIR}/silk/sum_sqr_shift.c
        ${OPUS_SRC_DIR}/silk/stereo_decode_pred.c
        ${OPUS_SRC_DIR}/silk/stereo_encode_pred.c
        ${OPUS_SRC_DIR}/silk/stereo_find_predictor.c
        ${OPUS_SRC_DIR}/silk/stereo_quant_pred.c
        ${OPUS_SRC_DIR}/silk/LPC_fit.c
        ${OPUS_SRC_DIR}/silk/float/apply_sine_window_FLP.c
        ${OPUS_SRC_DIR}/silk/float/corrMatrix_FLP.c
        ${OPUS_SRC_DIR}/silk/float/encode_frame_FLP.c
        ${OPUS_SRC_DIR}/silk/float/find_LPC_FLP.c
        ${OPUS_SRC_DIR}/silk/float/find_LTP_FLP.c
        ${OPUS_SRC_DIR}/silk/float/find_pitch_lags_FLP.c
        ${OPUS_SRC_DIR}/silk/float/find_pred_coefs_FLP.c
        ${OPUS_SRC_DIR}/silk/float/LPC_analysis_filter_FLP.c
        ${OPUS_SRC_DIR}/silk/float/LTP_analysis_filter_FLP.c
        ${OPUS_SRC_DIR}/silk/float/LTP_scale_ctrl_FLP.c
        ${OPUS_SRC_DIR}/silk/float/noise_shape_analysis_FLP.c
        ${OPUS_SRC_DIR}/silk/float/process_gains_FLP.c
        ${OPUS_SRC_DIR}/silk/float/regularize_correlations_FLP.c
        ${OPUS_SRC_DIR}/silk/float/residual_energy_FLP.c
        ${OPUS_SRC_DIR}/silk/float/warped_autocorrelation_FLP.c
        ${OPUS_SRC_DIR}/silk/float/wrappers_FLP.c
        ${OPUS_SRC_DIR}/silk/float/autocorrelation_FLP.c
        ${OPUS_SRC_DIR}/silk/float/burg_modified_FLP.c
        ${OPUS_SRC_DIR}/silk/float/bwexpander_FLP.c
        ${OPUS_SRC_DIR}/silk/float/energy_FLP.c
        ${OPUS_SRC_DIR}/silk/float/inner_product_FLP.c
        ${OPUS_SRC_DIR}/silk/float/k2a_FLP.c
        ${OPUS_SRC_DIR}/silk/float/LPC_inv_pred_gain_FLP.c
        ${OPUS_SRC_DIR}/silk/float/pitch_analysis_core_FLP.c
        ${OPUS_SRC_DIR}/silk/float/scale_copy_vector_FLP.c
        ${OPUS_SRC_DIR}/silk/float/scale_vector_FLP.c
        ${OPUS_SRC_DIR}/silk/float/schur_FLP.c
        ${OPUS_SRC_DIR}/silk/float/sort_FLP.c
    )
    target_compile_definitions(fmt.hdll PRIVATE
        OPUS_BUILD
        USE_ALLOCA
        HAVE_LRINTF
        HAVE_CONFIG_H
    )
    target_include_directories(fmt.hdll PRIVATE
        ${OPUSFILE_INCLUDE_DIR}
        ${OPUS_INCLUDE_DIR}
        ${OPUS_SRC_DIR}
    )

	file(GLOB OPUS_SRC_FILES ${OPUS_SRC_DIR}/src/*.c)
    file(GLOB OPUS_CELT_FILES ${OPUS_SRC_DIR}/celt/*.c)
    file(GLOB OPUS_SILK_FILES ${OPUS_SRC_DIR}/silk/*.c)
    file(GLOB OPUS_SILK_FLOAT_FILES ${OPUS_SRC_DIR}/silk/float/*.c)
    set_source_files_properties(${OPUS_SRC_FILES} PROPERTIES
        INCLUDE_DIRECTORIES "${OPUS_SRC_DIR}/celt;${OPUS_SRC_DIR}/silk;${OPUS_SRC_DIR}/silk/float;${OPUS_INCLUDE_DIR};${OPUS_SRC_DIR}")
    set_source_files_properties(${OPUS_CELT_FILES} PROPERTIES
        INCLUDE_DIRECTORIES "${OPUS_SRC_DIR}/celt;${OPUS_INCLUDE_DIR};${OPUS_SRC_DIR}")
    set_source_files_properties(${OPUS_SILK_FILES} PROPERTIES
        INCLUDE_DIRECTORIES "${OPUS_SRC_DIR}/silk;${OPUS_SRC_DIR}/silk/float;${OPUS_SRC_DIR}/celt;${OPUS_INCLUDE_DIR};${OPUS_SRC_DIR}")
    set_source_files_properties(${OPUS_SILK_FLOAT_FILES} PROPERTIES
        INCLUDE_DIRECTORIES "${OPUS_SRC_DIR}/silk/float;${OPUS_SRC_DIR}/silk;${OPUS_SRC_DIR}/celt;${OPUS_INCLUDE_DIR};${OPUS_SRC_DIR}")
    set(OPUS_LIBRARIES)
else()
    target_include_directories(fmt.hdll PRIVATE
        ${OPUSFILE_INCLUDE_DIR}
        ${OPUS_INCLUDE_DIR}
    )

    include(ExternalProject)

    if(NOT DEFINED CMAKE_COMMON_DEP_ARGS)
        set(CMAKE_COMMON_DEP_ARGS
            -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
            -DCMAKE_POSITION_INDEPENDENT_CODE=ON
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
            -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
        )
    endif()

    ExternalProject_Add(libopus
        URL https://github.com/xiph/opus/releases/download/v1.5.2/opus-1.5.2.tar.gz
        URL_HASH SHA256=65c1d2f78b9f2fb20082c38cbe47c951ad5839345876e46941612ee87f9a7ce1
        CMAKE_ARGS
            ${CMAKE_COMMON_DEP_ARGS}
            -DBUILD_SHARED_LIBS=OFF
            -DCMAKE_INSTALL_LIBDIR=lib
            -DOPUS_BUILD_TESTING=OFF
            -DOPUS_BUILD_PROGRAMS=OFF
            -DOPUS_INSTALL_PKG_CONFIG_MODULE=OFF
            -DOPUS_INSTALL_CMAKE_CONFIG_MODULE=OFF
        BUILD_BYPRODUCTS <INSTALL_DIR>/lib/libopus.a
    )
    ExternalProject_Get_Property(libopus INSTALL_DIR)
    set(OPUS_INSTALL_DIR ${INSTALL_DIR})

    add_library(opus_lib STATIC IMPORTED)
    set_target_properties(opus_lib PROPERTIES IMPORTED_LOCATION ${OPUS_INSTALL_DIR}/lib/libopus.a)
    add_dependencies(opus_lib libopus)

    if(NOT DOWNLOAD_DEPENDENCIES)
        pkg_check_modules(OPUSFILE_OGG REQUIRED ogg)
        set(OPUS_OGG_LIBRARIES ${OPUSFILE_OGG_LINK_LIBRARIES})
    endif()

    set(OPUS_LIBRARIES opus_lib)
    if(NOT DOWNLOAD_DEPENDENCIES)
        list(APPEND OPUS_LIBRARIES ${OPUS_OGG_LIBRARIES})
    endif()
    list(APPEND OPUS_LIBRARIES m)

    add_dependencies(fmt.hdll libopus)
endif()

set(OPUS_ALL_INCLUDE_DIRS ${OPUSFILE_INCLUDE_DIR} ${OPUS_INCLUDE_DIR})

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
elseif(DOWNLOAD_DEPENDENCIES)
    find_package(ZLIB REQUIRED)

	ExternalProject_Add(turbojpeg-project
        URL https://github.com/libjpeg-turbo/libjpeg-turbo/releases/download/3.1.1/libjpeg-turbo-3.1.1.tar.gz
        URL_HASH SHA256=aadc97ea91f6ef078b0ae3a62bba69e008d9a7db19b34e4ac973b19b71b4217c
        CMAKE_ARGS
            ${CMAKE_COMMON_DEP_ARGS}
            -DBUILD_SHARED_LIBS=OFF
            -DCMAKE_INSTALL_LIBDIR=lib
        # INSTALL_BYPRODUCTS in CMake 3.26+
        BUILD_BYPRODUCTS <INSTALL_DIR>/lib/libturbojpeg.a
    )

    ExternalProject_Get_Property(turbojpeg-project INSTALL_DIR)

    add_library(turbojpeg STATIC IMPORTED)
    set_target_properties(turbojpeg PROPERTIES IMPORTED_LOCATION ${INSTALL_DIR}/lib/libturbojpeg.a)

    set(TurboJPEG_INCLUDE_DIRS ${INSTALL_DIR}/${CMAKE_INSTALL_INCLUDEDIR})
    set(TurboJPEG_LIBRARIES turbojpeg)

    add_dependencies(turbojpeg turbojpeg-project)

    ExternalProject_Add(libpng
        URL https://github.com/pnggroup/libpng/archive/refs/tags/v1.6.50.tar.gz
        URL_HASH SHA256=71158e53cfdf2877bc99bcab33641d78df3f48e6e0daad030afe9cb8c031aa46
        CMAKE_ARGS
            ${CMAKE_COMMON_DEP_ARGS}
            -DPNG_SHARED=OFF
            -DPNG_TESTS=OFF
            -DPNG_TOOLS=OFF
            -DPNG_FRAMEWORK=OFF
            -DCMAKE_INSTALL_LIBDIR=lib
        # INSTALL_BYPRODUCTS in CMake 3.26+
        BUILD_BYPRODUCTS <INSTALL_DIR>/lib/libpng.a
    )
    ExternalProject_Get_Property(libpng INSTALL_DIR)

    add_library(png STATIC IMPORTED)
    set_target_properties(png PROPERTIES IMPORTED_LOCATION ${INSTALL_DIR}/lib/libpng.a)

    set(PNG_INCLUDE_DIRS ${INSTALL_DIR}/${CMAKE_INSTALL_INCLUDEDIR})
    set(PNG_LIBRARIES png)

    add_dependencies(png libpng)

    ExternalProject_Add(libogg
        URL https://github.com/xiph/ogg/releases/download/v1.3.6/libogg-1.3.6.tar.gz
        URL_HASH SHA256=83e6704730683d004d20e21b8f7f55dcb3383cdf84c0daedf30bde175f774638
        CMAKE_ARGS
            ${CMAKE_COMMON_DEP_ARGS}
            -DCMAKE_INSTALL_LIBDIR=lib
        # INSTALL_BYPRODUCTS in CMake 3.26+
        BUILD_BYPRODUCTS <INSTALL_DIR>/lib/libogg.a
    )
    ExternalProject_Get_Property(libogg INSTALL_DIR)
    set(OGG_INSTALL_DIR ${INSTALL_DIR})

    set(OGG_LIBRARY ${OGG_INSTALL_DIR}/lib/libogg.a)
    set(OGG_INCLUDE_DIR ${OGG_INSTALL_DIR}/${CMAKE_INSTALL_INCLUDEDIR})

    ExternalProject_Add(libvorbis
        URL https://github.com/xiph/vorbis/releases/download/v1.3.7/libvorbis-1.3.7.tar.xz
        URL_HASH SHA256=b33cc4934322bcbf6efcbacf49e3ca01aadbea4114ec9589d1b1e9d20f72954b
        CMAKE_ARGS
            ${CMAKE_COMMON_DEP_ARGS}
            -DOGG_LIBRARY=${OGG_LIBRARY}
            -DOGG_INCLUDE_DIR=${OGG_INCLUDE_DIR}
            -DCMAKE_INSTALL_LIBDIR=lib
            -DCMAKE_POLICY_VERSION_MINIMUM=3.5 # https://github.com/xiph/vorbis/issues/113
        # INSTALL_BYPRODUCTS in CMake 3.26+
        BUILD_BYPRODUCTS
            <INSTALL_DIR>/lib/libvorbis.a
            <INSTALL_DIR>/lib/libvorbisfile.a
    )
    add_dependencies(libvorbis libogg)
    ExternalProject_Get_Property(libvorbis INSTALL_DIR)
    set(VORBIS_INSTALL_DIR ${INSTALL_DIR})

    add_library(ogg STATIC IMPORTED)
    set_target_properties(ogg PROPERTIES IMPORTED_LOCATION ${OGG_LIBRARY})

    add_library(vorbis STATIC IMPORTED)
    set_target_properties(vorbis PROPERTIES IMPORTED_LOCATION ${VORBIS_INSTALL_DIR}/lib/libvorbis.a)
    add_library(vorbisfile STATIC IMPORTED)
    set_target_properties(vorbisfile PROPERTIES IMPORTED_LOCATION ${VORBIS_INSTALL_DIR}/lib/libvorbisfile.a)

    set(VORBIS_INCLUDE_DIR ${VORBIS_INSTALL_DIR}/${CMAKE_INSTALL_INCLUDEDIR} ${OGG_INCLUDE_DIR})
    set(OGGVORBIS_LIBRARIES vorbis ogg vorbisfile)

    add_dependencies(vorbisfile libvorbis)
    add_dependencies(vorbis libvorbis)
    add_dependencies(ogg libogg)
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
    ${OPUS_ALL_INCLUDE_DIRS}
)

target_link_libraries(fmt.hdll
    libhl
    ${ZLIB_LIBRARIES}
    ${PNG_LIBRARIES}
    ${TurboJPEG_LIBRARIES}
    ${OGGVORBIS_LIBRARIES}
    ${OPUS_LIBRARIES}
)

install(
    TARGETS
        fmt.hdll
    DESTINATION ${HDLL_DESTINATION}
)
