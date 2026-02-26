/*
 * JIT ELF Generation for Debug/Profiler Support
 *
 * When HL_JIT_DEBUG=1 is set, registers JIT-compiled code with GDB via
 * the GDB JIT interface. This allows GDB to show proper function names
 * for JIT-compiled code without writing files to disk.
 *
 * NOTE: This header must be included AFTER jit_common.h which provides
 * the necessary type definitions (jit_ctx, hl_module, etc.)
 */

#ifndef JIT_ELF_H
#define JIT_ELF_H

#include <stdint.h>

/* ========== GDB JIT Interface ========== */
/*
 * These structures and symbols implement the GDB JIT Compilation Interface.
 * See: https://sourceware.org/gdb/current/onlinedocs/gdb/JIT-Interface.html
 *
 * GDB sets a breakpoint on __jit_debug_register_code(). When called,
 * GDB reads __jit_debug_descriptor to find newly registered ELF objects.
 */

typedef enum {
    JIT_NOACTION = 0,
    JIT_REGISTER_FN,
    JIT_UNREGISTER_FN
} jit_actions_t;

struct jit_code_entry {
    struct jit_code_entry *next_entry;
    struct jit_code_entry *prev_entry;
    const char *symfile_addr;
    uint64_t symfile_size;
};

struct jit_descriptor {
    uint32_t version;
    uint32_t action_flag;
    struct jit_code_entry *relevant_entry;
    struct jit_code_entry *first_entry;
};

/* Global symbols required by GDB JIT interface (defined in jit_elf.c) */
extern struct jit_descriptor __jit_debug_descriptor;
void __jit_debug_register_code(void);

/*
 * Register JIT code with GDB via the JIT interface.
 *
 * Creates an in-memory ELF object with debug symbols and registers it
 * with GDB. No files are written to disk.
 *
 * Parameters:
 *   ctx       - JIT context with debug info
 *   m         - HashLink module with function metadata
 *   code_size - Size of generated code in bytes
 *   code      - Pointer to generated machine code (already allocated)
 *
 * Returns:
 *   jit_code_entry pointer on success (for later cleanup), NULL on failure
 */
struct jit_code_entry *gdb_jit_register(jit_ctx *ctx, hl_module *m,
                                        int code_size, unsigned char *code);

/*
 * Unregister JIT code from GDB.
 *
 * Call this when freeing the JIT code to inform GDB the symbols are no
 * longer valid.
 *
 * Parameters:
 *   entry - The entry returned by gdb_jit_register()
 */
void gdb_jit_unregister(struct jit_code_entry *entry);

/* ELF constants for AArch64 */
#define ELF_MAGIC       "\x7f" "ELF"
#define ELFCLASS64      2
#define ELFDATA2LSB     1       /* Little-endian */
#define EV_CURRENT      1
#define ET_REL          1       /* Relocatable object (used for GDB JIT) */
#define ET_DYN          3       /* Shared object */
#define EM_AARCH64      183

/* Section header types */
#define SHT_NULL        0
#define SHT_PROGBITS    1
#define SHT_SYMTAB      2
#define SHT_STRTAB      3
#define SHT_NOBITS      8       /* No file data, just address/size (used for .text in GDB JIT) */

/* Section header flags */
#define SHF_WRITE       1
#define SHF_ALLOC       2
#define SHF_EXECINSTR   4

/* Symbol binding/type */
#define STB_LOCAL       0
#define STB_GLOBAL      1
#define STT_NOTYPE      0
#define STT_FUNC        2

#define ELF64_ST_INFO(bind, type) (((bind) << 4) | ((type) & 0xf))

/* Special section indices */
#define SHN_UNDEF       0
#define SHN_ABS         0xfff1

/* ELF64 structures */
typedef struct {
    unsigned char e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} Elf64_Ehdr;

typedef struct {
    uint32_t sh_name;
    uint32_t sh_type;
    uint64_t sh_flags;
    uint64_t sh_addr;
    uint64_t sh_offset;
    uint64_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint64_t sh_addralign;
    uint64_t sh_entsize;
} Elf64_Shdr;

typedef struct {
    uint32_t st_name;
    unsigned char st_info;
    unsigned char st_other;
    uint16_t st_shndx;
    uint64_t st_value;
    uint64_t st_size;
} Elf64_Sym;

#endif /* JIT_ELF_H */
