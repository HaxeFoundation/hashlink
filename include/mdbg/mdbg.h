/*
 * Copyright (C)2005-2020 Haxe Foundation
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

#include <stdbool.h>
#include <pthread.h>
#include <mach/mach_types.h>

#ifndef MDBG_DEBUG
#define MDBG_DEBUG 0
#endif

#ifndef MDBG_LOG_LEVEL
#define MDBG_LOG_LEVEL 0
#endif

#define MDBG_API(func)    mdbg_##func

typedef bool        status_t;

struct exception_ports_info;

typedef struct debug_session {
    mach_port_t task;
    pid_t pid;
    uint64_t current_thread;

    mach_port_t exception_port;
    struct exception_ports_info* old_exception_ports;
    pthread_t exception_handler_thread;
    mach_port_t wait_sem;

    int process_status;
} debug_session;


/* x64 Registers */

#define     REG_RAX     1
#define     REG_RBX     2
#define     REG_RCX     3
#define     REG_RDX     4
#define     REG_RDI     5
#define     REG_RSI     6
#define     REG_RBP     7
#define     REG_RSP     8
#define     REG_R8      9
#define     REG_R9      10
#define     REG_R10     11
#define     REG_R11     12
#define     REG_R12     13
#define     REG_R13     14
#define     REG_R14     15
#define     REG_R15     16
#define     REG_RIP     17
#define     REG_RFLAGS  18


/* x64 Debug Registers */

#define     REG_DR0     100
#define     REG_DR1     101
#define     REG_DR2     102
#define     REG_DR3     103
#define     REG_DR4     104
#define     REG_DR5     105
#define     REG_DR6     106
#define     REG_DR7     107


/** Public API **/

extern status_t        MDBG_API(session_attach)( pid_t pid );

extern status_t        MDBG_API(session_detach)( pid_t pid );

extern int             MDBG_API(session_wait)(pid_t pid, int *thread, int timeout );

extern status_t        MDBG_API(session_pause)( pid_t pid) ;

extern status_t        MDBG_API(session_resume)( pid_t pid );

extern debug_session*  MDBG_API(session_get)( pid_t pid);

extern status_t        MDBG_API(read_memory)( pid_t pid, unsigned char* addr, unsigned char* dest, int size );

extern status_t        MDBG_API(write_memory)( pid_t pid, unsigned char* addr, unsigned char* src, int size );

extern void*           MDBG_API(read_register)( pid_t pid, int thread, int reg, bool is64 );

extern status_t        MDBG_API(write_register)( pid_t pid, int thread, int reg, void *value, bool is64 );
