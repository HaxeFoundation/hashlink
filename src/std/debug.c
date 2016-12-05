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
#include <hl.h>

#ifdef HL_WIN
static HANDLE last_process = NULL;
static int last_pid = -1;
static HANDLE OpenPID( int pid ) {
	if( pid == last_pid )
		return last_process;
	CloseHandle(last_process);
	last_pid = pid;
	last_process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	return last_process;
}
#endif

HL_API void *hl_debug_address( int low, int high ) {
#	ifdef HL_64
	return (void*)((int_val)low | (((int_val)high)<<32));
#	else
	if( high != 0 )
		return NULL;
	return (void*)low;
#	endif
}

HL_API bool hl_debug_start( int pid ) {
#	ifdef HL_WIN
	last_pid = -1;
	return (bool)DebugActiveProcess(pid);
#	else
	return false;
#	endif
}

HL_API bool hl_debug_stop( int pid ) {
#	ifdef HL_WIN
	BOOL b = DebugActiveProcessStop(pid);
	last_pid = -1;
	CloseHandle(last_process);
	last_process = NULL;
	return (bool)b;
#	else
	return false;
#	endif
}

HL_API bool hl_debug_breakpoint( int pid ) {
#	ifdef HL_WIN
	return (bool)DebugBreakProcess(OpenPID(pid));
#	else
	return false;
#	endif
}

HL_API bool hl_debug_read( int pid, vbyte *addr, vbyte *buffer, int size ) {
#	ifdef HL_WIN
	return (bool)ReadProcessMemory(OpenPID(pid),addr,buffer,size,NULL);
#	else
	return false;
#	endif
}

HL_API bool hl_debug_write( int pid, vbyte *addr, vbyte *buffer, int size ) {
#	ifdef HL_WIN
	return (bool)WriteProcessMemory(OpenPID(pid),addr,buffer,size,NULL);
#	else
	return false;
#	endif
}

HL_API bool hl_debug_flush( int pid, vbyte *addr, int size ) {
#	ifdef HL_WIN
	return (bool)FlushInstructionCache(OpenPID(pid),addr,size);
#	else
#	endif
}

HL_API int hl_debug_wait( int pid, int *thread, int timeout ) {
#	ifdef HL_WIN
	DEBUG_EVENT e;
	if( !WaitForDebugEvent(&e,timeout) )
		return -1;
	*thread = e.dwThreadId;
	switch( e.dwDebugEventCode ) {
	case EXCEPTION_DEBUG_EVENT:
		switch( e.u.Exception.ExceptionRecord.ExceptionCode ) {
		case EXCEPTION_BREAKPOINT:
			return 1;
		case EXCEPTION_SINGLE_STEP:
			return 2;
		default:
			return 3;
		}
	case EXIT_PROCESS_DEBUG_EVENT:
		return 0;
	default:
		ContinueDebugEvent(e.dwProcessId, e.dwThreadId, DBG_CONTINUE);
		break;
	}
	return -1;
#	else
	return 0;
#	endif
}

HL_API bool hl_debug_resume( int pid, int thread ) {
#	ifdef HL_WIN
	return (bool)ContinueDebugEvent(pid, thread, DBG_CONTINUE);
#	else
	return false;
#	endif
}

DEFINE_PRIM(_BOOL, debug_start, _I32);
DEFINE_PRIM(_VOID, debug_stop, _I32);
DEFINE_PRIM(_BOOL, debug_breakpoint, _I32);
DEFINE_PRIM(_BYTES, debug_address, _I32 _I32);
DEFINE_PRIM(_BOOL, debug_read, _I32 _BYTES _BYTES _I32);
DEFINE_PRIM(_BOOL, debug_write, _I32 _BYTES _BYTES _I32);
DEFINE_PRIM(_BOOL, debug_flush, _I32 _BYTES _I32);
DEFINE_PRIM(_I32, debug_wait, _I32 _REF(_I32) _I32);
DEFINE_PRIM(_BOOL, debug_resume, _I32 _I32);
