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

#ifndef HL_WIN
#	include <sys/types.h>
#	include <unistd.h>
#	include <errno.h>
#	if !defined(HL_MAC)
#		if defined(HL_BSD)
#			include <sys/wait.h>
#		else
#			include <wait.h>
#		endif
#	endif
#endif

#include <stdio.h>
#include <stdlib.h>

typedef struct _vprocess vprocess;

struct _vprocess {
	void (*finalize)( vprocess * );
#ifdef HL_WIN
	HANDLE oread;
	HANDLE eread;
	HANDLE iwrite;
	PROCESS_INFORMATION pinf;
#else
	int oread;
	int eread;
	int iwrite;
	int pid;
#endif
};

static void process_finalize( vprocess *p ) {
#	ifdef HL_WIN
	CloseHandle(p->eread);
	CloseHandle(p->oread);
	CloseHandle(p->iwrite);
	CloseHandle(p->pinf.hProcess);
	CloseHandle(p->pinf.hThread);
#	else
	close(p->eread);
	close(p->oread);
	close(p->iwrite);
#	endif
}

HL_PRIM vprocess *hl_process_run( vbyte *cmd, varray *vargs ) {
	vprocess *p;
#	ifdef HL_WIN
	SECURITY_ATTRIBUTES sattr;		
	STARTUPINFO sinf;
	HANDLE proc = GetCurrentProcess();
	HANDLE oread,eread,iwrite;
	if( vargs )
		return NULL; // should have been pre-processed by toplevel
	p = (vprocess*)hl_gc_alloc_finalizer(sizeof(vprocess));
	p->finalize = process_finalize;
	// startup process
	sattr.nLength = sizeof(sattr);
	sattr.bInheritHandle = TRUE;
	sattr.lpSecurityDescriptor = NULL;
	memset(&sinf,0,sizeof(sinf));
	sinf.cb = sizeof(sinf);
	sinf.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	sinf.wShowWindow = SW_HIDE;
	CreatePipe(&oread,&sinf.hStdOutput,&sattr,0);
	CreatePipe(&eread,&sinf.hStdError,&sattr,0);
	CreatePipe(&sinf.hStdInput,&iwrite,&sattr,0);
	DuplicateHandle(proc,oread,proc,&p->oread,0,FALSE,DUPLICATE_SAME_ACCESS);
	DuplicateHandle(proc,eread,proc,&p->eread,0,FALSE,DUPLICATE_SAME_ACCESS);
	DuplicateHandle(proc,iwrite,proc,&p->iwrite,0,FALSE,DUPLICATE_SAME_ACCESS);
	CloseHandle(oread);
	CloseHandle(eread);
	CloseHandle(iwrite);
	if( !CreateProcess(NULL,(uchar*)cmd,NULL,NULL,TRUE,0,NULL,NULL,&sinf,&p->pinf) ) {
		// handles will be finalized
		return NULL;
	}		
	// close unused pipes
	CloseHandle(sinf.hStdOutput);
	CloseHandle(sinf.hStdError);
	CloseHandle(sinf.hStdInput);
#	else
	char **argv;
	if (isRaw) {
		argv = (char**)malloc(sizeof(char*)*4);
		argv[0] = "/bin/sh";
		argv[1] = "-c";
		argv[2] = val_string(cmd);
		argv[3] = NULL;
	} else {
		argv = (char**)malloc(sizeof(char*)*(val_array_size(vargs)+2));
		argv[0] = val_string(cmd);
		for(i=0;i<val_array_size(vargs);i++) {
			value v = val_array_ptr(vargs)[i];
			val_check(v,string);
			argv[i+1] = val_string(v);
		}
		argv[i+1] = NULL;
	}
	int input[2], output[2], error[2];
	if( pipe(input) || pipe(output) || pipe(error) )
		neko_error();
	p = (vprocess*)hl_gc_alloc_finalizer(sizeof(vprocess));
	p->pid = fork();
	if( p->pid == -1 ) {
		close(input[0]);
		close(input[1]);
		close(output[0]);
		close(output[1]);
		close(error[0]);
		close(error[1]);
		return NULL;
	}
	// child
	if( p->pid == 0 ) {
		close(input[1]);
		close(output[0]);
		close(error[0]);
		dup2(input[0],0);
		dup2(output[1],1);
		dup2(error[1],2);
		execvp(argv[0],argv);
		fprintf(stderr,"Command not found : %s\n",cmd);
		exit(1);
	}
	// parent
	close(input[0]);
	close(output[1]);
	close(error[1]);
	p->iwrite = input[1];
	p->oread = output[0];
	p->eread = error[0];
#	endif
	p->finalize = process_finalize;
	return p;
}

HL_PRIM int hl_process_stdout_read( vprocess *p, vbyte *str, int pos, int len ) {
#	ifdef HL_WIN
	DWORD nbytes;
	if( !ReadFile(p->oread,str+pos,len,&nbytes,NULL) )
		return -1;
	return nbytes;
#	else
	int nbytes = read(p->oread,str+pos,len);
	if( nbytes <= 0 )
		return -1;
	return nbytes;
#	endif
}

HL_PRIM int hl_process_stderr_read( vprocess *p, vbyte *str, int pos, int len ) {
#	ifdef HL_WIN
	DWORD nbytes;
	if( !ReadFile(p->eread,str+pos,len,&nbytes,NULL) )
		return -1;
	return nbytes;
#	else
	int nbytes = read(p->eread,str+pos,len);
	if( nbytes <= 0 )
		return -1;
	return nbytes;
#	endif
}

HL_PRIM int hl_process_stdin_write( vprocess *p, vbyte *str, int pos, int len ) {
#	ifdef HL_WIN
	DWORD nbytes;
	if( !WriteFile(p->iwrite,str+pos,len,&nbytes,NULL) )
		return -1;
	return nbytes;
#	else
	int nbytes = write(p->iwrite,str+pos,len);
	if( nbytes < 0 )
		return -1;
	return nbytes;
#	endif
}

HL_PRIM bool hl_process_stdin_close( vprocess *p ) {
#	ifdef HL_WIN
	if( !CloseHandle(p->iwrite) )
		return false;
#	else
	if( close(p->iwrite) )
		return false;
	p->iwrite = -1;
#	endif
	return true;
}

HL_PRIM int hl_process_exit( vprocess *p ) {
#	ifdef HL_WIN
	DWORD rval;
	WaitForSingleObject(p->pinf.hProcess,INFINITE);
	if( !GetExitCodeProcess(p->pinf.hProcess,&rval) )
		return -1;
	return rval;
#	else
	int rval;
	while( waitpid(p->pid,&rval,0) != p->pid ) {
		if( errno == EINTR )
			continue;
		return -1;
	}
	if( !WIFEXITED(rval) ) {
		if( WIFSIGNALED(rval) )
			return 0x40000000 | WTERMSIG(rval);
		return -2;
	}
	return WEXITSTATUS(rval);
#	endif
}

HL_PRIM int hl_process_pid( vprocess *p ) {
#	ifdef HL_WIN
	return p->pinf.dwProcessId;
#	else
	return p->pid;
#	endif
}

HL_PRIM void hl_process_close( vprocess *p ) {
	if( !p->finalize ) return;
	p->finalize = NULL;
	process_finalize(p);
}

HL_PRIM void hl_process_kill( vprocess *p ) {
#	ifdef HL_WIN
	TerminateProcess(p->pinf.hProcess,0xCDCDCDCD);
#	else
	kill(p->pid,9);
#	endif
}

#define _PROCESS _ABSTRACT(hl_process)

DEFINE_PRIM( _PROCESS, process_run, _BYTES _ARR);
DEFINE_PRIM( _I32, process_stdout_read, _PROCESS _BYTES _I32 _I32);
DEFINE_PRIM( _I32, process_stderr_read, _PROCESS _BYTES _I32 _I32);
DEFINE_PRIM( _BOOL, process_stdin_close, _PROCESS);
DEFINE_PRIM( _I32, process_stdin_write, _PROCESS _BYTES _I32 _I32);
DEFINE_PRIM( _I32, process_exit, _PROCESS);
DEFINE_PRIM( _I32, process_pid, _PROCESS);
DEFINE_PRIM( _VOID, process_close, _PROCESS);
DEFINE_PRIM( _VOID, process_kill, _PROCESS);

/* ************************************************************************ */
