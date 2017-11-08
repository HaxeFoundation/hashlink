This is TurboJPEG 1.4.2 sources

In order to be compilable with any MSVC version, we are not using the provided static lib.
Instead we compile the sources + a created lib per target from simd obj files using:

  lib /out:simd.lib *.obj

This lib should not contain any reference to a specific LIBC

The following patch was added to jsimd_i386.c (init_smd):

# ifdef _DEBUG
  simd_support &= ~JSIMD_SSE2; // using SSE2 in Debug mode will crash
# endif

Also added in jconfig.h

#define WITH_SIMD
