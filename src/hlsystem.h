#ifndef HLSYSTEM_H
#define HLSYSTEM_H

#include <hl.h>

/* System specific headers required internally */
#ifdef HL_WIN
#	undef _GUID
#	if defined(HL_WIN_DESKTOP) || defined(HL_XBS)
#		include <windows.h>
#	else
#		include <xdk.h>
#	endif
#endif

// hardware exceptions (access violation, stack overflow) are turned into haxe exceptions
#if defined(HL_VCC) && !defined(HL_XBO)
#	define CATCH_STACK_OVERFLOW
#endif

#endif
