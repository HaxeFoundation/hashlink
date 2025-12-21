/*
 * Test ARM64 debugger (mdbg) code quality and bug detection
 *
 * These tests verify that known bugs in mdbg.c have been fixed.
 * Tests will FAIL if bugs are present, PASS when fixed.
 *
 * Compile: cc -o test_mdbg test_mdbg.c -framework CoreFoundation -arch arm64
 * Run: ./test_mdbg
 */

#ifdef __aarch64__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <mach/mach.h>

/* Test result codes - matching test_harness.h */
#define TEST_PASS 0
#define TEST_FAIL 1
#define TEST_SKIP 2

/* Colors for output - matching test_harness.h */
#define GREEN  "\033[32m"
#define RED    "\033[31m"
#define YELLOW "\033[33m"
#define RESET  "\033[0m"

/* Test infrastructure */
typedef int (*test_func_t)(void);

typedef struct {
    const char *name;
    test_func_t func;
} test_entry_t;

#define TEST(name) static int test_##name(void)
#define TEST_ENTRY(name) { #name, test_##name }

static int run_tests(test_entry_t *tests, int count) {
    int passed = 0, failed = 0, skipped = 0;

    printf("\n=== Running %d mdbg tests ===\n\n", count);

    for (int i = 0; i < count; i++) {
        printf("  [%d/%d] %s ... ", i + 1, count, tests[i].name);
        fflush(stdout);

        int result = tests[i].func();

        switch (result) {
            case TEST_PASS:
                printf(GREEN "PASS" RESET "\n");
                passed++;
                break;
            case TEST_FAIL:
                printf(RED "FAIL" RESET "\n");
                failed++;
                break;
            case TEST_SKIP:
                printf(YELLOW "SKIP" RESET "\n");
                skipped++;
                break;
        }
    }

    printf("\n=== Results: %d passed, %d failed, %d skipped ===\n\n",
           passed, failed, skipped);

    return failed > 0 ? 1 : 0;
}

/* Helper: Read file contents */
static char* read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *content = malloc(size + 1);
    if (!content) {
        fclose(f);
        return NULL;
    }

    fread(content, 1, size, f);
    content[size] = '\0';
    fclose(f);

    return content;
}

/* Helper: Check if pattern exists in content */
static bool contains(const char *content, const char *pattern) {
    return strstr(content, pattern) != NULL;
}

/* Helper: Count occurrences of pattern */
static int count_occurrences(const char *content, const char *pattern) {
    int count = 0;
    const char *p = content;
    size_t len = strlen(pattern);

    while ((p = strstr(p, pattern)) != NULL) {
        count++;
        p += len;
    }
    return count;
}

/* Path to mdbg.c - adjust if needed */
#define MDBG_PATH "include/mdbg/mdbg.c"

/* ============================================================
 * Bug #1: Missing semaphore_signal in EXC_BAD_ACCESS handler
 *
 * The EXC_BAD_ACCESS handler must call semaphore_signal()
 * before returning, otherwise session_wait() will timeout.
 * ============================================================ */
TEST(bug1_exc_bad_access_signals_semaphore) {
    char *content = read_file(MDBG_PATH);
    if (!content) {
        fprintf(stderr, "    Cannot read %s\n", MDBG_PATH);
        return TEST_SKIP;
    }

    /*
     * Look for the pattern in EXC_BAD_ACCESS handler:
     *   else if(exception == EXC_BAD_ACCESS) {
     *       ...
     *       semaphore_signal(sess->wait_sem);  <-- MUST EXIST
     *       return KERN_SUCCESS;
     *   }
     *
     * We check that between "exception == EXC_BAD_ACCESS" and next "return KERN_SUCCESS"
     * there is a semaphore_signal call.
     */

    char *bad_access = strstr(content, "exception == EXC_BAD_ACCESS");
    if (!bad_access) {
        fprintf(stderr, "    EXC_BAD_ACCESS handler not found\n");
        free(content);
        return TEST_FAIL;
    }

    /* Find the return statement after EXC_BAD_ACCESS */
    char *return_stmt = strstr(bad_access, "return KERN_SUCCESS");
    if (!return_stmt) {
        fprintf(stderr, "    return statement not found in handler\n");
        free(content);
        return TEST_FAIL;
    }

    /* Check if semaphore_signal exists between EXC_BAD_ACCESS and return */
    size_t range = return_stmt - bad_access;
    char *handler_code = malloc(range + 1);
    strncpy(handler_code, bad_access, range);
    handler_code[range] = '\0';

    bool has_signal = contains(handler_code, "semaphore_signal");
    free(handler_code);
    free(content);

    if (!has_signal) {
        fprintf(stderr, "    MISSING: semaphore_signal() in EXC_BAD_ACCESS handler\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/* ============================================================
 * Bug #1b: Missing semaphore_signal in EXC_BAD_INSTRUCTION handler
 * ============================================================ */
TEST(bug1b_exc_bad_instruction_signals_semaphore) {
    char *content = read_file(MDBG_PATH);
    if (!content) {
        fprintf(stderr, "    Cannot read %s\n", MDBG_PATH);
        return TEST_SKIP;
    }

    char *bad_instr = strstr(content, "exception == EXC_BAD_INSTRUCTION");
    if (!bad_instr) {
        fprintf(stderr, "    EXC_BAD_INSTRUCTION handler not found\n");
        free(content);
        return TEST_FAIL;
    }

    char *return_stmt = strstr(bad_instr, "return KERN_SUCCESS");
    if (!return_stmt) {
        fprintf(stderr, "    return statement not found in handler\n");
        free(content);
        return TEST_FAIL;
    }

    size_t range = return_stmt - bad_instr;
    char *handler_code = malloc(range + 1);
    strncpy(handler_code, bad_instr, range);
    handler_code[range] = '\0';

    bool has_signal = contains(handler_code, "semaphore_signal");
    free(handler_code);
    free(content);

    if (!has_signal) {
        fprintf(stderr, "    MISSING: semaphore_signal() in EXC_BAD_INSTRUCTION handler\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/* ============================================================
 * Bug #2: Memory leak in read_register
 *
 * get_thread_state() allocates memory that must be freed
 * after extracting the register value.
 * ============================================================ */
TEST(bug2_read_register_frees_memory) {
    char *content = read_file(MDBG_PATH);
    if (!content) {
        fprintf(stderr, "    Cannot read %s\n", MDBG_PATH);
        return TEST_SKIP;
    }

    /* Find read_register function */
    char *func_start = strstr(content, "read_register(mach_port_t task");
    if (!func_start) {
        fprintf(stderr, "    read_register function not found\n");
        free(content);
        return TEST_FAIL;
    }

    /* Find end of function (next function or end marker) */
    char *func_end = strstr(func_start, "\nstatic kern_return_t write_register");
    if (!func_end) {
        func_end = func_start + 500; /* Approximate */
    }

    size_t range = func_end - func_start;
    char *func_code = malloc(range + 1);
    strncpy(func_code, func_start, range);
    func_code[range] = '\0';

    /* Check for free() call after get_thread_state or get_debug_state */
    bool has_free = contains(func_code, "free(regs)") ||
                    contains(func_code, "free(state)");

    free(func_code);
    free(content);

    if (!has_free) {
        fprintf(stderr, "    MISSING: free() call in read_register - memory leak!\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/* ============================================================
 * Bug #2b: Memory leak in write_register
 * ============================================================ */
TEST(bug2b_write_register_frees_memory) {
    char *content = read_file(MDBG_PATH);
    if (!content) {
        fprintf(stderr, "    Cannot read %s\n", MDBG_PATH);
        return TEST_SKIP;
    }

    char *func_start = strstr(content, "write_register(mach_port_t task");
    if (!func_start) {
        fprintf(stderr, "    write_register function not found\n");
        free(content);
        return TEST_FAIL;
    }

    char *func_end = strstr(func_start, "\n#pragma mark Memory");
    if (!func_end) {
        func_end = func_start + 800;
    }

    size_t range = func_end - func_start;
    char *func_code = malloc(range + 1);
    strncpy(func_code, func_start, range);
    func_code[range] = '\0';

    bool has_free = contains(func_code, "free(regs)") ||
                    contains(func_code, "free(state)");

    free(func_code);
    free(content);

    if (!has_free) {
        fprintf(stderr, "    MISSING: free() call in write_register - memory leak!\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/* ============================================================
 * Bug #3: Incomplete debug register names
 *
 * get_register_name should handle REG_DR4-REG_DR7 since
 * get_debug_reg handles them.
 * ============================================================ */
TEST(bug3_complete_debug_register_names) {
    char *content = read_file(MDBG_PATH);
    if (!content) {
        fprintf(stderr, "    Cannot read %s\n", MDBG_PATH);
        return TEST_SKIP;
    }

    /* Find get_register_name function */
    char *func_start = strstr(content, "get_register_name(int reg)");
    if (!func_start) {
        fprintf(stderr, "    get_register_name function not found\n");
        free(content);
        return TEST_FAIL;
    }

    char *func_end = strstr(func_start, "#pragma mark");
    if (!func_end) {
        func_end = func_start + 1000;
    }

    size_t range = func_end - func_start;
    char *func_code = malloc(range + 1);
    strncpy(func_code, func_start, range);
    func_code[range] = '\0';

    /* Check for REG_DR4, DR5, DR6, DR7 cases */
    bool has_dr4 = contains(func_code, "REG_DR4");
    bool has_dr5 = contains(func_code, "REG_DR5");
    bool has_dr6 = contains(func_code, "REG_DR6");
    bool has_dr7 = contains(func_code, "REG_DR7");

    free(func_code);
    free(content);

    if (!has_dr4 || !has_dr5 || !has_dr6 || !has_dr7) {
        fprintf(stderr, "    MISSING: REG_DR4-DR7 cases in get_register_name\n");
        fprintf(stderr, "    DR4:%s DR5:%s DR6:%s DR7:%s\n",
                has_dr4 ? "ok" : "MISSING",
                has_dr5 ? "ok" : "MISSING",
                has_dr6 ? "ok" : "MISSING",
                has_dr7 ? "ok" : "MISSING");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/* ============================================================
 * Verification: ARM64 thread state structure size
 * ============================================================ */
TEST(verify_arm64_thread_state_size) {
    size_t expected = 272;
    size_t actual = sizeof(arm_thread_state64_t);

    if (actual != expected) {
        fprintf(stderr, "    Expected %zu bytes, got %zu\n", expected, actual);
        return TEST_FAIL;
    }
    return TEST_PASS;
}

/* ============================================================
 * Verification: ARM64 debug state structure size
 * ============================================================ */
TEST(verify_arm64_debug_state_size) {
    size_t expected = 520;
    size_t actual = sizeof(arm_debug_state64_t);

    if (actual != expected) {
        fprintf(stderr, "    Expected %zu bytes, got %zu\n", expected, actual);
        return TEST_FAIL;
    }
    return TEST_PASS;
}

/* ============================================================
 * Verification: CPSR is 32-bit requiring special handling
 * ============================================================ */
TEST(verify_cpsr_is_32bit) {
    arm_thread_state64_t state;
    if (sizeof(state.__cpsr) != 4) {
        fprintf(stderr, "    __cpsr should be 4 bytes, got %zu\n",
                sizeof(state.__cpsr));
        return TEST_FAIL;
    }
    return TEST_PASS;
}

/* ============================================================
 * Main
 * ============================================================ */
int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    printf("mdbg ARM64 Bug Detection Tests\n");
    printf("================================\n");
    printf("Tests will FAIL if bugs are present, PASS when fixed.\n");

    test_entry_t tests[] = {
        /* Bug detection tests - should FAIL until fixed */
        TEST_ENTRY(bug1_exc_bad_access_signals_semaphore),
        TEST_ENTRY(bug1b_exc_bad_instruction_signals_semaphore),
        TEST_ENTRY(bug2_read_register_frees_memory),
        TEST_ENTRY(bug2b_write_register_frees_memory),
        TEST_ENTRY(bug3_complete_debug_register_names),

        /* Verification tests - should PASS */
        TEST_ENTRY(verify_arm64_thread_state_size),
        TEST_ENTRY(verify_arm64_debug_state_size),
        TEST_ENTRY(verify_cpsr_is_32bit),
    };

    int count = sizeof(tests) / sizeof(tests[0]);
    return run_tests(tests, count);
}

#else /* !__aarch64__ */

#include <stdio.h>

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    printf("mdbg tests are only applicable to ARM64 architecture.\n");
    return 0;
}

#endif /* __aarch64__ */
