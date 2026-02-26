/*
 * JIT ELF Generation for GDB Debug Support
 *
 * Implements the GDB JIT Compilation Interface to register JIT-compiled
 * code with GDB at runtime. This allows GDB to show proper function names
 * and set breakpoints in JIT code without writing files to disk.
 *
 * See: https://sourceware.org/gdb/current/onlinedocs/gdb/JIT-Interface.html
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "jit_common.h"
#include "jit_elf.h"

/* ========== GDB JIT Interface Global Symbols ========== */
/*
 * These symbols MUST have these exact names and NOT be static.
 * GDB looks for them by name to implement the JIT interface.
 */

struct jit_descriptor __jit_debug_descriptor = {1, JIT_NOACTION, NULL, NULL};

/* GDB sets a breakpoint here. The noinline and memory barrier prevent
 * the compiler from optimizing this away or reordering around it. */
__attribute__((noinline)) void __jit_debug_register_code(void) {
    __asm__ volatile("" ::: "memory");
}

/* ========== ELF Writer Utilities ========== */

typedef struct {
    unsigned char *buf;
    int pos;
    int size;
} elf_writer;

static void elf_init(elf_writer *w, int initial_size) {
    w->buf = (unsigned char *)malloc(initial_size);
    w->pos = 0;
    w->size = initial_size;
}

static void elf_grow(elf_writer *w, int needed) {
    if (w->pos + needed > w->size) {
        int new_size = w->size * 2;
        while (w->pos + needed > new_size)
            new_size *= 2;
        w->buf = (unsigned char *)realloc(w->buf, new_size);
        w->size = new_size;
    }
}

static void elf_write(elf_writer *w, const void *data, int len) {
    elf_grow(w, len);
    memcpy(w->buf + w->pos, data, len);
    w->pos += len;
}

static void elf_write8(elf_writer *w, uint8_t val) {
    elf_grow(w, 1);
    w->buf[w->pos++] = val;
}

static void elf_write16(elf_writer *w, uint16_t val) {
    elf_grow(w, 2);
    memcpy(w->buf + w->pos, &val, 2);
    w->pos += 2;
}

static void elf_write32(elf_writer *w, uint32_t val) {
    elf_grow(w, 4);
    memcpy(w->buf + w->pos, &val, 4);
    w->pos += 4;
}

static void elf_write64(elf_writer *w, uint64_t val) {
    elf_grow(w, 8);
    memcpy(w->buf + w->pos, &val, 8);
    w->pos += 8;
}

static void elf_pad(elf_writer *w, int alignment) {
    int pad = ((w->pos + alignment - 1) & ~(alignment - 1)) - w->pos;
    if (pad > 0) {
        elf_grow(w, pad);
        memset(w->buf + w->pos, 0, pad);
        w->pos += pad;
    }
}

static void elf_free(elf_writer *w) {
    if (w->buf) free(w->buf);
    w->buf = NULL;
}

/* ========== Function Name Generation ========== */

/*
 * Build function name from hl_function structure.
 * Returns allocated string (uses hl_to_utf8 which is GC'd, caller must not free).
 */
static const char *get_function_name(hl_function *f, char *fallback, int fallback_size) {
    if (f->obj) {
        /* Method: ClassName.methodName */
        static char name[512];
        char *cls = hl_to_utf8(f->obj->name);
        char *meth = hl_to_utf8(f->field.name);
        snprintf(name, sizeof(name), "%s.%s", cls, meth);
        return name;
    } else if (f->field.ref) {
        /* Closure: ClassName.~parentMethod.closureIndex */
        static char name[512];
        char *cls = hl_to_utf8(f->field.ref->obj->name);
        char *meth = hl_to_utf8(f->field.ref->field.name);
        snprintf(name, sizeof(name), "%s.~%s.%d", cls, meth, f->ref);
        return name;
    } else {
        /* Anonymous function */
        snprintf(fallback, fallback_size, "fun$%d", f->findex);
        return fallback;
    }
}

/* ========== GDB JIT Registration ========== */

/*
 * Section indices for the minimal ELF we generate.
 * Order: NULL, .text, .symtab, .strtab, .shstrtab
 */
enum {
    SEC_NULL = 0,
    SEC_TEXT,
    SEC_SYMTAB,
    SEC_STRTAB,
    SEC_SHSTRTAB,
    SEC_COUNT
};

struct jit_code_entry *gdb_jit_register(jit_ctx *ctx, hl_module *m,
                                        int code_size, unsigned char *code) {
    elf_writer w;
    elf_writer strtab;    /* .strtab symbol names */
    elf_writer shstrtab;  /* .shstrtab section names */
    int i;

    int num_functions = m->code->nfunctions;
    int num_symbols = num_functions + 1;  /* +1 for _hl_jit_code base symbol */

    /* Section name string offsets */
    int shstr_text, shstr_symtab, shstr_strtab, shstr_shstrtab;

    /* Text section virtual address = actual code address in memory */
    uint64_t text_vaddr = (uint64_t)(uintptr_t)code;

    elf_init(&w, 0x10000);
    elf_init(&strtab, 0x10000);
    elf_init(&shstrtab, 64);

    /* ========== Build .shstrtab (section name strings) ========== */
    elf_write8(&shstrtab, 0);  /* Null string at offset 0 */
    shstr_text = shstrtab.pos;
    elf_write(&shstrtab, ".text", 6);
    shstr_symtab = shstrtab.pos;
    elf_write(&shstrtab, ".symtab", 8);
    shstr_strtab = shstrtab.pos;
    elf_write(&shstrtab, ".strtab", 8);
    shstr_shstrtab = shstrtab.pos;
    elf_write(&shstrtab, ".shstrtab", 10);

    /* ========== Build .strtab (symbol name strings) ========== */
    elf_write8(&strtab, 0);  /* Null string at offset 0 */

    /* Base symbol for the JIT code region */
    int base_sym_name = strtab.pos;
    elf_write(&strtab, "_hl_jit_code", 13);

    /* Function symbols */
    int *sym_names = (int *)malloc(sizeof(int) * num_functions);
    for (i = 0; i < num_functions; i++) {
        hl_function *f = &m->code->functions[i];
        char fallback[64];
        const char *name = get_function_name(f, fallback, sizeof(fallback));
        sym_names[i] = strtab.pos;
        elf_write(&strtab, name, strlen(name) + 1);
    }

    /* ========== Calculate layout ========== */
    /*
     * ELF structure (no program headers for ET_REL):
     *   ELF Header
     *   .symtab (NULL symbol + function symbols)
     *   .strtab (symbol names)
     *   .shstrtab (section names)
     *   Section Headers
     */

    uint64_t ehdr_size = sizeof(Elf64_Ehdr);

    /* .symtab immediately follows ELF header */
    uint64_t symtab_offset = ehdr_size;
    uint64_t symtab_size = (1 + num_symbols) * sizeof(Elf64_Sym);

    /* .strtab follows .symtab */
    uint64_t strtab_offset = symtab_offset + symtab_size;
    uint64_t strtab_size = strtab.pos;

    /* .shstrtab follows .strtab */
    uint64_t shstrtab_offset = strtab_offset + strtab_size;
    uint64_t shstrtab_size = shstrtab.pos;

    /* Section headers at end (8-byte aligned) */
    uint64_t shdr_offset = (shstrtab_offset + shstrtab_size + 7) & ~7;

    /* ========== Write ELF Header ========== */
    /* e_ident */
    elf_write(&w, "\x7f" "ELF", 4);      /* Magic */
    elf_write8(&w, ELFCLASS64);           /* 64-bit */
    elf_write8(&w, ELFDATA2LSB);          /* Little-endian */
    elf_write8(&w, EV_CURRENT);           /* Version */
    elf_write8(&w, 0);                    /* OS/ABI (NONE) */
    elf_write(&w, "\0\0\0\0\0\0\0\0", 8); /* Padding */

    elf_write16(&w, ET_REL);              /* e_type: relocatable */
    elf_write16(&w, EM_AARCH64);          /* e_machine */
    elf_write32(&w, EV_CURRENT);          /* e_version */
    elf_write64(&w, 0);                   /* e_entry (none) */
    elf_write64(&w, 0);                   /* e_phoff (no program headers) */
    elf_write64(&w, shdr_offset);         /* e_shoff */
    elf_write32(&w, 0);                   /* e_flags */
    elf_write16(&w, sizeof(Elf64_Ehdr));  /* e_ehsize */
    elf_write16(&w, 0);                   /* e_phentsize (no program headers) */
    elf_write16(&w, 0);                   /* e_phnum */
    elf_write16(&w, sizeof(Elf64_Shdr));  /* e_shentsize */
    elf_write16(&w, SEC_COUNT);           /* e_shnum */
    elf_write16(&w, SEC_SHSTRTAB);        /* e_shstrndx */

    /* ========== Write .symtab ========== */
    /* NULL symbol (required at index 0) */
    for (i = 0; i < (int)sizeof(Elf64_Sym); i++) elf_write8(&w, 0);

    /* _hl_jit_code base symbol */
    elf_write32(&w, base_sym_name);                      /* st_name */
    elf_write8(&w, ELF64_ST_INFO(STB_GLOBAL, STT_FUNC)); /* st_info */
    elf_write8(&w, 0);                                   /* st_other */
    elf_write16(&w, SEC_TEXT);                           /* st_shndx */
    elf_write64(&w, text_vaddr);                         /* st_value = absolute address */
    elf_write64(&w, code_size);                          /* st_size */

    /* Function symbols */
    for (i = 0; i < num_functions; i++) {
        hl_function *f = &m->code->functions[i];
        hl_debug_infos *dbg = ctx->debug ? &ctx->debug[i] : NULL;

        uint64_t func_addr = text_vaddr;
        uint64_t func_size = 0;

        if (dbg && dbg->offsets) {
            func_addr = text_vaddr + dbg->start;
            func_size = dbg->large ?
                ((int *)dbg->offsets)[f->nops] :
                ((unsigned short *)dbg->offsets)[f->nops];
        }

        elf_write32(&w, sym_names[i]);                       /* st_name */
        elf_write8(&w, ELF64_ST_INFO(STB_GLOBAL, STT_FUNC)); /* st_info */
        elf_write8(&w, 0);                                   /* st_other */
        elf_write16(&w, SEC_TEXT);                           /* st_shndx */
        elf_write64(&w, func_addr);                          /* st_value = absolute address */
        elf_write64(&w, func_size);                          /* st_size */
    }

    /* ========== Write .strtab ========== */
    elf_write(&w, strtab.buf, strtab.pos);

    /* ========== Write .shstrtab ========== */
    elf_write(&w, shstrtab.buf, shstrtab.pos);

    /* ========== Write Section Headers ========== */
    elf_pad(&w, 8);

    /* SEC_NULL */
    for (i = 0; i < (int)sizeof(Elf64_Shdr); i++) elf_write8(&w, 0);

    /* SEC_TEXT - SHT_NOBITS with sh_addr = actual JIT code address */
    elf_write32(&w, shstr_text);              /* sh_name */
    elf_write32(&w, SHT_NOBITS);              /* sh_type (no file data) */
    elf_write64(&w, SHF_ALLOC | SHF_EXECINSTR); /* sh_flags */
    elf_write64(&w, text_vaddr);              /* sh_addr = JIT code address */
    elf_write64(&w, 0);                       /* sh_offset (no file data) */
    elf_write64(&w, code_size);               /* sh_size */
    elf_write32(&w, 0);                       /* sh_link */
    elf_write32(&w, 0);                       /* sh_info */
    elf_write64(&w, 16);                      /* sh_addralign */
    elf_write64(&w, 0);                       /* sh_entsize */

    /* SEC_SYMTAB */
    elf_write32(&w, shstr_symtab);            /* sh_name */
    elf_write32(&w, SHT_SYMTAB);              /* sh_type */
    elf_write64(&w, 0);                       /* sh_flags */
    elf_write64(&w, 0);                       /* sh_addr */
    elf_write64(&w, symtab_offset);           /* sh_offset */
    elf_write64(&w, symtab_size);             /* sh_size */
    elf_write32(&w, SEC_STRTAB);              /* sh_link -> .strtab */
    elf_write32(&w, 1);                       /* sh_info: first non-local symbol */
    elf_write64(&w, 8);                       /* sh_addralign */
    elf_write64(&w, sizeof(Elf64_Sym));       /* sh_entsize */

    /* SEC_STRTAB */
    elf_write32(&w, shstr_strtab);            /* sh_name */
    elf_write32(&w, SHT_STRTAB);              /* sh_type */
    elf_write64(&w, 0);                       /* sh_flags */
    elf_write64(&w, 0);                       /* sh_addr */
    elf_write64(&w, strtab_offset);           /* sh_offset */
    elf_write64(&w, strtab_size);             /* sh_size */
    elf_write32(&w, 0);                       /* sh_link */
    elf_write32(&w, 0);                       /* sh_info */
    elf_write64(&w, 1);                       /* sh_addralign */
    elf_write64(&w, 0);                       /* sh_entsize */

    /* SEC_SHSTRTAB */
    elf_write32(&w, shstr_shstrtab);          /* sh_name */
    elf_write32(&w, SHT_STRTAB);              /* sh_type */
    elf_write64(&w, 0);                       /* sh_flags */
    elf_write64(&w, 0);                       /* sh_addr */
    elf_write64(&w, shstrtab_offset);         /* sh_offset */
    elf_write64(&w, shstrtab_size);           /* sh_size */
    elf_write32(&w, 0);                       /* sh_link */
    elf_write32(&w, 0);                       /* sh_info */
    elf_write64(&w, 1);                       /* sh_addralign */
    elf_write64(&w, 0);                       /* sh_entsize */

    /* ========== Register with GDB ========== */
    struct jit_code_entry *entry = (struct jit_code_entry *)malloc(sizeof(struct jit_code_entry));
    if (!entry) {
        free(sym_names);
        elf_free(&w);
        elf_free(&strtab);
        elf_free(&shstrtab);
        return NULL;
    }

    entry->symfile_addr = (const char *)w.buf;
    entry->symfile_size = w.pos;
    entry->prev_entry = NULL;

    /* Link into the descriptor's list */
    entry->next_entry = __jit_debug_descriptor.first_entry;
    if (__jit_debug_descriptor.first_entry) {
        __jit_debug_descriptor.first_entry->prev_entry = entry;
    }
    __jit_debug_descriptor.first_entry = entry;

    /* Notify GDB */
    __jit_debug_descriptor.relevant_entry = entry;
    __jit_debug_descriptor.action_flag = JIT_REGISTER_FN;
    __jit_debug_register_code();

    fprintf(stderr, "GDB JIT: Registered %d functions (%d bytes ELF, code at %p)\n",
            num_functions, w.pos, code);

    /* Clean up temporary buffers (but NOT w.buf - that's now owned by entry) */
    free(sym_names);
    /* Don't call elf_free(&w) - w.buf is now entry->symfile_addr */
    elf_free(&strtab);
    elf_free(&shstrtab);

    return entry;
}

void gdb_jit_unregister(struct jit_code_entry *entry) {
    if (!entry) return;

    /* Unlink from list */
    if (entry->prev_entry) {
        entry->prev_entry->next_entry = entry->next_entry;
    } else {
        __jit_debug_descriptor.first_entry = entry->next_entry;
    }
    if (entry->next_entry) {
        entry->next_entry->prev_entry = entry->prev_entry;
    }

    /* Notify GDB */
    __jit_debug_descriptor.relevant_entry = entry;
    __jit_debug_descriptor.action_flag = JIT_UNREGISTER_FN;
    __jit_debug_register_code();

    /* Free resources */
    free((void *)entry->symfile_addr);
    free(entry);
}
