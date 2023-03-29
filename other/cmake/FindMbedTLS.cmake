############################################################################
# FindMdebTLS.txt
# Copyright (C) 2015  Belledonne Communications, Grenoble France
#
############################################################################
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
############################################################################
#
# - Find the mbedTLS include file and library
#
#  MBEDTLS_FOUND - system has mbedTLS
#  MBEDTLS_INCLUDE_DIRS - the mbedTLS include directory
#  MBEDTLS_LIBRARIES - The libraries needed to use mbedTLS

include(CMakePushCheckState)
include(CheckIncludeFile)
include(CheckCSourceCompiles)
include(CheckSymbolExists)


find_path(MBEDTLS_INCLUDE_DIRS
	NAMES mbedtls/ssl.h
	PATH_SUFFIXES include
)

# find the three mbedtls library
find_library(MBEDTLS_LIBRARY
	NAMES mbedtls
)

find_library(MBEDX509_LIBRARY
	NAMES mbedx509
)

find_library(MBEDCRYPTO_LIBRARY
	NAMES mbedcrypto
)

cmake_push_check_state(RESET)
set(CMAKE_REQUIRED_INCLUDES ${MBEDTLS_INCLUDE_DIRS} ${CMAKE_REQUIRED_INCLUDES_${BUILD_TYPE}})
list(APPEND CMAKE_REQUIRED_LIBRARIES ${MBEDTLS_LIBRARY} ${MBEDX509_LIBRARY} ${MBEDCRYPTO_LIBRARY})

# check we have a mbedTLS version 2 or above(all functions are prefixed mbedtls_)
if(MBEDTLS_LIBRARY AND MBEDX509_LIBRARY AND MBEDCRYPTO_LIBRARY)
	check_symbol_exists(mbedtls_ssl_init "mbedtls/ssl.h" MBEDTLS_V2)
	if(NOT MBEDTLS_V2)
		message ("MESSAGE: NO MBEDTLS_V2")
		message ("MESSAGE: MBEDTLS_LIBRARY=" ${MBEDTLS_LIBRARY})
		message ("MESSAGE: MBEDX509_LIBRARY=" ${MBEDX509_LIBRARY})
		message ("MESSAGE: MBEDCRYPTO_LIBRARY=" ${MBEDCRYPTO_LIBRARY})
		set (MBEDTLS_VERSION_1 On)
	else()
		# Are we mbdetls 2 or 3?
		# from version 3 and on, version number is given in include/mbedtls/build_info.h.
		# This file does not exists before version 3
		if (EXISTS "${MBEDTLS_INCLUDE_DIRS}/mbedtls/build_info.h")
			set (MBEDTLS_VERSION_3 On)
		else()
			set (MBEDTLS_VERSION_2 On)
		endif()
	endif()

endif()

check_symbol_exists(mbedtls_ssl_conf_dtls_srtp_protection_profiles "mbedtls/ssl.h" DTLS_SRTP_AVAILABLE)

# Define the imported target for the three mbedtls libraries
foreach(targetname "mbedtls" "mbedx509" "mbedcrypto")
	string(TOUPPER ${targetname} varprefix)
	add_library(${targetname} SHARED IMPORTED)
	if (WIN32)
		set_target_properties(${targetname} PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${${varprefix}_INCLUDE_DIRS}"
			IMPORTED_IMPLIB "${${varprefix}_LIBRARY}"
		)
	else()
		set_target_properties(${targetname} PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${${varprefix}_INCLUDE_DIRS}"
			IMPORTED_LOCATION "${${varprefix}_LIBRARY}"
		)
	endif()
endforeach()
unset(varprefix)

# MBEDTLS_LIBRARIES only needs to contain the name of the targets
set (MBEDTLS_LIBRARIES
	mbedtls
	mbedx509
	mbedcrypto
)


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MbedTLS
	DEFAULT_MSG
	MBEDTLS_INCLUDE_DIRS MBEDTLS_LIBRARIES
)

cmake_pop_check_state()



mark_as_advanced(MBEDTLS_INCLUDE_DIRS MBEDTLS_LIBRARIES DTLS_SRTP_AVAILABLE)
