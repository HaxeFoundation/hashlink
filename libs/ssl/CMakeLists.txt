if (WIN32)
    set(MBEDTLS_INCLUDE_DIRS ${INCLUDES_BASE_DIR}/mbedtls/include .)
    set(MBEDTLS_LIBRARIES crypt32)

    add_library(ssl.hdll SHARED
        ${INCLUDES_BASE_DIR}/mbedtls/library/aes.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/aesce.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/aesni.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/aria.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/asn1parse.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/asn1write.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/base64.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/bignum.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/bignum_core.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/bignum_mod.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/bignum_mod_raw.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/block_cipher.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/camellia.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/ccm.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/chacha20.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/chachapoly.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/cipher.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/cipher_wrap.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/cmac.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/constant_time.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/ctr_drbg.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/debug.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/des.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/dhm.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/ecdh.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/ecdsa.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/ecjpake.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/ecp.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/ecp_curves.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/ecp_curves_new.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/entropy.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/entropy_poll.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/error.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/gcm.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/hkdf.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/hmac_drbg.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/lmots.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/lms.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/md.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/md5.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/memory_buffer_alloc.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/mps_reader.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/mps_trace.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/net_sockets.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/nist_kw.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/oid.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/padlock.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/pem.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/pk.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/pk_ecc.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/pk_wrap.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/pkcs12.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/pkcs5.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/pkcs7.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/pkparse.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/pkwrite.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/platform.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/platform_util.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/poly1305.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/psa_crypto.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/psa_crypto_aead.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/psa_crypto_cipher.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/psa_crypto_client.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/psa_crypto_driver_wrappers_no_static.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/psa_crypto_ecp.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/psa_crypto_ffdh.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/psa_crypto_hash.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/psa_crypto_mac.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/psa_crypto_pake.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/psa_crypto_rsa.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/psa_crypto_se.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/psa_crypto_slot_management.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/psa_crypto_storage.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/psa_its_file.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/psa_util.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/ripemd160.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/rsa.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/rsa_alt_helpers.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/sha1.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/sha256.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/sha3.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/sha512.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/ssl_cache.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/ssl_ciphersuites.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/ssl_client.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/ssl_cookie.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/ssl_debug_helpers_generated.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/ssl_msg.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/ssl_ticket.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/ssl_tls.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/ssl_tls12_client.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/ssl_tls12_server.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/ssl_tls13_client.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/ssl_tls13_generic.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/ssl_tls13_keys.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/ssl_tls13_server.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/threading.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/timing.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/version.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/version_features.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/x509.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/x509_create.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/x509_crl.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/x509_crt.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/x509_csr.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/x509write.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/x509write_crt.c
        ${INCLUDES_BASE_DIR}/mbedtls/library/x509write_csr.c
        ssl.c
    )

    target_compile_definitions(ssl.hdll
        PRIVATE
        MBEDTLS_USER_CONFIG_FILE="mbedtls_user_config.h"
    )
else()
    find_package(MbedTLS REQUIRED)

    add_library(ssl.hdll SHARED
        ssl.c
    )
endif()

set_as_hdll(ssl)

target_include_directories(ssl.hdll
    PRIVATE
    ${MBEDTLS_INCLUDE_DIRS}
)

target_link_libraries(ssl.hdll
    libhl
    ${MBEDTLS_LIBRARIES}
)

if(APPLE)
	find_library(SECURITY_LIBRARY Security REQUIRED)
	find_library(COREFOUNDATION_LIBRARY CoreFoundation REQUIRED)
	target_link_libraries(ssl.hdll ${COREFOUNDATION_LIBRARY} ${SECURITY_LIBRARY})
endif()

install(
    TARGETS
        ssl.hdll
    DESTINATION ${HDLL_DESTINATION}
)
