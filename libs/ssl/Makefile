INCLUDES_BASE_DIR = ../../include
CFLAGS = -Wall -O3 -std=c11 -m64 -fPIC -pthread -fno-omit-frame-pointer
CFLAGS += -fvisibility=hidden -I. -I${INCLUDES_BASE_DIR}/mbedtls/include -I ../../src
CFLAGS += -D MBEDTLS_USER_CONFIG_FILE=\"mbedtls_user_config.h\"
LFLAGS = -L../.. -lhl

UNAME := $(shell uname)
ifeq ($(UNAME),Darwin)
CFLAGS += -framework Security -framework CoreFoundation
endif

SSL = \
	${INCLUDES_BASE_DIR}/mbedtls/library/aes.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/aesce.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/aesni.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/aria.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/asn1parse.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/asn1write.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/base64.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/bignum.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/bignum_core.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/bignum_mod.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/bignum_mod_raw.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/block_cipher.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/camellia.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/ccm.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/chacha20.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/chachapoly.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/cipher.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/cipher_wrap.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/cmac.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/constant_time.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/ctr_drbg.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/debug.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/des.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/dhm.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/ecdh.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/ecdsa.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/ecjpake.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/ecp.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/ecp_curves.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/ecp_curves_new.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/entropy.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/entropy_poll.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/error.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/gcm.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/hkdf.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/hmac_drbg.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/lmots.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/lms.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/md.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/md5.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/memory_buffer_alloc.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/mps_reader.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/mps_trace.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/net_sockets.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/nist_kw.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/oid.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/padlock.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/pem.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/pk.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/pk_ecc.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/pk_wrap.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/pkcs12.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/pkcs5.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/pkcs7.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/pkparse.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/pkwrite.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/platform.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/platform_util.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/poly1305.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/psa_crypto.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/psa_crypto_aead.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/psa_crypto_cipher.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/psa_crypto_client.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/psa_crypto_driver_wrappers_no_static.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/psa_crypto_ecp.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/psa_crypto_ffdh.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/psa_crypto_hash.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/psa_crypto_mac.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/psa_crypto_pake.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/psa_crypto_rsa.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/psa_crypto_se.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/psa_crypto_slot_management.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/psa_crypto_storage.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/psa_its_file.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/psa_util.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/ripemd160.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/rsa.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/rsa_alt_helpers.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/sha1.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/sha256.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/sha3.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/sha512.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/ssl_cache.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/ssl_ciphersuites.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/ssl_client.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/ssl_cookie.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/ssl_debug_helpers_generated.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/ssl_msg.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/ssl_ticket.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/ssl_tls.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/ssl_tls12_client.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/ssl_tls12_server.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/ssl_tls13_client.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/ssl_tls13_generic.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/ssl_tls13_keys.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/ssl_tls13_server.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/threading.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/timing.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/version.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/version_features.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/x509.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/x509_create.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/x509_crl.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/x509_crt.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/x509_csr.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/x509write.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/x509write_crt.o \
	${INCLUDES_BASE_DIR}/mbedtls/library/x509write_csr.o \
	ssl.o

all: ssl.hdll

ssl.hdll: ${SSL}
	${CC} ${CFLAGS} -shared -o $@ $^ ${LFLAGS}

%.o: %.c
	${CC} ${CFLAGS} -o $@ -c $<

clean:
	rm -f ${SSL} *.hdll
