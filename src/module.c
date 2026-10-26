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
#include <hlmodule.h>
#include <jit.h>

#ifdef HL_WIN
#	undef _GUID
#	include <windows.h>
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#	define dlopen(l,p)		(void*)( (l) ? LoadLibraryA(l) : (HMODULE)&__ImageBase)
#	define dlsym(h,n)		GetProcAddress((HANDLE)h,n)
#else
#	include <dlfcn.h>
#endif

#ifdef __GNUC__
#	define STACK_WALK_C_FRAMES
#endif

HL_API void hl_prim_not_loaded( const uchar *err );


static hl_module **cur_modules = NULL;
static int modules_count = 0;

static bool module_resolve_pos( hl_module *m, void *addr, int *fidx, int *fpos ) {
	int code_pos = ((int)(int_val)((unsigned char*)addr - (unsigned char*)m->jit_code));
	int min, max;
	hl_debug_infos *dbg;
	hl_function *fdebug;
	if( m->jit_debug == NULL )
		return false;
	// lookup function from code pos
	min = 0;
	max = m->code->nfunctions;
	while( min < max ) {
		int mid = (min + max) >> 1;
		hl_debug_infos *p = m->jit_debug + mid;
		if( p->start <= code_pos )
			min = mid + 1;
		else
			max = mid;
	}
	if( min == 0 )
		return false; // hl_callback
	do {
		min--;
		*fidx = min;
		dbg = m->jit_debug + min;
		fdebug = m->code->functions + min;
	} while( !dbg->offsets );
	// lookup inside function
	min = 0;
	max = fdebug->nops;
	code_pos -= dbg->start;
	while( min < max ) {
		int mid = (min + max) >> 1;
		int offset = dbg->large ? ((int*)dbg->offsets)[mid] : ((unsigned short*)dbg->offsets)[mid];
		if( offset < code_pos )
			min = mid + 1;
		else
			max = mid;
	}
	if( min == 0 )
		return false; // ???
	*fpos = min - 1;
	return true;
}

hl_module **hl_get_modules( int *count ) {
	*count = modules_count;
	return cur_modules;
}

uchar *hl_module_resolve_symbol_full( void *addr, uchar *out, int *outSize, int **r_debug_addr ) {
	int *debug_addr;
	int file, line;
	int pos = 0;
	int fidx, fpos;
	hl_function *fdebug;
	int i;
	hl_module *m = NULL;
	for(i=0;i<modules_count;i++) {
		m = cur_modules[i];
		if( addr >= m->jit_code && addr <= (void*)((char*)m->jit_code + m->codesize) ) break;
	}
	if( i == modules_count )
		return NULL;
	if( !module_resolve_pos(m,addr,&fidx,&fpos) )
		return NULL;
	// extract debug info
	fdebug = m->code->functions + fidx;
	debug_addr = fdebug->debug + ((fpos&0xFFFF) * 2);
	file = debug_addr[0];
	line = debug_addr[1];
	if( r_debug_addr ) {
		*r_debug_addr = debug_addr;
		if( file < 0 ) return NULL; // already cached
	}
	if( !out )
		return NULL;
	int size = *outSize;
	if( fdebug->obj )
		pos += usprintf(out,size - pos,USTR("%s.%s("),fdebug->obj->name,fdebug->field.name);
	else if( fdebug->field.ref )
		pos += usprintf(out,size - pos,USTR("%s.~%s.%d("),fdebug->field.ref->obj->name, fdebug->field.ref->field.name, fdebug->ref);
	else
		pos += usprintf(out,size - pos,USTR("fun$%d("),fdebug->findex);
	pos += hl_from_utf8(out + pos,size - pos,m->code->debugfiles[file&0x7FFFFFFF]);
	pos += usprintf(out + pos, size - pos, USTR(":%d)"), line);
	*outSize = pos;
	return out;
}

static uchar *module_resolve_symbol( void *addr, uchar *out, int *outSize ) {
	return hl_module_resolve_symbol_full(addr,out,outSize,NULL);
}

static bool module_is_jit_code( void *addr, bool functions_only ) {
	for(int i=0;i<modules_count;i++) {
		hl_module *m = cur_modules[i];
		unsigned char *code = m->jit_code;
		int code_size = m->codesize;
		if( functions_only && m->jit_debug ) {
			int s = m->jit_debug[0].start;
			code += s;
			code_size -= s;
		}
		if( addr >= (void*)code && addr < (void*)(code + code_size) )
			return true;
	}
	return false;
}

static void **walk_at( void *p, void *stack_base, void *copy ) {
	return copy ? (void**)((char*)copy + ((char*)p - (char*)stack_base)) : (void**)p;
}

static bool is_call_site( void *ret ) {
#if defined(_M_X64) || defined(__x86_64__)
	unsigned char *p = (unsigned char*)ret;
	int k;
	if( !module_is_jit_code(ret,true) || !module_is_jit_code((char*)ret - 8,true) ) return false;
	if( p[-5] == 0xE8 ) return true;
	for(k=2;k<=7;k++) {
		int reg = (p[-k+1] >> 3) & 7;
		if( p[-k] == 0xFF && (reg == 2 || reg == 3) ) return true;
	}
	return false;
#elif defined(_M_ARM64) || defined(__aarch64__)
	unsigned int ins;
	if( ((int_val)ret) & 3 ) return false;
	if( !module_is_jit_code(ret,true) || !module_is_jit_code((char*)ret - 4,true) ) return false;
	ins = ((unsigned int*)ret)[-1];
	// BL imm26 / BLR Xn
	return (ins & 0xFC000000) == 0x94000000 || (ins & 0xFFFFFC1F) == 0xD63F0000;
#else
	return false;
#endif
}

static int frame_chain( void **fp, void *stack_base, void *stack_top, void *copy, int max, bool *clean ) {
	int n = 0;
	*clean = false;
	while( n < max ) {
		void **rec;
		void *ret, *next;
		if( (void*)fp < stack_base || (void*)(fp + 2) > stack_top ) break;
		if( ((int_val)fp) & (sizeof(void*) - 1) ) break;
		rec = walk_at(fp,stack_base,copy);
		next = rec[0];
		ret = rec[1];
		if( next <= (void*)fp || next >= stack_top ) break;
		if( !is_call_site(ret) ) {
			*clean = !module_is_jit_code(ret,true);
			break;
		}
		n++;
		fp = (void**)next;
	}
	return n;
}

int hl_module_capture_stack_walk( void *pc, void *fp, void *stack_base, void *stack_top, void *copy, void **out, int size ) {
	void **scan = (void**)stack_base;
	int count = 0;
	bool have = module_is_jit_code(pc,true);
	if( !out ) size = 0x7FFFFFFF;
	while( count < size ) {
		void **rec;
		void *ret, *next;
		if( !have ) {
			void **a = scan;
			bool clean = false;
			while( (void*)(a + 2) <= stack_top ) {
				int n = frame_chain(a,stack_base,stack_top,copy,3,&clean);
				if( n >= 2 || (n == 1 && clean) ) break;
				a++;
			}
			if( (void*)(a + 2) > stack_top ) break;
			{
				void **b = a;
				while( b > scan ) {
					void *v = walk_at(b - 1,stack_base,copy)[0];
					b--;
					if( is_call_site(v) ) {
						if( out ) out[count] = v;
						count++;
						break;
					}
				}
				if( count == size ) break;
			}
			rec = walk_at(a,stack_base,copy);
			pc = rec[1];
			fp = rec[0];
			scan = a + 1;
			have = true;
		}
		if( out ) out[count] = pc;
		count++;
		if( fp < stack_base || (void*)((char*)fp + 2 * sizeof(void*)) > stack_top ) break;
		rec = walk_at(fp,stack_base,copy);
		next = rec[0];
		ret = rec[1];
		if( next <= fp || next >= stack_top ) break;
		if( module_is_jit_code(ret,true) ) {
			pc = ret;
			fp = next;
		} else {
			scan = (void**)((char*)fp + 2 * sizeof(void*));
			have = false;
		}
	}
	return count;
}

#ifdef WIN64_UNWIND_TABLES

// x64 registers numbering used by the unwind codes
#define UREG_RSP	4
#define UREG_RBP	5

static bool unwind_read( void *addr, void *stack_base, void *stack_top, void *copy, void **out ) {
	if( addr < stack_base || (char*)addr + sizeof(void*) > (char*)stack_top || (((int_val)addr) & (sizeof(void*) - 1)) ) return false;
	*out = *walk_at(addr,stack_base,copy);
	return true;
}

/*
	Unwinds one native frame with the x64 unwind tables, reading the stack (or its copy) ourselves :
	RtlVirtualUnwind would read the live stack of a thread that is either suspended (deadlock risk)
	or already resumed. regs[] holds the nonvolatile registers, rsp being *sp.
*/
static bool unwind_native_frame( void **pc, char **sp, void **regs, bool leaf, void *stack_base, void *stack_top, void *copy ) {
	DWORD64 base;
	PRUNTIME_FUNCTION f = RtlLookupFunctionEntry((DWORD64)*pc, &base, NULL);
	char *rsp = *sp;
	if( !f ) {
		// leaf function without unwind data : the return address is at the top of the stack
		if( !leaf || !unwind_read(rsp,stack_base,stack_top,copy,pc) ) return false;
		*sp = rsp + sizeof(void*);
		return true;
	}
	if( leaf ) {
		// stopped inside an epilog : the prolog is partly undone, emulate the remaining instructions
		unsigned char *p = (unsigned char*)*pc;
		char *esp = rsp;
		void *eregs[16];
		bool ok = true;
		memcpy(eregs,regs,sizeof(eregs));
		if( p[0] == 0x48 && p[1] == 0x83 && p[2] == 0xC4 ) {
			esp += (signed char)p[3];
			p += 4;
		} else if( p[0] == 0x48 && p[1] == 0x81 && p[2] == 0xC4 ) {
			esp += *(int*)(p + 3);
			p += 7;
		} else if( (p[0] & 0xFE) == 0x48 && p[1] == 0x8D && ((p[2] >> 3) & 7) == UREG_RSP && (p[2] >> 6) != 0 && (p[2] >> 6) != 3 && (p[2] & 7) != UREG_RSP ) {
			// lea rsp, [reg + disp]
			int reg = (p[2] & 7) | ((p[0] & 1) << 3);
			int mod = p[2] >> 6;
			esp = (char*)eregs[reg] + (mod == 1 ? (signed char)p[3] : *(int*)(p + 3));
			p += mod == 1 ? 4 : 7;
		}
		while( ok ) {
			int reg;
			if( p[0] >= 0x58 && p[0] <= 0x5F ) {
				reg = p[0] - 0x58;
				p++;
			} else if( p[0] == 0x41 && p[1] >= 0x58 && p[1] <= 0x5F ) {
				reg = p[1] - 0x58 + 8;
				p += 2;
			} else
				break;
			ok = unwind_read(esp,stack_base,stack_top,copy,&eregs[reg]);
			esp += sizeof(void*);
		}
		// a jump only ends an epilog (tail call) after the stack was restored, otherwise it's just a jump
		bool restored = p != (unsigned char*)*pc;
		if( ok && (p[0] == 0xC3 || p[0] == 0xC2 || (p[0] == 0xF3 && p[1] == 0xC3) || (restored && (p[0] == 0xE9 || p[0] == 0xEB || (p[0] == 0x48 && p[1] == 0xFF && p[2] == 0x25)))) ) {
			if( !unwind_read(esp,stack_base,stack_top,copy,pc) ) return false;
			memcpy(regs,eregs,sizeof(eregs));
			*sp = esp + sizeof(void*);
			return true;
		}
	}
	unsigned char *info = (unsigned char*)(base + f->UnwindData);
	unsigned int prolog_offset = (unsigned int)((DWORD64)*pc - (base + f->BeginAddress));
	char *frame = rsp;
	while( true ) {
		int flags = info[0] >> 3;
		int ncodes = info[2];
		int freg = info[3] & 15;
		int foffset = info[3] >> 4;
		unsigned short *codes = (unsigned short*)(info + 4);
		// codes are only partly executed while still inside the prolog
		if( prolog_offset >= info[1] ) prolog_offset = 0xFFFFFFFF;
		if( freg ) {
			bool fp_set = prolog_offset == 0xFFFFFFFF;
			for(int i=0;i<ncodes && !fp_set;i++)
				if( ((codes[i] >> 8) & 15) == 3 && (unsigned int)(codes[i] & 0xFF) <= prolog_offset ) fp_set = true;
			if( fp_set ) frame = (char*)regs[freg] - foffset * 16;
		}
		for(int i=0;i<ncodes;) {
			int offset = codes[i] & 0xFF;
			int op = (codes[i] >> 8) & 15;
			int opinfo = codes[i] >> 12;
			int size;
			switch( op ) {
			case 1: size = opinfo ? 3 : 2; break;
			case 4: case 6: case 8: size = 2; break;
			case 5: case 7: case 9: size = 3; break;
			default: size = 1; break;
			}
			if( i + size > ncodes ) return false;
			if( prolog_offset != 0xFFFFFFFF && offset > (int)prolog_offset ) {
				i += size;
				continue;
			}
			switch( op ) {
			case 0: // PUSH_NONVOL
				if( !unwind_read(rsp,stack_base,stack_top,copy,&regs[opinfo]) ) return false;
				rsp += sizeof(void*);
				break;
			case 1: // ALLOC_LARGE
				rsp += opinfo ? (codes[i+1] | ((unsigned int)codes[i+2] << 16)) : codes[i+1] * 8;
				break;
			case 2: // ALLOC_SMALL
				rsp += opinfo * 8 + 8;
				break;
			case 3: // SET_FPREG
				rsp = frame;
				break;
			case 4: // SAVE_NONVOL
				if( !unwind_read(frame + codes[i+1] * 8,stack_base,stack_top,copy,&regs[opinfo]) ) return false;
				break;
			case 5: // SAVE_NONVOL_FAR
				if( !unwind_read(frame + (codes[i+1] | ((unsigned int)codes[i+2] << 16)),stack_base,stack_top,copy,&regs[opinfo]) ) return false;
				break;
			case 10: // PUSH_MACHFRAME
				if( opinfo ) rsp += sizeof(void*);
				if( !unwind_read(rsp,stack_base,stack_top,copy,pc) ) return false;
				if( !unwind_read(rsp + 3 * sizeof(void*),stack_base,stack_top,copy,(void**)sp) ) return false;
				return true;
			default: // xmm saves and version 2 epilog descriptions
				break;
			}
			i += size;
		}
		if( !(flags & UNW_FLAG_CHAININFO) ) break;
		PRUNTIME_FUNCTION chained = (PRUNTIME_FUNCTION)&codes[(ncodes + 1) & ~1];
		info = (unsigned char*)(base + chained->UnwindData);
		prolog_offset = 0xFFFFFFFF;
	}
	if( !unwind_read(rsp,stack_base,stack_top,copy,pc) ) return false;
	*sp = rsp + sizeof(void*);
	return true;
}

/*
	Captures the stack of a thread from a full register context : native frames are unwound with
	the x64 unwind tables, JIT frames by following their frame pointers. Unlike scanning the stack,
	this can't be fooled by stale frames left in the stack by previous calls.
*/
int hl_module_capture_stack_unwind( void *pc, void *sp, void **regs, bool leaf, void *stack_base, void *stack_top, void *copy, void **out, int size ) {
	int count = 0;
	char *rsp = (char*)sp;
	if( !out ) size = 0x7FFFFFFF;
	while( count < size && pc ) {
		if( module_is_jit_code(pc,false) ) {
			void **fp = (void**)regs[UREG_RBP];
			if( (char*)fp < rsp ) fp = NULL; // not a frame of this stack part
			if( module_is_jit_code(pc,true) ) {
				if( out ) out[count] = pc;
				count++;
			}
			// stopped before the frame is set (push rbp / mov rbp,rsp) or after it is undone (ret) :
			// rbp is still the caller one, so get the caller return address from the stack top
			if( leaf ) {
				unsigned char *p = (unsigned char*)pc;
				void *ret = NULL;
				if( p[0] == 0x55 || p[0] == 0xC3 )
					unwind_read(rsp,stack_base,stack_top,copy,&ret);
				else if( p[-1] == 0x55 && p[0] == 0x48 && ((p[1] == 0x89 && p[2] == 0xE5) || (p[1] == 0x8B && p[2] == 0xEC)) )
					unwind_read(rsp + sizeof(void*),stack_base,stack_top,copy,&ret);
				if( ret && module_is_jit_code(ret,true) && count < size ) {
					if( out ) out[count] = ret;
					count++;
				}
			}
			leaf = false;
			pc = NULL;
			while( count < size ) {
				void *next, *ret;
				if( !unwind_read(fp,stack_base,stack_top,copy,&next) || !unwind_read(fp + 1,stack_base,stack_top,copy,&ret) ) break;
				if( module_is_jit_code(ret,false) ) {
					if( (void**)next <= fp || next >= stack_top ) break;
					if( module_is_jit_code(ret,true) ) {
						if( out ) out[count] = ret;
						count++;
					}
					fp = (void**)next;
					continue;
				}
				// returning into native code : unwind it from the JIT frame end.
				// The saved rbp is any value the native code had in it, not necessarily a frame
				pc = ret;
				rsp = (char*)(fp + 2);
				regs[UREG_RBP] = next;
				break;
			}
			continue;
		}
		char *prev = rsp;
		if( !unwind_native_frame(&pc,&rsp,regs,leaf,stack_base,stack_top,copy) ) break;
		if( rsp <= prev ) break;
		leaf = false;
	}
	return count;
}

#endif

int hl_module_capture_stack( void **stack, int size ) {
#ifdef WIN64_UNWIND_TABLES
	CONTEXT context;
	RtlCaptureContext(&context);
	int count = 0;
	while(true) {
		DWORD64 base;
		DWORD64 ip = context.Rip;
		PRUNTIME_FUNCTION fn_entry = RtlLookupFunctionEntry(ip, &base, NULL);
		if (!fn_entry) {
			break;
		}
		void *module_addr = (void*)ip;
		for(int i=0;i<modules_count;i++) {
			hl_module *m = cur_modules[i];
			unsigned char *code = m->jit_code;
			int code_size = m->codesize;
			if( module_addr >= (void*)code && module_addr < (void*)(code + code_size) ) {
				if( stack && count == size )
					break;
				if( stack )
					stack[count++] = module_addr;
				else
					count++;
				break;
			}
		}

		if (stack && count == size) {
			break;
		}

		void* handler_data;
		ULONG64 establisher_frame;
		RtlVirtualUnwind(0, base, ip, fn_entry, &context, &handler_data, &establisher_frame, NULL);
		if (context.Rip == 0) {
			break;
		}
	}
	return count;
#else
	void *stack_top = hl_get_thread()->stack_top;
	void *pc = NULL, *fp = NULL;
#	ifdef STACK_WALK_C_FRAMES
	void **cur = (void**)__builtin_frame_address(0);
	bool from_stub = false;
	int depth = 0;
	while( depth++ < 64 ) {
		void **next;
		void *ret;
		if( (void*)cur < (void*)&stack || (void*)(cur + 2) > stack_top || (((int_val)cur) & (sizeof(void*) - 1)) ) break;
		next = (void**)cur[0];
		ret = cur[1];
		if( next <= cur || (void*)next >= stack_top ) break;
		if( module_is_jit_code(ret,true) ) {
			// null access stubs are reached with push+jmp
			if( from_stub || is_call_site(ret) ) {
				pc = ret;
				fp = next;
			}
			break;
		}
		from_stub = module_is_jit_code(ret,false);
		cur = next;
	}
#	endif
	return hl_module_capture_stack_walk(pc, fp, (void**)&stack, stack_top, NULL, stack, size);
#endif
}

static bool module_capture_break_context( void **rip, void **regs ) {
#ifdef WIN64_UNWIND_TABLES
	CONTEXT c;
	RtlCaptureContext(&c);
	while( !module_is_jit_code((void*)c.Rip,false) ) {
		DWORD64 base;
		PRUNTIME_FUNCTION fn_entry = RtlLookupFunctionEntry(c.Rip, &base, NULL);
		if( !fn_entry ) return false;
		void *handler_data;
		ULONG64 establisher_frame;
		RtlVirtualUnwind(0, base, c.Rip, fn_entry, &c, &handler_data, &establisher_frame, NULL);
		if( c.Rip == 0 ) return false;
	}
	*rip = (void*)c.Rip;
	regs[0] = (void*)c.Rax; regs[1] = (void*)c.Rcx; regs[2] = (void*)c.Rdx; regs[3] = (void*)c.Rbx;
	regs[4] = (void*)c.Rsp; regs[5] = (void*)c.Rbp; regs[6] = (void*)c.Rsi; regs[7] = (void*)c.Rdi;
	regs[8] = (void*)c.R8; regs[9] = (void*)c.R9; regs[10] = (void*)c.R10; regs[11] = (void*)c.R11;
	regs[12] = (void*)c.R12; regs[13] = (void*)c.R13; regs[14] = (void*)c.R14; regs[15] = (void*)c.R15;
	for(int i=0;i<16;i++)
		regs[16+i] = (void*)((M128A*)&c.Xmm0)[i].Low;
	return true;
#else
	return false;
#endif
}

static void hl_module_types_dump( void (*fdump)( void *, int) ) {
	int ntypes = 0;
	int i, j, fcount = 0;
	for(i=0;i<modules_count;i++)
		ntypes += cur_modules[i]->code->ntypes;
	fdump(&ntypes,4);
	for(i=0;i<modules_count;i++) {
		hl_module *m = cur_modules[i];
		for(j=0;j<m->code->ntypes;j++) {
			hl_type *t = m->code->types + j;
			fdump(&t,sizeof(void*));
			if( t->kind == HFUN ) fcount++;
		}
	}
	fdump(&fcount,4);
	for(i=0;i<modules_count;i++) {
		hl_module *m = cur_modules[i];
		for(j=0;j<m->code->ntypes;j++) {
			hl_type *t = m->code->types + j;
			if( t->kind == HFUN ) {
				hl_type *ct = (hl_type*)&t->fun->closure_type;
				fdump(&ct,sizeof(void*));
			}
		}
	}
}

hl_module *hl_module_alloc( hl_code *c ) {
	int i;
	int gsize = 0;
	hl_module *m = (hl_module*)malloc(sizeof(hl_module));
	if( m == NULL )
		return NULL;
	memset(m,0,sizeof(hl_module));
	m->code = c;
	m->globals_indexes = (int*)malloc(sizeof(int)*c->nglobals);
	if( m->globals_indexes == NULL ) {
		hl_module_free(m);
		return NULL;
	}
	for(i=0;i<c->nglobals;i++) {
		gsize += hl_pad_size(gsize, c->globals[i]);
		m->globals_indexes[i] = gsize;
		gsize += hl_type_size(c->globals[i]);
	}
	m->globals_size = gsize;
	m->globals_data = (unsigned char*)malloc(gsize);
	if( m->globals_data == NULL ) {
		hl_module_free(m);
		return NULL;
	}
	memset(m->globals_data,0,gsize);
	m->functions_ptrs = (void**)malloc(sizeof(void*)*(c->nfunctions + c->nnatives));
	m->functions_indexes = (int*)malloc(sizeof(int)*(c->nfunctions + c->nnatives));
	m->ctx.functions_types = (hl_type**)malloc(sizeof(void*)*(c->nfunctions + c->nnatives));
	if( m->functions_ptrs == NULL || m->functions_indexes == NULL || m->ctx.functions_types == NULL ) {
		hl_module_free(m);
		return NULL;
	}
	memset(m->functions_ptrs,0,sizeof(void*)*(c->nfunctions + c->nnatives));
	memset(m->functions_indexes,0xFF,sizeof(int)*(c->nfunctions + c->nnatives));
	memset(m->ctx.functions_types,0,sizeof(void*)*(c->nfunctions + c->nnatives));
	hl_alloc_init(&m->ctx.alloc);
	m->ctx.functions_ptrs = m->functions_ptrs;
	return m;
}

void hl_module_null_function() {
	hl_error("Null function ptr");
}

static void append_fields( char **p, hl_type *t );

static void append_type( char **p, hl_type *t ) {
	*(*p)++ = TYPE_STR[t->kind];
	switch( t->kind ) {
	case HFUN:
		{
			int i;
			for(i=0;i<t->fun->nargs;i++)
				append_type(p,t->fun->args[i]);
			*(*p)++ = '_';
			append_type(p,t->fun->ret);
			break;
		}
	case HREF:
	case HNULL:
		append_type(p,t->tparam);
		break;
	case HOBJ:
		{
			append_fields(p, t);
			*(*p)++ = '_';
		}
		break;
	case HABSTRACT:
		*p += utostr(*p,100,t->abs_name);
		*(*p)++ = '_';
		break;
	default:
		break;
	}
}

static void append_fields( char **p, hl_type *t ) {
	int i;
	if( t->obj->super )
		append_fields(p, t->obj->super);
	for(i=0;i<t->obj->nfields;i++)
		append_type(p,t->obj->fields[i].t);
}

#define DISABLED_LIB_PTR ((void*)(int_val)2)

static void *resolve_library( const char *lib, bool is_opt ) {
	char tmp[256];
	void *h;

#	ifndef HL_CONSOLE
	static char *DISABLED_LIBS = NULL;
	if( !DISABLED_LIBS ) {
		DISABLED_LIBS = getenv("HL_DISABLED_LIBS");
		if( !DISABLED_LIBS ) DISABLED_LIBS = "";
	}
	char *disPart = strstr(DISABLED_LIBS, lib);
	if( disPart ) {
		disPart += strlen(lib);
		if( *disPart == 0 || *disPart == ',' )
			return DISABLED_LIB_PTR;
	}
#	endif

	if( strcmp(lib,"builtin") == 0 )
		return dlopen(NULL,RTLD_LAZY);

	if( strcmp(lib,"std") == 0 ) {
#	ifdef HL_WIN
#		ifdef HL_64
		h = dlopen("libhl64.dll",RTLD_LAZY);
		if( h == NULL ) h = dlopen("libhl.dll",RTLD_LAZY);
#		else
		h = dlopen("libhl.dll",RTLD_LAZY);
#		endif
		if( h == NULL && !is_opt ) hl_fatal1("Failed to load library %s","libhl.dll");
		return h;
#	else
		return RTLD_DEFAULT;
#	endif
	}

	strcpy(tmp,lib);

#	ifdef HL_64
	strcpy(tmp+strlen(lib),"64.hdll");
	h = dlopen(tmp,RTLD_LAZY);
	if( h != NULL ) return h;
#	endif

	strcpy(tmp+strlen(lib),".hdll");
	h = dlopen(tmp,RTLD_LAZY);
	if( h == NULL && !is_opt )
		hl_fatal1("Failed to load library %s",tmp);
	return h;
}

static void disabled_primitive() {
	hl_error("This library primitive has been disabled");
}

void hl_module_init_indexes( hl_module *m ) {
	int i;
	for(i=0;i<m->code->nfunctions;i++) {
		hl_function *f = m->code->functions + i;
		m->functions_indexes[f->findex] = i;
		m->ctx.functions_types[f->findex] = f->type;
	}
	for(i=0;i<m->code->nnatives;i++) {
		hl_native *n = m->code->natives + i;
		m->functions_indexes[n->findex] = i + m->code->nfunctions;
		m->ctx.functions_types[n->findex] = n->t;
	}
	for(i=0;i<m->code->ntypes;i++) {
		hl_type *t = m->code->types + i;
		switch( t->kind ) {
		case HOBJ:
		case HSTRUCT:
			t->obj->m = &m->ctx;
			t->obj->global_value = ((int)(int_val)t->obj->global_value) ? (void**)(int_val)(m->globals_data + m->globals_indexes[(int)(int_val)t->obj->global_value-1]) : NULL;
			{
				int j;
				for(j=0;j<t->obj->nproto;j++) {
					hl_obj_proto *p = t->obj->proto + j;
					hl_function *f = m->code->functions + m->functions_indexes[p->findex];
					f->obj = t->obj;
					f->field.name = p->name;
				}
				for(j=0;j<t->obj->nbindings;j++) {
					int fid = t->obj->bindings[j<<1];
					int mid = t->obj->bindings[(j<<1)|1];
					hl_obj_field *of = hl_obj_field_fetch(t,fid);
					switch( of->t->kind ) {
					case HFUN:
					case HDYN:
						{
							hl_function *f = m->code->functions + m->functions_indexes[mid];
							f->obj = t->obj;
							f->field.name = of->name;
						}
						break;
					default:
						break;
					}
				}
			}
			break;
		case HENUM:
			hl_init_enum(t,&m->ctx);
			t->tenum->global_value = ((int)(int_val)t->tenum->global_value) ? (void**)(int_val)(m->globals_data + m->globals_indexes[(int)(int_val)t->tenum->global_value-1]) : NULL;
			break;
		case HVIRTUAL:
			hl_init_virtual(t,&m->ctx);
			break;
		default:
			break;
		}
	}
	for(i=0;i<m->code->nfunctions;i++) {
		int k;
		hl_function *f = m->code->functions + i;
		hl_function *real_f = f;
		while( real_f && !real_f->obj ) real_f = real_f->field.ref;
		if( real_f == NULL ) continue;
		for(k=0;k<f->nops;k++) {
			hl_opcode *op = f->ops + k;
			switch( op->op ) {
			case OCall0:
			case OCall1:
			case OCall2:
			case OCall3:
			case OCall4:
			case OCallN:
			case OStaticClosure:
			case OInstanceClosure:
				if( m->functions_indexes[op->p2] < m->code->nfunctions ) {
					hl_function *floc = m->code->functions + m->functions_indexes[op->p2];
					if( floc->obj ) continue;
					floc->field.ref = real_f;
					floc->ref = real_f->ref++;
				}
				break;
			default:
				break;
			}
		}
	}

	static hl_type_obj obj_entry = {0};
	hl_function *fent = m->code->functions + m->functions_indexes[m->code->entrypoint];
	obj_entry.name = USTR("");
	fent->obj = &obj_entry;
	fent->field.name = USTR("init");
}

#ifdef HL_VTUNE
#include <jitprofiling.h>
#define VTUNE_OFFSET(dbg,j)	((int)(dbg->large ? ((int*)dbg->offsets)[j] : ((unsigned short*)dbg->offsets)[j]))
h_bool hl_module_init_vtune( hl_module *m ) {
	int i;
	if( !iJIT_IsProfilingActive() || m->jit_debug == NULL )
		return false;
	for(i=0;i<m->code->nfunctions;i++) {
		hl_function *f = m->code->functions + i;
		void *faddr = m->functions_ptrs[f->findex];
		hl_debug_infos *dbg = m->jit_debug + i;

		iJIT_Method_Load jm = {0};
		char out[256];
		if( dbg->offsets == NULL ) continue;
		jm.method_id = iJIT_GetNewMethodID();
		if( f->obj ) {
			jm.class_file_name = hl_to_utf8(f->obj->name);
			jm.method_name = hl_to_utf8(f->field.name);
		} else if( f->field.ref ) {
			jm.class_file_name = hl_to_utf8(f->field.ref->obj->name);
			jm.method_name = hl_to_utf8(f->field.ref->field.name);
		} else {
			sprintf(out,"fun$%d", f->findex);
			jm.method_name = out;
		}
		jm.method_load_address = faddr;
		jm.method_size = VTUNE_OFFSET(dbg,f->nops);

		int j;
		int file = f->debug[0] & 0x7FFFFFFF;
		int curline = -1;
		LineNumberInfo *lines = (LineNumberInfo*)malloc(sizeof(LineNumberInfo)*f->nops);
		int nlines = 0;
		jm.source_file_name = m->code->debugfiles[file];
		for(j=0;j<f->nops;j++) {
			int file2 = f->debug[j<<1] & 0x7FFFFFFF;
			int line = f->debug[(j<<1)|1];
			if( file2 != file || line == curline ) continue;
			if( nlines ) lines[nlines-1].Offset = VTUNE_OFFSET(dbg,j);
			lines[nlines].LineNumber = line;
			curline = line;
			nlines++;
		}
		if( nlines ) lines[nlines-1].Offset = jm.method_size;
		if( nlines && jm.method_size ) {
			jm.line_number_table = lines;
			jm.line_number_size = nlines;
		}
		iJIT_NotifyEvent(iJVM_EVENT_TYPE_METHOD_LOAD_FINISHED,(void*)&jm);
		free(lines);
	}
	return true;
}
static void modules_init_vtune() {
	int i;
	for(i=0;i<modules_count;i++)
		hl_module_init_vtune(cur_modules[i]);
}
#endif

void hl_module_init_natives( hl_module *m ) {
	char tmp[256];
	int i;
	void *libHandler = NULL;
	const char *curlib = NULL, *sign;
	for(i=0;i<m->code->nnatives;i++) {
		hl_native *n = m->code->natives + i;
		const char *lib = n->lib;
		bool is_opt = *lib == '?';
		char *p = tmp;
		void *f;
		if( is_opt ) lib++;
		if( curlib != lib ) {
			curlib = lib;
			libHandler = resolve_library(lib, is_opt);
		}
		if( libHandler == DISABLED_LIB_PTR ) {
			m->functions_ptrs[n->findex] = disabled_primitive;
			continue;
		}
		strcpy(p,"hlp_");
		p += 4;
		strcpy(p,n->name);
		p += strlen(n->name);
		*p++ = 0;
		f = dlsym(libHandler,tmp);
		if( f == NULL ) {
			if( is_opt ) {
				m->functions_ptrs[n->findex] = hl_prim_not_loaded;
				continue;
			}
			hl_fatal2("Failed to load function %s@%s",n->lib,n->name);
		}
		m->functions_ptrs[n->findex] = ((void *(*)( const char **p ))f)(&sign);
		p = tmp;
		append_type(&p,n->t);
		*p++ = 0;
		if( sign ) {
			int slen = (int)strlen(sign);
			if( slen && sign[slen-1] == *HL_CALLB ) {
				hl_jit_tag_callback(m->functions_ptrs[n->findex]);
				slen--;
			}
			if( slen != (int)strlen(tmp) || memcmp(sign,tmp,slen) != 0 )
				hl_fatal4("Invalid signature for function %s@%s : %s required but %s found in hdll",n->lib,n->name,tmp,sign);
		}
	}
}

void hl_module_init_constant( hl_module *m, hl_constant *c ) {
	hl_type *t = m->code->globals[c->global];
	hl_runtime_obj *rt;
	vdynamic **global = (vdynamic**)(m->globals_data + m->globals_indexes[c->global]);
	vdynamic *v = NULL;
	switch (t->kind) {
	case HOBJ:
	case HSTRUCT:
		rt = hl_get_obj_rt(t);
		v = (vdynamic*)hl_malloc(&m->ctx.alloc,rt->size);
		v->t = t;
		for(int i=0;i<c->nfields;i++) {
			int idx = c->fields[i];
			hl_type *ft = t->obj->fields[i].t;
			void *addr = (char*)v + rt->fields_indexes[i];
			switch (ft->kind) {
			case HI32:
				*(int*)addr = m->code->ints[idx];
				break;
			case HBOOL:
				*(bool*)addr = idx != 0;
				break;
			case HF64:
				*(double*)addr = m->code->floats[idx];
				break;
			case HBYTES:
				*(const void**)addr = hl_get_ustring(m->code, idx);
				break;
			case HTYPE:
				*(hl_type**)addr = m->code->types + idx;
				break;
			default:
				*(void**)addr = *(void**)(m->globals_data + m->globals_indexes[idx]);
				break;
			}
		}
		break;
	default:
		hl_fatal("assert");
	}
	*global = v;
	hl_remove_root(global);
}

void hl_module_add( hl_module *m ) {
	hl_module **old_modules = cur_modules;
	hl_module **new_modules = (hl_module**)malloc(sizeof(void*)*(modules_count + 1));
	memcpy(new_modules, old_modules, sizeof(void*)*modules_count);
	new_modules[modules_count] = m;
	cur_modules = new_modules;
	modules_count++;
	free(old_modules);
}

int hl_module_init( hl_module *m, int flags ) {
	int i;
	jit_ctx *ctx;
	bool hot_reload = (flags & HL_MODULE_HOT_RELOAD) != 0;
	if( hot_reload ) hl_hot_reload_prepare(m);
	// RESET globals
	for(i=0;i<m->code->nglobals;i++) {
		hl_type *t = m->code->globals[i];
		if( t->kind == HFUN ) *(void**)(m->globals_data + m->globals_indexes[i]) = hl_module_null_function;
		if( hl_is_ptr(t) )
			hl_add_root(m->globals_data+m->globals_indexes[i]);
	}
	// inits
	if( hot_reload ) m->hash = hl_code_hash_alloc(m->code);
	hl_module_init_natives(m);
	hl_module_init_indexes(m);
#	ifdef WIN64_UNWIND_TABLES
	m->unwind_table_size = m->code->nfunctions + 10; // extra space for jit internals
	m->unwind_table = malloc(sizeof(RUNTIME_FUNCTION) * m->unwind_table_size);
	memset(m->unwind_table, 0, sizeof(RUNTIME_FUNCTION) * m->unwind_table_size);
#	endif
	// JIT
	ctx = hl_jit_alloc();
	if( ctx == NULL )
		return 0;
	bool dump = (flags & HL_MODULE_DUMP) != 0;
	m->debug = (flags & HL_MODULE_DEBUG) != 0;
	hl_jit_init(ctx, m);
	for(i=0;i<m->code->nfunctions;i++) {
		hl_function *f = m->code->functions + i;
		int fpos = hl_jit_function(ctx, m, f);
		if( fpos < 0 ) {
			hl_jit_free(ctx, false);
			return 0;
		}
		m->functions_ptrs[f->findex] = (void*)(int_val)fpos;
		if( dump ) hl_emit_dump(ctx);
	}
	m->jit_code = hl_jit_code(ctx, m, &m->codesize, &m->jit_debug, NULL);
	for(i=0;i<m->code->nfunctions;i++) {
		hl_function *f = m->code->functions + i;
		m->functions_ptrs[f->findex] = ((unsigned char*)m->jit_code) + ((int_val)m->functions_ptrs[f->findex]);
	}
	// INIT constants
	for(i=0;i<m->code->nconstants;i++) {
		hl_constant *c = m->code->constants + i;
		hl_module_init_constant(m, c);
	}
	hl_module_add(m);
	hl_setup.resolve_symbol = module_resolve_symbol;
	hl_setup.capture_stack = hl_module_capture_stack;
	hl_setup.capture_break_context = module_capture_break_context;
	hl_gc_set_dump_types(hl_module_types_dump);
#	ifdef HL_VTUNE
	hl_setup.vtune_init = modules_init_vtune;
#	endif
#	ifdef WIN64_UNWIND_TABLES
	RtlAddFunctionTable(m->unwind_table, m->unwind_table_size, (DWORD64)m->jit_code);
#	endif
	hl_jit_free(ctx, false);
	if( hot_reload ) hl_hot_reload_init(m);
	return 1;
}

bool hl_module_same_type( hl_type *t1, hl_type *t2 ) {
	if( hl_safe_cast(t1,t2) )
		return true;
	if( t1->kind != t2->kind )
		return false;
	int i;
	switch( t1->kind ) {
	case HOBJ:
	case HSTRUCT:
		return ucmp(t1->obj->name,t2->obj->name) == 0;
	case HFUN:
	case HMETHOD:
		if( t1->fun->nargs != t2->fun->nargs )
			return false;
		for(i=0;i<t1->fun->nargs;i++)
			if( !hl_module_same_type(t1->fun->args[i],t2->fun->args[i]) )
				return false;
		return hl_module_same_type(t1->fun->ret, t2->fun->ret);
	case HVIRTUAL:
		if( t1->virt == t2->virt )
			return true;
		if( t1->virt->nfields != t2->virt->nfields )
			return false;
		hl_type_virtual *vt2 = t2->virt;
		t2->virt = t1->virt; // assume equals !
		for(i=0;i<t1->virt->nfields;i++) {
			hl_obj_field *f1 = t1->virt->fields + i;
			hl_obj_field *f2 = t2->virt->fields + i;
			if( f1->hashed_name != f2->hashed_name || !hl_module_same_type(f1->t, f2->t) ) {
				t2->virt = vt2;
				return false;
			}
		}
		t2->virt = vt2;
		return true;
	case HABSTRACT:
		return ucmp(t1->abs_name, t2->abs_name) == 0;
	case HENUM:
		return ucmp(t1->tenum->name, t2->tenum->name) == 0;
	case HPACKED:
		return hl_module_same_type(t1->tparam, t2->tparam);
	default:
		break;
	}
	return false;
}

static int check_same_obj( hl_type_obj *o1, hl_type_obj *o2 ) {
	if( o1->nproto != o2->nproto || o1->nfields != o2->nfields || o1->nbindings != o2->nbindings )
		return -1;
	int i;
	for(i=0;i<o1->nproto;i++)
		if( o1->proto[i].hashed_name != o2->proto[i].hashed_name )
			return -1;
	for(i=0;i<o1->nfields;i++)
		if( o1->fields[i].hashed_name != o2->fields[i].hashed_name || !hl_module_same_type(o1->fields[i].t,o2->fields[i].t) )
			return -1;
	for(i=0;i<o1->nproto;i++)
		if( o1->proto[i].pindex != o2->proto[i].pindex )
			return -2;
	return 0;
}

hl_type *hl_module_resolve_type( hl_module *m, hl_type *t, bool err ) {
	int i;
	if( t->kind != HOBJ && t->kind != HSTRUCT )
		return NULL;
	for(i=0;i<m->code->ntypes;i++) {
		hl_type *t2 = m->code->types + i;
		if( t2->kind == t->kind && ucmp(t->obj->name,t2->obj->name) == 0 ) {
			int r = check_same_obj(t->obj,t2->obj);
			if( r == 0 )
				return t2;
			if( err ) {
				if( r == -2 )
					hl_error("Class '%s' has different prototype than loader (missing @:virtual)",t->obj->name);
				else
					hl_error("Class '%s' has different definition than loader",t->obj->name);
			}
			return NULL;
		}
	}
	return NULL;
}

void hl_module_free( hl_module *m ) {
	for(int i=0;i<m->code->nglobals;i++) {
		if( hl_is_ptr(m->code->globals[i]) )
			hl_remove_root(m->globals_data+m->globals_indexes[i]);
	}
	hl_free(&m->ctx.alloc);
	hl_free_executable_memory(m->code, m->codesize);
#ifdef WIN64_UNWIND_TABLES
	RtlDeleteFunctionTable(m->unwind_table);
	free(m->unwind_table);
#endif
	if( m->hash ) hl_code_hash_free(m->hash);
	if( m->hot ) hl_hot_reload_free(m->hot);
	free(m->hot_slots);
	free(m->functions_indexes);
	free(m->functions_ptrs);
	free(m->ctx.functions_types);
	free(m->globals_indexes);
	free(m->globals_data);
	if( m->jit_debug ) {
		int i;
		for(i=0;i<m->code->nfunctions;i++)
			free(m->jit_debug[i].offsets);
		free(m->jit_debug);
	}
	if( m->jit_ctx )
		hl_jit_free(m->jit_ctx,false);
	free(m);
}
