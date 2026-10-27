/*
 * Copyright (C)2015-2016 Haxe Foundation
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
#include <jit.h>
#include "hlsystem.h"

#ifdef HL_WIN
#	include <locale.h>
#	undef _GUID
#	include <windows.h>
typedef uchar pchar;
#define pprintf(str,file)	uprintf(USTR(str),file)
#define pfopen(file,ext) _wfopen(file,USTR(ext))
#define pcompare wcscmp
#define ptoi(s)	wcstol(s,NULL,10)
#define PSTR(x) USTR(x)
#else
#	include <sys/stat.h>
typedef char pchar;
#define pprintf printf
#define pfopen fopen
#define pcompare strcmp
#define ptoi atoi
#define PSTR(x) x
#endif

#if defined(HL_WIN_DESKTOP) && defined(HL_DX12_AGILITY_VERSION)
__declspec(dllexport) extern const UINT D3D12SDKVersion = HL_DX12_AGILITY_VERSION;
__declspec(dllexport) extern const char* D3D12SDKPath = u8"./D3D12/";
#endif

typedef struct {
	pchar *file;
	hl_code *code;
	hl_module *m;
	vdynamic *ret;
	int64 file_time;
	int64 file_size;
	bool has_build_id;
	unsigned char build_id[HL_BUILD_ID_SIZE];	// of the code loaded last (bytecode 7)
} main_context;

/*
	Last write time in the best resolution the file system offers, so that two versions
	written within the same second are told apart (the size is compared too).
*/
static int64 pfiletime( pchar *file, int64 *size ) {
#ifdef HL_WIN
	WIN32_FILE_ATTRIBUTE_DATA data;
	if( !GetFileAttributesExW(file,GetFileExInfoStandard,&data) ) {
		*size = -1;
		return -1;
	}
	*size = ((int64)data.nFileSizeHigh << 32) | data.nFileSizeLow;
	return ((int64)data.ftLastWriteTime.dwHighDateTime << 32) | data.ftLastWriteTime.dwLowDateTime;
#else
	struct stat st;
	if( stat(file,&st) != 0 ) {
		*size = -1;
		return -1;
	}
	*size = (int64)st.st_size;
#	if defined(__APPLE__)
	return (int64)st.st_mtimespec.tv_sec * 1000000000 + st.st_mtimespec.tv_nsec;
#	else
	return (int64)st.st_mtim.tv_sec * 1000000000 + st.st_mtim.tv_nsec;
#	endif
#endif
}

static hl_code *load_code( const pchar *file, char **error_msg, bool print_errors ) {
	hl_code *code;
	FILE *f = pfopen(file,"rb");
	int pos, size;
	char *fdata;
	if( f == NULL ) {
		if( print_errors ) pprintf("File not found '%s'\n",file);
		return NULL;
	}
	fseek(f, 0, SEEK_END);
	size = (int)ftell(f);
	fseek(f, 0, SEEK_SET);
	fdata = (char*)malloc(size);
	pos = 0;
	while( pos < size ) {
		int r = (int)fread(fdata + pos, 1, size-pos, f);
		if( r <= 0 ) {
			if( print_errors ) pprintf("Failed to read '%s'\n",file);
			return NULL;
		}
		pos += r;
	}
	fclose(f);
	code = hl_code_read((unsigned char*)fdata, size, error_msg);
	free(fdata);
	return code;
}

static main_context* main_ctx = NULL;

HL_API double hl_sys_time();

// a reload delayed by the running code is tried again at most every HL_HOT_RELOAD_RETRY seconds (0.5 by default)
static double retry_delay() {
	static double delay = -1;
	if( delay < 0 ) {
		const char *v = getenv("HL_HOT_RELOAD_RETRY");
		delay = v ? atof(v) : 0.5;
		if( delay < 0 ) delay = 0;
	}
	return delay;
}

static void set_build_id( main_context *m, hl_code *code ) {
	m->has_build_id = code->version >= 7;
	if( m->has_build_id ) memcpy(m->build_id, code->build_id, HL_BUILD_ID_SIZE);
}

/*
	From bytecode 7, the start of the file tells its length and its build id : a file still being
	written (shorter than its length) is checked again at the next call, and a file with the code
	already loaded (written again, or compiled to the same bytecode) is not read.
*/
static bool check_header( main_context *m, pchar *file, int64 size, bool *same ) {
	unsigned char head[HL_HEADER_SIZE], id[HL_BUILD_ID_SIZE];
	int file_size;
	*same = false;
	FILE *f = pfopen(file,"rb");
	if( f == NULL ) return false;
	int n = (int)fread(head, 1, HL_HEADER_SIZE, f);
	fclose(f);
	int version = hl_code_read_header(head, n, &file_size, id);
	if( version < 0 ) return false;
	if( version < 7 ) return true;
	if( file_size != size ) return false;
	*same = m->has_build_id && memcmp(id, m->build_id, HL_BUILD_ID_SIZE) == 0;
	return true;
}

static bool check_reload( vbyte *alt_file ) {
	static double last_try = 0;
	main_context* m = main_ctx;
	pchar* file = alt_file ? (pchar*)alt_file : m->file;
	int64 size;
	int64 time = pfiletime(file,&size);
	bool changed;
	if( time == m->file_time && size == m->file_size ) {
		if( !hl_setup.hot_retry || hl_sys_time() - last_try < retry_delay() )
			return false;
		// the function that delayed it still runs : no need to read the file and compare it again
		if( hl_hot_still_blocked() ) {
			last_try = hl_sys_time();
			return false;
		}
	} else {
		bool same;
		if( !check_header(m, file, size, &same) )
			return false;
		if( same ) {
			m->file_time = time;
			m->file_size = size;
			return false;
		}
	}
	last_try = hl_sys_time();
	hl_setup.hot_retry = false;
	char *error_msg = NULL;
	hl_code *code = load_code(file, &error_msg, false);
	if( code == NULL )
		return false;
	changed = hl_module_patch(m->m, code);
	m->file_time = time;
	m->file_size = size;
	set_build_id(m, code);
	hl_code_free(code);
	return changed;
}

static bool load_plugin( pchar *file ) {
	char *error_msg = NULL;
	hl_code *code = load_code(file, &error_msg, false);
	if( code == NULL )
		return false;
	int i;
	for(i=0;i<code->ntypes;i++) {
		hl_type *t1 = code->types + i;
		if( t1->kind != HOBJ && t1->kind != HSTRUCT ) continue;
		hl_type *t2 = hl_module_resolve_type(main_ctx->m, t1, false);
		// ensure that cast will work between types !
		if( t2 ) t1->obj->name = t2->obj->name;
	}
	hl_module *m = hl_module_alloc(code);
	if( m == NULL )
		return false;
	if( !hl_module_init(m,0) )
		return false;
	hl_code_free(code);
	vclosure cl;
	cl.t = m->code->functions[m->functions_indexes[m->code->entrypoint]].type;
	cl.fun = m->functions_ptrs[m->code->entrypoint];
	cl.hasValue = 0;
 	hl_dyn_call(&cl,NULL,0);
	return true;
}

static vdynamic *resolve_type( hl_type *t, hl_type *gt ) {
	hl_type *t2 = hl_module_resolve_type(main_ctx->m, t, true);
	if( t2 == NULL || (t2->kind != HOBJ && t2->kind != HSTRUCT))
		return NULL;
	hl_module_context *m = t->obj->m;
	hl_module_context *m2 = t2->obj->m;
	hl_type *gt2 = hl_module_resolve_type(main_ctx->m, gt, true);
	// patch bindings (constructor, etc.)
	int i;
	for(i=0;i<gt->obj->nbindings;i++) {
		int mid1 = gt->obj->bindings[(i<<1)|1];
		int mid2 = gt2->obj->bindings[(i<<1)|1];
		void *fun1 = m->functions_ptrs[mid1];
		void **fun2 = &m2->functions_ptrs[mid2];
		hl_jit_patch_method(fun1,fun2);
	}
	// patch methods
	for(i=0;i<t->obj->nproto;i++) {
		int mid1 = t->obj->proto[i].findex;
		int mid2 = t2->obj->proto[i].findex;
		void *fun1 = m->functions_ptrs[mid1];
		void **fun2 = &m2->functions_ptrs[mid2];
		hl_jit_patch_method(fun1,fun2);
	}
	return (vdynamic*)*t2->obj->global_value;
}

#ifdef HL_VCC
// this allows some runtime detection to switch to high performance mode
__declspec(dllexport) DWORD NvOptimusEnablement = 1;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
#endif

#if defined(HL_LINUX) || defined(HL_MAC)
#include <signal.h>
static void handle_signal( int signum ) {
	signal(signum, SIG_DFL);
	printf("SIGNAL %d[%s]\n",signum,strsignal(signum));
	if( hl_get_thread() != NULL ) {
		hl_dump_stack();
	}
	fflush(stdout);
	raise(signum);
}
static void setup_handler() {
	struct sigaction act;
	act.sa_sigaction = NULL;
	act.sa_handler = handle_signal;
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);
	signal(SIGPIPE, SIG_IGN);
	sigaction(SIGSEGV,&act,NULL);
	sigaction(SIGTERM,&act,NULL);
}
#else
static void setup_handler() {
}
#endif

#ifdef HL_WIN
int wmain(int argc, pchar *argv[]) {
#else
int main(int argc, pchar *argv[]) {
#endif
	static vclosure cl;
	pchar *file = NULL;
	char *error_msg = NULL;
	int debug_port = -1;
	bool debug_wait = false;
	bool debug_opt = false;
	bool hot_reload = false;
	bool dump = false;
	int profile_count = -1;
	main_context ctx;
	bool isExc = false;
	int first_boot_arg = -1;
	argv++;
	argc--;

	while( argc ) {
		pchar *arg = *argv++;
		argc--;
		if( pcompare(arg,PSTR("--debug")) == 0 ) {
			if( argc-- == 0 ) break;
			debug_port = ptoi(*argv++);
			continue;
		}
		if( pcompare(arg,PSTR("--debug-wait")) == 0 ) {
			debug_wait = true;
			continue;
		}
		if( pcompare(arg,PSTR("--debug-opt")) == 0 ) {
			debug_opt = true;
			continue;
		}
		if( pcompare(arg,PSTR("--version")) == 0 ) {
			printf("%d.%d.%d",HL_VERSION>>16,(HL_VERSION>>8)&0xFF,HL_VERSION&0xFF);
			return 0;
		}
#		ifdef HL_DEBUG
		if( pcompare(arg,PSTR("--dump")) == 0 ) {
			dump = true;
			continue;
		}
#		endif
		if( pcompare(arg,PSTR("--hot-reload")) == 0 ) {
			hot_reload = true;
			continue;
		}
		if( pcompare(arg,PSTR("--profile")) == 0 ) {
			if( argc-- == 0 ) break;
			profile_count = ptoi(*argv++);
			continue;
		}
		if( *arg == '-' || *arg == '+' ) {
			if( first_boot_arg < 0 ) first_boot_arg = argc + 1;
			// skip value
			if( argc && **argv != '+' && **argv != '-' ) {
				argc--;
				argv++;
			}
			continue;
		}
		file = arg;
		break;
	}
	if( file == NULL ) {
		FILE *fchk;
		file = PSTR("hlboot.dat");
		fchk = pfopen(file,"rb");
		if( fchk == NULL ) {
			printf("HL/JIT %d.%d.%d (c)2015-2026 Haxe Foundation\n  Usage : hl [--debug <port>] [--debug-wait] <file>\n",HL_VERSION>>16,(HL_VERSION>>8)&0xFF,HL_VERSION&0xFF);
#			ifdef HL_DEBUG
			printf("  Debug : hl --dump <file> to dump the jit code without running it\n");
#			endif
			return 1;
		}
		fclose(fchk);
		if( first_boot_arg >= 0 ) {
			argv -= first_boot_arg;
			argc = first_boot_arg;
		}
	}
	hl_global_init();
	hl_setup.file_path = file;
	hl_setup.sys_args = (pchar**)argv;
	hl_setup.sys_nargs = argc;
	hl_sys_init();
	hl_register_thread(&ctx);
	main_ctx = &ctx;
	ctx.file = file;
	ctx.code = load_code(file, &error_msg, true);
	if( ctx.code == NULL ) {
		if( error_msg ) printf("%s\n", error_msg);
		return 1;
	}
	ctx.m = hl_module_alloc(ctx.code);
	if( ctx.m == NULL )
		return 2;
	if( !hl_module_init(ctx.m,(hot_reload?HL_MODULE_HOT_RELOAD:0) | (dump?HL_MODULE_DUMP:0) | (debug_port > 0 && !debug_opt?HL_MODULE_DEBUG:0)) )
		return 3;
	if( hot_reload ) {
		ctx.file_time = pfiletime(ctx.file,&ctx.file_size);
		set_build_id(&ctx, ctx.code);
		hl_setup.reload_check = check_reload;
	}
	hl_setup.load_plugin = load_plugin;
	hl_setup.resolve_type = resolve_type;
	hl_code_free(ctx.code);
	if( dump ) {
		// the code has been dumped while jitting, don't run it
		hl_module_free(ctx.m);
		hl_free(&ctx.code->alloc);
		hl_global_free();
		return 0;
	}
	if( debug_port > 0 && !hl_module_debug(ctx.m,debug_port,debug_wait) ) {
		fprintf(stderr,"Could not start debugger on port %d\n",debug_port);
		return 4;
	}
	cl.t = ctx.code->functions[ctx.m->functions_indexes[ctx.m->code->entrypoint]].type;
	cl.fun = ctx.m->functions_ptrs[ctx.m->code->entrypoint];
	cl.hasValue = 0;
	setup_handler();
	hl_profile_setup(profile_count);
	ctx.ret = hl_dyn_call_safe(&cl,NULL,0,&isExc);
	hl_profile_end();
	if( isExc ) {
		hl_print_uncaught_exception(ctx.ret);
		hl_debug_break();
#ifdef HL_THREADS
		hl_gc_major();
#else
		hl_global_free();
#endif
		return 1;
	}
#ifdef HL_THREADS
	// other threads may still be running and crash if globals are freed, so only run a gc here
	hl_gc_major();
#else
	hl_module_free(ctx.m);
	hl_free(&ctx.code->alloc);
	hl_global_free();
#endif
	return 0;
}

