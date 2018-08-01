/*
 * Copyright (C)2005-2016 Haxe Foundation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#ifndef OS_H
#define OS_H

#if defined(_WIN32)
#	define OS_WINDOWS
#endif

#if defined(__APPLE__) || defined(macintosh)
#	define OS_MAC
#endif

#if defined(linux) || defined(__linux__)
#	define OS_LINUX
#endif

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#	define OS_BSD
#endif

#if defined(__GNU__)
#	define OS_HURD
#endif

#if defined(__CYGWIN__)
#	define OS_CYGWIN
#endif

#if defined(OS_LINUX) || defined(OS_MAC) || defined(OS_BSD) || defined(OS_GNUKBSD) || defined (OS_HURD) || defined (OS_CYGWIN)
#	define OS_POSIX
#endif

#if defined(OS_MAC) || defined(OS_BSD)
#	include <machine/endian.h>
#elif !defined(OS_WINDOWS)
#	include <endian.h>
#endif

#ifndef TARGET_BIG_ENDIAN
#	define TARGET_LITTLE_ENDIAN
#endif

#ifndef true
#	define true 1
#	define false 0
	typedef int bool;
#endif

#endif
/* ************************************************************************ */
