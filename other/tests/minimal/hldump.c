/*
 * Simple HashLink bytecode dumper
 * Dumps functions and opcodes from a .hl file
 */
#include <hl.h>
#include <hlmodule.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Opcode names from opcodes.h */
static const char *opcode_names[] = {
    "OMov", "OInt", "OFloat", "OBool", "OBytes", "OString", "ONull",
    "OAdd", "OSub", "OMul", "OSDiv", "OUDiv", "OSMod", "OUMod",
    "OShl", "OSShr", "OUShr", "OAnd", "OOr", "OXor",
    "ONeg", "ONot", "OIncr", "ODecr",
    "OCall0", "OCall1", "OCall2", "OCall3", "OCall4", "OCallN", "OCallMethod", "OCallThis", "OCallClosure",
    "OStaticClosure", "OInstanceClosure", "OVirtualClosure",
    "OGetGlobal", "OSetGlobal",
    "OField", "OSetField", "OGetThis", "OSetThis",
    "ODynGet", "ODynSet",
    "OJTrue", "OJFalse", "OJNull", "OJNotNull", "OJSLt", "OJSGte", "OJSGt", "OJSLte", "OJULt", "OJUGte", "OJNotLt", "OJNotGte", "OJEq", "OJNotEq", "OJAlways",
    "OToDyn", "OToSFloat", "OToUFloat", "OToInt", "OSafeCast", "OUnsafeCast", "OToVirtual",
    "OLabel", "ORet", "OThrow", "ORethrow", "OSwitch", "ONullCheck", "OTrap", "OEndTrap",
    "OGetI8", "OGetI16", "OGetMem", "OGetArray", "OSetI8", "OSetI16", "OSetMem", "OSetArray",
    "ONew", "OArraySize", "OType", "OGetType", "OGetTID",
    "ORef", "OUnref", "OSetref",
    "OMakeEnum", "OEnumAlloc", "OEnumIndex", "OEnumField", "OSetEnumField",
    "OAssert", "ORefData", "ORefOffset",
    "ONop", "OPrefetch", "OAsm", "OCatch"
};

static const char *type_kind_name(hl_type_kind k) {
    switch (k) {
        case HVOID: return "void";
        case HUI8: return "u8";
        case HUI16: return "u16";
        case HI32: return "i32";
        case HI64: return "i64";
        case HF32: return "f32";
        case HF64: return "f64";
        case HBOOL: return "bool";
        case HBYTES: return "bytes";
        case HDYN: return "dyn";
        case HFUN: return "fun";
        case HOBJ: return "obj";
        case HARRAY: return "array";
        case HTYPE: return "type";
        case HREF: return "ref";
        case HVIRTUAL: return "virtual";
        case HDYNOBJ: return "dynobj";
        case HABSTRACT: return "abstract";
        case HENUM: return "enum";
        case HNULL: return "null";
        case HMETHOD: return "method";
        case HSTRUCT: return "struct";
        case HPACKED: return "packed";
        default: return "???";
    }
}

static void print_type(hl_type *t) {
    if (!t) {
        printf("null");
        return;
    }
    printf("%s", type_kind_name(t->kind));
    if (t->kind == HOBJ && t->obj && t->obj->name) {
        printf("(%ls)", (wchar_t*)t->obj->name);
    } else if (t->kind == HFUN && t->fun) {
        printf("(");
        for (int i = 0; i < t->fun->nargs; i++) {
            if (i > 0) printf(",");
            print_type(t->fun->args[i]);
        }
        printf(")->");
        print_type(t->fun->ret);
    }
}

static void dump_function(hl_code *c, hl_function *f, int verbose) {
    printf("\n=== Function %d ===\n", f->findex);
    printf("  Type: ");
    print_type(f->type);
    printf("\n");
    printf("  Registers: %d\n", f->nregs);
    printf("  Opcodes: %d\n", f->nops);

    if (verbose) {
        printf("  Register types:\n");
        for (int i = 0; i < f->nregs && i < 20; i++) {
            printf("    r%d: ", i);
            print_type(f->regs[i]);
            printf("\n");
        }
        if (f->nregs > 20) printf("    ... (%d more)\n", f->nregs - 20);
    }

    printf("  Code:\n");
    for (int i = 0; i < f->nops; i++) {
        hl_opcode *op = &f->ops[i];
        const char *name = (op->op < sizeof(opcode_names)/sizeof(opcode_names[0]))
                          ? opcode_names[op->op] : "???";
        printf("    %4d: %-16s %d, %d, %d", i, name, op->p1, op->p2, op->p3);

        /* Show extra info for some opcodes */
        switch (op->op) {
            case OInt:
                if (op->p2 >= 0 && op->p2 < c->nints)
                    printf("  ; r%d = %d", op->p1, c->ints[op->p2]);
                break;
            case OString:
                if (op->p2 >= 0 && op->p2 < c->nstrings)
                    printf("  ; r%d = \"%s\"", op->p1, c->strings[op->p2]);
                break;
            case OBool:
                printf("  ; r%d = %s", op->p1, op->p2 ? "true" : "false");
                break;
            case OCall0:
            case OCall1:
            case OCall2:
            case OCall3:
            case OCall4:
            case OCallN:
                printf("  ; call F%d", op->p2);
                break;
            case OJAlways:
                printf("  ; goto %d", (i + 1) + op->p1);
                break;
            case OJTrue:
            case OJFalse:
            case OJNull:
            case OJNotNull:
                printf("  ; if r%d goto %d", op->p1, (i + 1) + op->p2);
                break;
            case OJSLt:
            case OJSGte:
            case OJEq:
            case OJNotEq:
                printf("  ; if r%d,r%d goto %d", op->p1, op->p2, (i + 1) + op->p3);
                break;
            case ORet:
                printf("  ; return r%d", op->p1);
                break;
            case OGetGlobal:
                printf("  ; r%d = global[%d]", op->p1, op->p2);
                break;
            case OSetGlobal:
                printf("  ; global[%d] = r%d", op->p2, op->p1);
                break;
            case OField:
                printf("  ; r%d = r%d.field[%d]", op->p1, op->p2, op->p3);
                break;
            case OSetField:
                printf("  ; r%d.field[%d] = r%d", op->p1, op->p2, op->p3);
                break;
            case ONew:
                printf("  ; r%d = new", op->p1);
                break;
            default:
                break;
        }
        printf("\n");
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file.hl> [function_index | -a] [-v]\n", argv[0]);
        fprintf(stderr, "  -a: dump all functions\n");
        fprintf(stderr, "  -v: verbose (show register types)\n");
        return 1;
    }

    const char *filename = argv[1];
    int target_func = -1;  /* -1 means entrypoint only */
    int dump_all = 0;
    int verbose = 0;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-a") == 0) {
            dump_all = 1;
        } else {
            target_func = atoi(argv[i]);
        }
    }

    /* Initialize HL */
    hl_global_init();

    /* Load the bytecode */
    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Cannot open %s\n", filename);
        return 1;
    }

    fseek(f, 0, SEEK_END);
    int size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *data = malloc(size);
    fread(data, 1, size, f);
    fclose(f);

    /* Parse bytecode */
    char *error_msg = NULL;
    hl_code *code = hl_code_read((unsigned char*)data, size, &error_msg);
    free(data);

    if (!code) {
        fprintf(stderr, "Failed to parse bytecode: %s\n", error_msg ? error_msg : "unknown error");
        return 1;
    }

    /* Print summary */
    printf("HashLink Bytecode: %s\n", filename);
    printf("  Version: %d\n", code->version);
    printf("  Entrypoint: F%d\n", code->entrypoint);
    printf("  Types: %d\n", code->ntypes);
    printf("  Globals: %d\n", code->nglobals);
    printf("  Natives: %d\n", code->nnatives);
    printf("  Functions: %d\n", code->nfunctions);
    printf("  Strings: %d\n", code->nstrings);
    printf("  Ints: %d\n", code->nints);
    printf("  Floats: %d\n", code->nfloats);

    /* Print natives */
    if (code->nnatives > 0) {
        printf("\n=== Natives ===\n");
        for (int i = 0; i < code->nnatives; i++) {
            hl_native *n = &code->natives[i];
            printf("  F%d: %s@%s ", n->findex, n->name, n->lib);
            print_type(n->t);
            printf("\n");
        }
    }

    /* Dump functions */
    if (dump_all) {
        /* Dump all functions */
        printf("\n--- All Functions ---\n");
        for (int i = 0; i < code->nfunctions; i++) {
            dump_function(code, &code->functions[i], verbose);
        }
    } else if (target_func >= 0) {
        int found = 0;
        /* Check if it's a native function first */
        for (int i = 0; i < code->nnatives; i++) {
            if (code->natives[i].findex == target_func) {
                hl_native *n = &code->natives[i];
                printf("\n=== Native %d ===\n", n->findex);
                printf("  Library: %s\n", n->lib);
                printf("  Name: %s\n", n->name);
                printf("  Type: ");
                print_type(n->t);
                printf("\n");
                found = 1;
                break;
            }
        }
        /* Find and dump specific function */
        for (int i = 0; i < code->nfunctions; i++) {
            if (code->functions[i].findex == target_func) {
                dump_function(code, &code->functions[i], verbose);
                found = 1;
                break;
            }
        }
        if (!found) {
            printf("\nFunction F%d not found\n", target_func);
        }
    } else {
        /* Dump entrypoint function */
        printf("\n--- Entrypoint Function ---\n");
        for (int i = 0; i < code->nfunctions; i++) {
            if (code->functions[i].findex == code->entrypoint) {
                dump_function(code, &code->functions[i], verbose);
                break;
            }
        }
    }

    return 0;
}
