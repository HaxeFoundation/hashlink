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

#endif
