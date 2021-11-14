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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <semaphore.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/ptrace.h>
#include <sys/event.h>
#include <sys/sysctl.h>
#include <mach/mach.h>
#include <mach/mach_types.h>
#include <mach/mach_vm.h>
#include <mach/vm_prot.h>
#include <mach/exception_types.h>


#include <mdbg/mach_exc.h>

#include "mdbg.h"


#pragma mark Defines


#define     STATUS_TIMEOUT          -1
#define     STATUS_EXIT              0
#define     STATUS_BREAKPOINT        1
#define     STATUS_SINGLESTEP        2
#define     STATUS_ERROR             3
#define     STATUS_HANDLED           4
#define     STATUS_STACKOVERFLOW     5
#define     STATUS_WATCHBREAK        0x100

#define     SINGLESTEP_TRAP          0x00000100

#define     MAX_EXCEPTION_PORTS      16

/* Forward declarations */

boolean_t mach_exc_server(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP);
static struct debug_session *find_session(mach_port_t task);
static mach_port_t get_task(pid_t pid);
static mach_port_t get_thread(mach_port_t mach_task, uint thread_num);
static uint64_t get_thread_id(thread_t thread);
static x86_thread_state64_t* get_thread_state(mach_port_t mach_thread);
static kern_return_t set_thread_state(thread_t mach_thread, x86_thread_state64_t *break_state);
static x86_debug_state64_t* get_debug_state(thread_t mach_thread);
static kern_return_t set_debug_state(thread_t mach_thread, x86_debug_state64_t *break_state);

static void* task_exception_server (mach_port_t exception_port);


#pragma mark Structs

typedef struct {
    debug_session **list;
    unsigned int count;
} pointer_list;

pointer_list sessions;


typedef struct {
    mach_msg_type_number_t  count;
    exception_mask_t        masks[MAX_EXCEPTION_PORTS];
    exception_handler_t     ports[MAX_EXCEPTION_PORTS];
    exception_behavior_t    behaviors[MAX_EXCEPTION_PORTS];
    thread_state_flavor_t   flavors[MAX_EXCEPTION_PORTS];
} exception_ports_info;


#pragma mark Helpers

static void* safe_malloc(size_t x) {
    void *mal = malloc(x);
    if(mal == NULL) {
        fprintf(stderr, "[-safe_malloc] Error Exiting\n");
        exit(-1);
    }
    memset(mal, 0, x);
    return mal;
}

#pragma mark Debug

#if MDBG_DEBUG
#   define DEBUG_PRINT(fmt,...) \
        do { fprintf(stdout, "%s[%d] %s(): " fmt "\n",__FILE__, __LINE__, __func__, ##__VA_ARGS__); } while (0);
#   if MDBG_LOG_LEVEL > 0
#   define DEBUG_PRINT_VERBOSE(fmt,...) \
        do { fprintf(stdout, "%s[%d] %s(): " fmt "\n",__FILE__, __LINE__, __func__, ##__VA_ARGS__); } while (0);
#   else
#   define DEBUG_PRINT_VERBOSE(fmt,...)
#endif
#else
#   define DEBUG_PRINT(fmt,...)
#   define DEBUG_PRINT_VERBOSE(fmt,...)
#endif


#define RETURN_ON_MACH_ERROR(code,msg,...) \
    if (code != KERN_SUCCESS) {\
        DEBUG_PRINT(msg,##__VA_ARGS__);\
        return code;\
    }

#define EXIT_ON_MACH_ERROR(code,msg,...) \
    if (code != KERN_SUCCESS) {\
        DEBUG_PRINT(msg,##__VA_ARGS__);\
        exit(0);\
    }

#if MDBG_DEBUG
static void log_buffer(unsigned char *buf, int size) {
	for (int i=0; i<size; i++) {
		printf("%02lx\t", (unsigned long)buf[i]);
		if ((i+1)%16 == 0) printf("\n");
	}
}

static char* get_signal_name(int sig) {
    switch(sig) {
        case 1: return "SIGHUP";
        case 2: return "SIGINT";
        case 3: return "SIGQUIT";
        case 4: return "SIGILL";
        case 5: return "SIGTRAP";
        case 6: return "SIGABRT";
        case 7: return "SIGEMT";
        case 8: return "SIGFPE";
        case 9: return "SIGKILL";
        case 10: return "SIGBUS";
        case 11: return "SIGSEGV";
        case 12: return "SIGSYS";
        case 13: return "SIGPIPE";
        case 14: return "SIGALRM";
        case 15: return "SIGTERM";
        case 16: return "SIGURG";
        case 17: return "SIGSTOP";
        case 18: return "SIGTSTP";
        case 19: return "SIGCONT";
        case 20: return "SIGCHLD";
        case 21: return "SIGTTIN";
        case 22: return "SIGTTOU";
        case 23: return "SIGIO";
        case 24: return "SIGXCPU";
        case 25: return "SIGXFSZ";
        case 26: return "SIGVTALRM";
        case 27: return "SIGPROF";
        case 28: return "SIGWINCH";
        case 29: return "SIGINFO";
        case 30: return "SIGUSR1";
        case 31: return "SIGUSR2";
        default: return "UNKNOWN";
    }
}

static char* exception_to_string(exception_type_t exc) {
    switch(exc) {
        case EXC_BREAKPOINT     : return "EXC_BREAKPOINT";
        case EXC_BAD_ACCESS     : return "EXC_BAD_ACCESS";
        case EXC_BAD_INSTRUCTION: return "EXC_BAD_INSTRUCTION";
        case EXC_ARITHMETIC     : return "EXC_ARITHMETIC";
        case EXC_EMULATION      : return "EXC_EMULATION";
        case EXC_SOFTWARE       : return "EXC_SOFTWARE";
        case EXC_SYSCALL        : return "EXC_SYSCALL";
        case EXC_MACH_SYSCALL   : return "EXC_MACH_SYSCALL";
        case EXC_RPC_ALERT      : return "EXC_RPC_ALERT";
        case EXC_CRASH          : return "EXC_CRASH";
        case EXC_RESOURCE       : return "EXC_RESOURCE";
        case EXC_GUARD          : return "EXC_GUARD";
        case NOTE_EXEC          : return "EXEC";
        case NOTE_FORK          : return "FORK";
        case NOTE_SIGNAL        : return "SIGNAL";
        case NOTE_EXIT          : return "EXIT";

        default:
            return "[-exception_to_string] unknown exception type!";
    }
}

static char* get_register_name(int reg) {
    switch(reg) {
        case REG_RAX: return "Rax";
        case REG_RBX: return "Rbx";
        case REG_RCX: return "Rcx";
        case REG_RDX: return "Rdx";
        case REG_RDI: return "Rdi";
        case REG_RSI: return "Rsi";
        case REG_RBP: return "Rbp";
        case REG_RSP: return "Rsp";
        case REG_R8:  return "R8";
        case REG_R9:  return "R9";
        case REG_R10: return "R10";
        case REG_R11: return "R11";
        case REG_R12: return "R12";
        case REG_R13: return "R13";
        case REG_R14: return "R14";
        case REG_R15: return "R15";
        case REG_RIP: return "Rip";
        case REG_RFLAGS: return "Rflags";

        case REG_DR0: return "Dr0";
        case REG_DR1: return "Dr1";
        case REG_DR2: return "Dr2";
        case REG_DR3: return "Dr3";
        case REG_DR4: return "Dr4";
        case REG_DR5: return "Dr5";
        case REG_DR6: return "Dr6";
        case REG_DR7: return "Dr7";

        default: return "invalid register";
    }
}
#endif

#pragma mark Debug helpers

// From: https://developer.apple.com/library/archive/qa/qa1361/_index.html
// Returns true if the current process is being debugged (either 
// running under the debugger or has a debugger attached post facto).
bool is_debugger_attached(void) {
    int                 junk;
    int                 mib[4];
    struct kinfo_proc   info;
    size_t              size;

    info.kp_proc.p_flag = 0;

    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PID;
    mib[3] = getpid();

    size = sizeof(info);
    junk = sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);
    assert(junk == 0);

    // We're being debugged if the P_TRACED flag is set.
    return ( (info.kp_proc.p_flag & P_TRACED) != 0 );
}


#pragma mark Debug sessions


static debug_session *create_debug_session(mach_port_t task, pid_t pid) {
    debug_session *exists = find_session(task);
    if(exists != NULL) {
        return exists;
    }

    debug_session *sess = malloc(sizeof(debug_session));
    memset(sess, 0, sizeof(debug_session));

    sess->task = task;
    sess->pid = pid;
    sess->exception_port = 0;
    sess->old_exception_ports = (struct exception_ports_info*)malloc(sizeof(exception_ports_info));
    sess->process_status = STATUS_HANDLED;

    // create a semaphore `wait_sem` to handle signaling between exception port thread and main thread
    kern_return_t kret = semaphore_create(task, &sess->wait_sem, 0, 0);
    EXIT_ON_MACH_ERROR(kret, "Fatal error: failed to create semaphore!");

    // add `debug_session` to `sessions` list for future lookup
    sessions.list = realloc(sessions.list, sizeof(debug_session*) * (sessions.count + 1));
    sessions.list[sessions.count++] = sess;

    DEBUG_PRINT("created session num: %i", sessions.count);
    return sess;
}

static bool destroy_debug_session(debug_session *sess) {
    semaphore_destroy(sess->task, sess->wait_sem);

    free(sess);

    if(sessions.count > 1) {
        debug_session **new_list = malloc(sizeof(debug_session*) * (sessions.count-1));
        int j = 0;
        for(int i = 0; i < sessions.count; i++) {
            if(sessions.list[i] != sess) {
                new_list[j++] = sessions.list[i];
            }
        }
        free(sessions.list);
        sessions.list = new_list;
    } else {
        free(sessions.list);
        sessions.list = NULL;
    }
    sessions.count--;

    return true;
}

static debug_session *find_session(mach_port_t task) {
    int i = 0;
    while(i < sessions.count) {
        if(sessions.list[i]->task == task) {
            return sessions.list[i];
        }
        i++;
    }
    return NULL;
}

static debug_session *find_session_by_pid(pid_t pid) {
    int i = 0;
    while(i < sessions.count) {
        if(sessions.list[i]->pid == pid) {
            return sessions.list[i];
        }
        i++;
    }
    return NULL;
}


#pragma mark Registers


__uint64_t *get_reg( x86_thread_state64_t *regs, int r ) {
    switch( r ) {
        case REG_RAX: return &regs->__rax;
        case REG_RBX: return &regs->__rbx;
        case REG_RCX: return &regs->__rcx;
        case REG_RDX: return &regs->__rdx;
        case REG_RDI: return &regs->__rdi;
        case REG_RSI: return &regs->__rsi;
        case REG_RBP: return &regs->__rbp;
        case REG_RSP: return &regs->__rsp;
        case REG_R8:  return &regs->__r8;
        case REG_R9:  return &regs->__r9;
        case REG_R10: return &regs->__r10;
        case REG_R11: return &regs->__r11;
        case REG_R12: return &regs->__r12;
        case REG_R13: return &regs->__r13;
        case REG_R14: return &regs->__r14;
        case REG_R15: return &regs->__r15;
        case REG_RIP: return &regs->__rip;
        case REG_RFLAGS: return &regs->__rflags;
    }
    return NULL;
}

__uint64_t *get_debug_reg( x86_debug_state64_t *regs, int r ) {
    switch( r ) {
        case REG_DR0: return &regs->__dr0;
        case REG_DR1: return &regs->__dr1;
        case REG_DR2: return &regs->__dr2;
        case REG_DR3: return &regs->__dr3;
        case REG_DR4: return &regs->__dr4;
        case REG_DR5: return &regs->__dr5;
        case REG_DR6: return &regs->__dr6;
        case REG_DR7: return &regs->__dr7;
    }
    return NULL;
}

__uint64_t read_register(mach_port_t task, int thread, int reg, bool is64 ) {
    __uint64_t *rdata;
    mach_port_t mach_thread = get_thread(task, thread);

    if(reg >= REG_DR0) {
        x86_debug_state64_t *regs = get_debug_state(mach_thread);
        rdata = get_debug_reg(regs, reg - 4);
    } else {
        x86_thread_state64_t *regs = get_thread_state(mach_thread);
        rdata = get_reg(regs, reg);
    }

    DEBUG_PRINT_VERBOSE("register %s is: 0x%08x\n", get_register_name(reg), *rdata);

    return *rdata;
}

static kern_return_t write_register(mach_port_t task, int thread, int reg, void *value, bool is64 ) {
    DEBUG_PRINT_VERBOSE("write register %i (%s) on thread %i", reg, get_register_name(reg), thread);

    __uint64_t *rdata;
    mach_port_t mach_thread = get_thread(task, thread);

    if(reg >= REG_DR0) {
        x86_debug_state64_t *regs = get_debug_state(mach_thread);
        rdata = get_debug_reg(regs, reg - 4);
        DEBUG_PRINT_VERBOSE("register flag for %s was: 0x%08x\n",get_register_name(reg), *rdata);

        *rdata = (__uint64_t)value;
        set_debug_state(mach_thread, regs);
    } else {
        x86_thread_state64_t *regs = get_thread_state(mach_thread);
        rdata = get_reg(regs, reg);
        DEBUG_PRINT_VERBOSE("register flag for %s was: 0x%08x\n",get_register_name(reg), *rdata);

        *rdata = (__uint64_t)value;
        set_thread_state(mach_thread, regs);
    }

    DEBUG_PRINT_VERBOSE("register flag for %s now is: 0x%08x\n",get_register_name(reg), *rdata);

    return KERN_SUCCESS;
}


#pragma mark Memory IO


static kern_return_t read_memory(mach_port_t task, mach_vm_address_t addr, mach_vm_address_t dest, int size) {
    mach_vm_size_t nread;
	kern_return_t kret = mach_vm_read_overwrite(task, addr, size, dest, &nread);
	
    EXIT_ON_MACH_ERROR(kret,"Error: probably reading from invalid address!");

    DEBUG_PRINT_VERBOSE("read %i bytes from %p", nread, addr);
    #if MDBG_DEBUG && MDBG_LOG_LEVEL > 1
    log_buffer(dest, size);
    printf("\n\n");
    #endif

    return kret;
}

static kern_return_t write_memory(mach_port_t task, mach_vm_address_t addr, mach_vm_address_t src, int size) {
    kern_return_t kret = mach_vm_protect(task, addr, size, 0, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE);
    EXIT_ON_MACH_ERROR(kret,"Fatal error: failed to acquire write permission!");

    kret = mach_vm_write(task, addr, src, size);
    EXIT_ON_MACH_ERROR(kret,"Fatal error: failed to write to traced process memory!");

    kret = mach_vm_protect(task, addr, size, 0, VM_PROT_READ | VM_PROT_EXECUTE);
    EXIT_ON_MACH_ERROR(kret,"Fatal error: failed to reset write permission!");

    DEBUG_PRINT_VERBOSE("wrote %i bytes to %p",size, addr);
    #if MDBG_DEBUG && MDBG_LOG_LEVEL
        log_buffer(src, size);
        printf("\n\n");
    #endif
    return kret;
}


#pragma mark Threads

static thread_act_port_array_t get_task_threads(mach_port_t mach_task, mach_msg_type_number_t *threadCount) {
    thread_act_port_array_t threadList;
    task_threads(mach_task, &threadList, threadCount);

    return threadList;
}

static mach_port_t get_thread(mach_port_t mach_task, uint thread_id) {
    thread_act_port_array_t threadList;
    mach_msg_type_number_t threadCount;

    kern_return_t kret = task_threads(mach_task, &threadList, &threadCount);
    if (kret != KERN_SUCCESS) {
        DEBUG_PRINT("get_thread() failed with message %s!\n", mach_error_string(kret));
        exit(0);
    }
    for(int i=0;i<threadCount;i++) {
        if(get_thread_id(threadList[i]) == thread_id) {
            return threadList[i];
        }
    }
    exit(0); // TODO: catch better
}

static thread_identifier_info_data_t* get_thread_info(thread_t thread) {

  thread_identifier_info_data_t *tident = safe_malloc(sizeof(thread_identifier_info_data_t));
  mach_msg_type_number_t tident_count = THREAD_IDENTIFIER_INFO_COUNT;
  kern_return_t kret = thread_info (thread, THREAD_IDENTIFIER_INFO, (thread_info_t)tident, &tident_count);

  EXIT_ON_MACH_ERROR(kret, "failed to get thread info");

  return tident;
}

static uint64_t get_thread_id(thread_t thread) {
    thread_identifier_info_data_t *tinfo = get_thread_info(thread);
    return tinfo->thread_id;
}

static x86_thread_state64_t* get_thread_state(thread_t mach_thread) {

    x86_thread_state64_t* state;
    mach_msg_type_number_t stateCount = x86_THREAD_STATE64_COUNT;

    state = safe_malloc(sizeof(x86_thread_state64_t));
    kern_return_t kret = thread_get_state( mach_thread, x86_THREAD_STATE64, (thread_state_t)state, &stateCount);
    if (kret != KERN_SUCCESS) {
        DEBUG_PRINT("Error failed with message %s!\n", mach_error_string(kret));
        exit(0);
    }
    return state;
}

static kern_return_t set_thread_state(thread_t mach_thread, x86_thread_state64_t *break_state) {

    kern_return_t kret = thread_set_state(mach_thread, x86_THREAD_STATE64, (thread_state_t)break_state, x86_THREAD_STATE64_COUNT);
    if (kret != KERN_SUCCESS) {
        DEBUG_PRINT("Error failed with message %s!\n", mach_error_string(kret));
        exit(0);
    }
    return kret;
}


// Debug register state

static x86_debug_state64_t* get_debug_state(thread_t mach_thread) {

    x86_debug_state64_t* state;
    mach_msg_type_number_t stateCount = x86_DEBUG_STATE64_COUNT;

    state = safe_malloc(sizeof(x86_debug_state64_t));
    kern_return_t kret = thread_get_state( mach_thread, x86_DEBUG_STATE64, (thread_state_t)state, &stateCount);
    if (kret != KERN_SUCCESS) {
        DEBUG_PRINT("Error failed with message %s!\n", mach_error_string(kret));
        exit(0);
    }
    return state;
}

static kern_return_t set_debug_state(thread_t mach_thread, x86_debug_state64_t *break_state) {

    kern_return_t kret = thread_set_state(mach_thread, x86_DEBUG_STATE64, (thread_state_t)break_state, x86_DEBUG_STATE64_COUNT);
    if (kret != KERN_SUCCESS) {
        DEBUG_PRINT("Error failed with message %s!\n", mach_error_string(kret));
        exit(0);
    }
    return kret;
}


#pragma mark Exception ports

static kern_return_t save_exception_ports(task_t task, exception_ports_info *info) {
    info->count = (sizeof (info->ports) / sizeof (info->ports[0]));

    return task_get_exception_ports(task, EXC_MASK_ALL, info->masks, &info->count, info->ports, info->behaviors, info->flavors);
}

static kern_return_t restore_exception_ports(task_t task, exception_ports_info *info) {
    int i;
    kern_return_t kret;

    for (i = 0; i < info->count; i++) {
        kret = task_set_exception_ports(task, info->masks[i], info->ports[i], info->behaviors[i], info->flavors[i]);
        if (kret != KERN_SUCCESS)
            return kret;
    }
    return KERN_SUCCESS;
}

#pragma mark Tasks

static mach_port_t get_task(pid_t pid) {
    debug_session *sess = find_session_by_pid(pid);
    if(sess != NULL) {
        return sess->task;
    }

    mach_port_t task;
    kern_return_t kret = task_for_pid(mach_task_self(), pid, &task);

    EXIT_ON_MACH_ERROR(kret,"Fatal error: failed to get task for pid %i",pid);

    return task;
}

static kern_return_t attach_to_task(mach_port_t task, pid_t pid) {

    if(find_session(task) != NULL) {
        DEBUG_PRINT("Warning already attached to task (%i). Not attaching again!",task);
        return KERN_SUCCESS;
    }
    debug_session *sess = create_debug_session(task, pid);

    kern_return_t kret;
    int err;

    kret = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &sess->exception_port);
    RETURN_ON_MACH_ERROR(kret,"mach_port_allocate failed");

    kret = mach_port_insert_right(mach_task_self(), sess->exception_port, sess->exception_port, MACH_MSG_TYPE_MAKE_SEND);
    RETURN_ON_MACH_ERROR(kret,"mach_port_insert_right failed");

    // store current exception ports
    save_exception_ports(task, (exception_ports_info*)sess->old_exception_ports);

    kret = task_set_exception_ports(task, EXC_MASK_ALL, sess->exception_port, EXCEPTION_STATE_IDENTITY|MACH_EXCEPTION_CODES, x86_THREAD_STATE64);
    RETURN_ON_MACH_ERROR(kret,"task_set_exception_ports failed");

    // launch mach exception port thread //
    err = pthread_create(&sess->exception_handler_thread, NULL, (void *(*)(void*))task_exception_server, (void *(*)(void*))(unsigned long long)sess->exception_port);
    EXIT_ON_MACH_ERROR(err,"can't create *task_exception_server* thread :[%s]",strerror(err));

    DEBUG_PRINT("successfully created mach exception port thread %d\n", 0);

    return KERN_SUCCESS;
}

static kern_return_t attach_to_pid(pid_t pid) {
    attach_to_task(get_task(pid), pid);

    return ptrace(PT_ATTACHEXC, pid, 0, 0) == 0 ? KERN_SUCCESS : KERN_FAILURE;
}

static kern_return_t detach_from_pid(pid_t pid) {
    debug_session *sess = find_session_by_pid( pid );

    if(sess != NULL) {
        DEBUG_PRINT("cleaning up debug session...");
        restore_exception_ports(sess->task, (exception_ports_info*)sess->old_exception_ports);
        ptrace(PT_DETACH, pid, 0, 0);
        destroy_debug_session(sess);

        DEBUG_PRINT("successfully ended session!");
    } else {
        DEBUG_PRINT("found no session to cleanup!");
    }

    return KERN_SUCCESS;
}


#pragma mark Exceptions


extern kern_return_t catch_mach_exception_raise /* stub – will not be called */
(
	mach_port_t exception_port,
	mach_port_t thread,
	mach_port_t task,
	exception_type_t exception,
	mach_exception_data_t code,
	mach_msg_type_number_t codeCnt
) {
    DEBUG_PRINT("this handler should not be called");  
    return MACH_RCV_INVALID_TYPE;
}

extern kern_return_t catch_mach_exception_raise_state /* stub – will not be called */
(
	mach_port_t exception_port,
	exception_type_t exception,
	const mach_exception_data_t code,
	mach_msg_type_number_t codeCnt,
	int *flavor,
	const thread_state_t old_state,
	mach_msg_type_number_t old_stateCnt,
	thread_state_t new_state,
	mach_msg_type_number_t *new_stateCnt
) {
    DEBUG_PRINT("this handler should not be called");                           
    return MACH_RCV_INVALID_TYPE;
}

extern kern_return_t catch_mach_exception_raise_state_identity(
	mach_port_t             exception_port,
	mach_port_t             thread,
	mach_port_t             task,
	exception_type_t        exception,
	exception_data_t        code,
	mach_msg_type_number_t  codeCnt,
	int *                   flavor,
	thread_state_t          old_state,
	mach_msg_type_number_t  old_stateCnt,
	thread_state_t          new_state,
	mach_msg_type_number_t *new_stateCnt
) {

    x86_thread_state64_t *state = (x86_thread_state64_t *) old_state;
    x86_thread_state64_t *newState = (x86_thread_state64_t *) new_state;

    debug_session *sess = find_session(task);
    sess->current_thread = get_thread_id(thread); /* set system-wide thread id */

    DEBUG_PRINT("exception occured on thread (%i): %s",sess->current_thread, exception_to_string(exception));
    DEBUG_PRINT("stack address: 0x%02lx", state->__rip);


    if (exception == EXC_SOFTWARE && code[0] == EXC_SOFT_SIGNAL) { // handling UNIX soft signal
        int subcode = code[2];

        DEBUG_PRINT("EXC_SOFTWARE signal: %s",get_signal_name(code[2]));

        if (subcode == SIGSTOP || subcode == SIGTRAP) {
             // clear signal to prevent default OS handling //
            ptrace(PT_THUPDATE,
                sess->pid,
                (caddr_t)(uintptr_t)thread,
            0);

            task_suspend(sess->task);
            sess->process_status = STATUS_BREAKPOINT;

            semaphore_signal(sess->wait_sem);

            return KERN_SUCCESS;
        }
        /*else if(subcode == SIGTERM) { // TODO: Check if we need this
            sess->process_status = STATUS_EXIT;
            return KERN_SUCCESS;
        }*/
    }
    else if(exception == EXC_BREAKPOINT) {
        task_suspend(sess->task);

        // check if single step mode
        if(state->__rflags & SINGLESTEP_TRAP) {
            state->__rflags &= ~SINGLESTEP_TRAP; // clear single-step
            sess->process_status = STATUS_SINGLESTEP;
            DEBUG_PRINT("SINGLE STEP");
        } else {
            sess->process_status = STATUS_BREAKPOINT;
        }

        // move past breakpoint by setting old to new thread state
        *newState = *state;
        *new_stateCnt = old_stateCnt;
        *flavor = x86_THREAD_STATE64;

        semaphore_signal(sess->wait_sem);

        return KERN_SUCCESS;
    }
    else if(exception == EXC_BAD_INSTRUCTION) {
         task_suspend(sess->task);
         sess->process_status = STATUS_BREAKPOINT;

         return KERN_SUCCESS;
    }
    else if(exception == EXC_BAD_ACCESS) {
        task_suspend(sess->task);
        sess->process_status = STATUS_ERROR;

        return KERN_SUCCESS;
    }
    else {
        DEBUG_PRINT("not handling this exception (%s)!", exception_to_string(exception));
    }

    return KERN_FAILURE;
}

static void* task_exception_server (mach_port_t exception_port) {
    mach_msg_return_t rt;
    mach_msg_header_t *msg;
    mach_msg_header_t *reply;

    msg   = safe_malloc(sizeof(union __RequestUnion__mach_exc_subsystem));
    reply = safe_malloc(sizeof(union __ReplyUnion__mach_exc_subsystem));

    DEBUG_PRINT("launching exception server...");

    int i = 0;
    while (1) {
        DEBUG_PRINT("waiting for next exception (%i)...",i);
        i++;

        rt = mach_msg(msg, MACH_RCV_MSG, 0, sizeof(union __RequestUnion__mach_exc_subsystem), exception_port, 0, MACH_PORT_NULL);

        if (rt!= MACH_MSG_SUCCESS) {
            DEBUG_PRINT("MACH_RCV_MSG stopped, exit from task_exception_server thread :%d\n", 1);
            return "MACH_RCV_MSG_FAILURE";
        }
        /*
        * Call out to the mach_exc_server generated by mig and mach_exc.defs.
        * This will in turn invoke one of:
        * mach_catch_exception_raise()
        * mach_catch_exception_raise_state()
        * mach_catch_exception_raise_state_identity()
        * .. depending on the behavior specified when registering the Mach exception port.
        */
        mach_exc_server(msg, reply);

        // Send the now-initialized reply
        rt = mach_msg(reply, MACH_SEND_MSG, reply->msgh_size, 0, MACH_PORT_NULL, 0, MACH_PORT_NULL);

        if (rt!= MACH_MSG_SUCCESS) {
            return "MACH_SEND_MSG_FAILURE";
        }
    }
}

static void wait_for_exception(debug_session *sess, int timeout /*in millis*/) {
    DEBUG_PRINT("waiting for next exception...");

    kern_return_t kret = semaphore_timedwait(sess->wait_sem, (struct mach_timespec){0,timeout * 1000000});
    if(kret == KERN_OPERATION_TIMED_OUT) {
        sess->process_status = STATUS_TIMEOUT;
        DEBUG_PRINT("wait timed out!");
    } else {
        DEBUG_PRINT("got notified of an exception!");
    }
}

#pragma mark Debug API


status_t MDBG_API(session_attach)( pid_t pid ) {
    return attach_to_pid(pid) == KERN_SUCCESS;
}

status_t MDBG_API(session_detach)( pid_t pid ) {
    return detach_from_pid(pid) == KERN_SUCCESS;
}

status_t MDBG_API(session_pause)( pid_t pid ) {
    return kill(pid, SIGTRAP) == 0;
}

int MDBG_API(session_wait)( pid_t pid, int *thread, int timeout ) {
    debug_session *sess = find_session_by_pid( pid );
    if(sess != NULL) {
        wait_for_exception(sess, timeout);
        *thread = sess->current_thread;

        return sess->process_status;
    }
    return 4;
}

status_t MDBG_API(session_resume)( pid_t pid ) {
    debug_session *sess = find_session_by_pid( pid );
    if(sess != NULL) {
        sess->process_status = STATUS_HANDLED;
        task_resume(sess->task);

        return true;
    }
    return false;
}

debug_session *MDBG_API(session_get)( pid_t pid ) {
    return find_session_by_pid( pid );
}

status_t MDBG_API(read_memory)( pid_t pid, unsigned char* addr, unsigned char* dest, int size ) {
    return read_memory( get_task(pid), (mach_vm_address_t)addr, (mach_vm_address_t)dest, size ) == KERN_SUCCESS;
}

status_t MDBG_API(write_memory)( pid_t pid, unsigned char* addr, unsigned char* src, int size ) {
    return write_memory( get_task(pid), (mach_vm_address_t)addr, (mach_vm_address_t)src, size ) == KERN_SUCCESS;
}

void* MDBG_API(read_register)( pid_t pid, int thread, int reg, bool is64 ) {
    return (void*)read_register( get_task(pid), thread, reg, is64 );
}

status_t MDBG_API(write_register)( pid_t pid, int thread, int reg, void *value, bool is64 ) {
    return write_register( get_task(pid), thread, reg, value, is64 ) == KERN_SUCCESS;
}
