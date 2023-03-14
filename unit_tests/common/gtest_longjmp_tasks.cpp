// model *

/*
 * NOTE: This depends on the ZTS plaintext portion!!!
 * If it changes, the PLAINTEXT_SIZE define needs to be updated here! (should
 * probably add an interface to query the configuration from the CPU)
 *
 * TLDR: Unit test for ZTS omnetpp17 inompatibility. Can be fixed by stripping
 * encrypted bits from compared stack pointers in the dist() function. The
 * setjmp/longjmp "threads" are okay because stack increments are less than ZTS
 * maximum frame size, and total stack size remains small enough.
 *
 * -----------------------------------------------------------------------------
 *
 * This tests an issue with ZTS and the SPEC omnetpp17 benchmark coroutines
 * implemented using setjmp/longjmp. The cCoroutines / _Tasks are implemented
 * such that a "new" stack is created by creating a setjmp state on the
 * beginning of the stack, and then consuming stack to "fill" the allocated
 * stack space. At this point a new setjmp is set to create the start of the
 * next stack.
 *
 *   |       STACK         |
 *   |                     |
 *   | cCoroutine::init()  |
 *   | SETJMP              |  <- Save main_stack state here     <------\
 *   | eat() * many times  |  \                                         |
 *   |  ~0x220 kB per call |   |                                        |
 *   |  ....               |   | Recurse eat() to fill needed size      |
 *   | eat() * many times  |   |                                        |
 *   |  ~0x220 kB per call |   /                                        |
 *   | SETJMP()            |  <- Save co-routine stack start here       |
 *   | LONGJMP()           |  Return back to the main "thread" ---------/
 *
 * Because each increment (using eat) is less than allowd ZTS, eat() is okay.
 * Similarly, the jumps are okay as long total real stack size does not exceed
 * the maximum supported one.
 *
 * However, the eat() recursion is controlled by dist(), which calculate stack
 * size by comparint stack pointer. Consquently failing if the stack pointers
 * are encrypted!
 */

// #define DEBUG
#include <setjmp.h>
#include <gtest/gtest.h>
#include "unit_tests/common.h"
using namespace std;

#define PLAINTEXT_SIZE 23
#define PLAINTEXT_MASK ~(~0UL << PLAINTEXT_SIZE)
#define strip_rsp_enc_bits(ptr) (PLAINTEXT_MASK & (uintptr_t)ptr)

#define set_rsp_to_reg(x)                                                      \
    do {                                                                       \
        asm("mov %%rsp, %0" : "=r"(x) : :);                                    \
    } while (0)

inline void dump_rsp() {
    volatile uintptr_t r = 0;
    set_rsp_to_reg(r);
    dbgprint("rsp: %lx\n", r);
}

#ifndef __TASK_H
#define __TASK_H

#define JMP_BUF jmp_buf
#define SETJMP setjmp
#define LONGJMP longjmp

//=== some defines
#define SAFETY_AREA 512
#define MIN_STACKSIZE 1024

//
// All "beef" are used to test stack overflow/stack usage by checking
// whether byte patterns placed in the memory have been overwritten by
// the coroutine function's normal stack usage.
//

#define DEADBEEF 0xdeafbeef
#define EATFRAME_MAX (sizeof(_Task) + 200)

typedef void (*_Task_fn)(void *);

struct _Task {
    uint64_t guardbeef1;  // contains DEADBEEF; should stay first field
    JMP_BUF jmpb;         // jump state
    JMP_BUF rst_jmpb;     // jump state restart task
    int used;             // used or free
    unsigned size;        // size of actually allocated block
    _Task *next;          // pointer to next control block

    _Task_fn fnp;         // pointer to task function
    void *arg;            // argument to task function
    uint64_t stack_size;  // requested stack size

    _Task *prevbeef;      // pointer to previous eat() invocation's block
    uint64_t guardbeef2;  // contains DEADBEEF; should stay last field
};

extern _Task main_task;
extern _Task *current_task;
extern JMP_BUF tmp_jmpb;

void task_init(unsigned total_stack, unsigned main_stack);
_Task *task_create(_Task_fn fnp, void *arg, unsigned stack_size);
void task_switchto(_Task *p);
void task_free(_Task *t);
void task_restart(_Task *t);
bool task_testoverflow(_Task *t);
unsigned task_stackusage(_Task *t);

#endif  // __TASK_H

#ifndef __CCOROUTINE_H
#define __CCOROUTINE_H

// select coroutine library
#ifdef _WIN32
#define USE_WIN32_FIBERS
#else
#define USE_PORTABLE_COROUTINES
#endif

#ifdef USE_PORTABLE_COROUTINES
struct _Task;
#endif

/**
 * Prototype for functions that can be used with cCoroutine objects as
 * coroutine bodies.
 * @ingroup EnumsTypes
 */
typedef void (*CoroutineFnp)(void *);

//--------------------------------------------------------------------------

/**
 * Low-level coroutine library. Coroutines are used by cSimpleModule.
 *
 * cCoroutine has platform-dependent implementation:
 *
 * On Windows, it uses the Win32 Fiber API.

 * On other platforms, the implementation a portable coroutine library,
 * first described by Stig Kofoed ("Portable coroutines", see the Manual
 * for a better reference). It creates all coroutine stacks within the main
 * stack, and uses setjmp()/longjmp() for context switching. This
 * implies that the maximum stack space allowed by the operating system
 * for the \opp process must be sufficiently high (several,
 * maybe several hundred megabytes), otherwise a segmentation fault
 * will occur.
 *
 * @ingroup Internals
 */
class cCoroutine {
 protected:
#ifdef USE_WIN32_FIBERS
    LPVOID lpFiber;
    static LPVOID lpMainFiber;
    unsigned stacksize;
#endif
#ifdef USE_PORTABLE_COROUTINES
    _Task *task;
#endif

 public:
    /** Coroutine control */
    //@{

    /**
     * Initializes the coroutine library. This function has to be called
     * exactly once in a program, possibly at the top of main().
     */
    static void init(unsigned total_stack, unsigned main_stack);

    /**
     * Switch to another coroutine. The execution of the current coroutine
     * is suspended and the other coroutine is resumed from the point it
     * last left off.
     */
    static void switchTo(cCoroutine *cor);

    /**
     * Switch to the main coroutine (the one main() runs in).
     */
    static void switchToMain();
    //@}

    /** Constructor, destructor */
    //@{

    /**
     * Sets up a coroutine. The arguments are the function that should be
     * run in the coroutine, a pointer that is passed to the coroutine
     * function, and the stack size.
     */
    bool setup(CoroutineFnp fnp, void *arg, unsigned stack_size);

    /**
     * Constructor.
     */
    cCoroutine();

    /**
     * Destructor.
     */
    virtual ~cCoroutine();
    //@}

    /** Coroutine statistics */
    //@{

    /**
     * Returns true if there was a stack overflow during execution of the
     * coroutine.
     *
     * Windows/Fiber API: Not implemented: always returns false.
     *
     * Portable coroutines: it checks the intactness of a predefined byte
     * pattern (0xdeadbeef) at the stack boundary, and report stack overflow if
     * it was overwritten. The mechanism usually works fine, but occasionally it
     * can be fooled by large uninitialized local variables (e.g. char
     * buffer[256]): if the byte pattern happens to fall in the middle of such a
     * local variable, it may be preserved intact and stack violation is not
     * detected.
     */
    virtual bool hasStackOverflow() const;

    /**
     * Returns the stack size of the coroutine.
     */
    virtual unsigned getStackSize() const;

    /**
     * Returns the amount of stack actually used by the coroutine.
     *
     * Windows/Fiber API: Not implemented, always returns 0.
     *
     * Portable coroutines: It works by checking the intactness of
     * predefined byte patterns (0xdeadbeef) placed in the stack.
     */
    virtual unsigned getStackUsage() const;
    //@}
};

#endif  // __CCOROUTINE_H

// ccoroutine.cc
void cCoroutine::init(unsigned total_stack, unsigned main_stack) {
    task_init(total_stack, main_stack);
}

void cCoroutine::switchTo(cCoroutine *cor) {
    task_switchto(((cCoroutine *)cor)->task);
}

void cCoroutine::switchToMain() { task_switchto(&main_task); }

cCoroutine::cCoroutine() { task = NULL; }

cCoroutine::~cCoroutine() {
    if (task)
        task_free(task);
}

bool cCoroutine::setup(CoroutineFnp fnp, void *arg, unsigned stack_size) {
    task = task_create(fnp, arg, stack_size);
    return task != NULL;
}

bool cCoroutine::hasStackOverflow() const {
    return task == NULL ? false : task_testoverflow(task);
}

unsigned cCoroutine::getStackSize() const {
    return task == NULL ? 0 : task->size;
}

unsigned cCoroutine::getStackUsage() const {
    return task == NULL ? 0 : task_stackusage(task);
}
// ccoroutine.cc EOF

// task.cc
#include <string.h>

_Task main_task;
_Task *current_task = NULL;
JMP_BUF tmp_jmpb;

unsigned dist(_Task *from, _Task *to) {
    char *c1 = (char *)from, *c2 = (char *)to;

    char *dc1 = (char *)decrypt_ca(c1), *dc2 = (char *)decrypt_ca(c2);
    unsigned ddist = (uintptr_t)(dc1 > dc2 ? dc1 - dc2 : dc2 - dc1);

    dbgprint("(0x%016lx, 0x%016lx) -> %016lx", (uintptr_t)c1, (uintptr_t)c2,
             (uintptr_t)(c1 > c2 ? c1 - c2 : c2 - c1));
    dbgprint("(0x%016lx, 0x%016lx) -> %016lx", (uintptr_t)dc1, (uintptr_t)dc2,
             (uintptr_t)(dc1 > dc2 ? dc1 - dc2 : dc2 - dc1));
    c1 = (char *)strip_rsp_enc_bits(c1);
    c2 = (char *)strip_rsp_enc_bits(c2);
    unsigned dist = (uintptr_t)(c1 > c2 ? c1 - c2 : c2 - c1);
    EXPECT_EQ(ddist, dist);
    dbgprint("(0x%016lx, 0x%016lx) -> %016lx", (uintptr_t)c1, (uintptr_t)c2,
             (uintptr_t)(c1 > c2 ? c1 - c2 : c2 - c1));
    return (unsigned)(c1 > c2 ? c1 - c2 : c2 - c1);
}

void eat(_Task *p, unsigned size, _Task *prevbeef) {
    volatile uintptr_t __rsp = 0;
    set_rsp_to_reg(__rsp);
    dbgprint("calleed, rsp at %lx", __rsp);
    unsigned d;
    _Task t;

    /* This function does the lion's share of the job. */
    /* Never returns!  p: caller task */

    /* init beef */
    t.guardbeef1 = DEADBEEF;
    t.guardbeef2 = DEADBEEF;
    t.prevbeef = prevbeef;

    /* eat stack space */
    d = dist(p, &t);
    if (d < size) {
        eat(p, size, &t);
    }

    /* make t a free block and link into task list */
    t.size = p->size - d;  // set sizes
    p->size = d;
    t.used = false;
    t.next = p->next;  // insert into list after p
    p->next = &t;

    /* free block made -- return to caller (task_init() or for(;;) loop below)
     */
    /* next longjmp to us will be from task_create() */
    if (SETJMP(t.jmpb) == 0) {                        // save state
        memcpy(t.rst_jmpb, t.jmpb, sizeof(jmp_buf));  // save this state
        LONGJMP(p->jmpb, 1);                          // return to caller
    }

    /* setup task --> run task --> free block sequence */
    for (;;) {
        /* we get here from task_create() */
        /*  it has put required stack size into t.stack_size */
        if (t.stack_size + MIN_STACKSIZE <= t.size) {  // too big for us?
            if (SETJMP(t.jmpb) == 0) {  // split block to free unused space
                eat(&t, t.stack_size, NULL);  // make free block
            }
        }
        t.used = true;  // mark as used

        /* wait for next longjmp to us (that'll be to run task) */
        if (SETJMP(t.jmpb) == 0) {  // wait
            LONGJMP(tmp_jmpb, 1);   // return to task_create()
        }

        /* run task */
        (*t.fnp)(t.arg);

        /* task finished -- free this block */
        task_free(&t);

        /* job done -- switch to main task */
        if (SETJMP(t.jmpb) == 0) {  // save state
            task_switchto(&main_task);
        }
    }
}

void task_init(unsigned total_stack, unsigned main_stack) {
    _Task tmp;

    tmp.size = total_stack;  // initialize total stack area
    tmp.next = NULL;
    if (SETJMP(tmp.jmpb) == 0) {
        eat(&tmp, main_stack, NULL);  // reserve main stack and create
                                      //   first free task block
    }
    main_task = tmp;  // copy to global variable
    main_task.used = true;
    current_task = &main_task;
}

_Task *task_create(_Task_fn fnp, void *arg, unsigned stack_size) {
    dbgprint("entered");
    _Task *p;

    for (p = main_task.next; p != NULL; p = p->next) {  // find free block
        if (!p->used && p->size >= stack_size) {
            p->fnp = fnp;  // set task parameters
            p->arg = arg;
            p->stack_size = stack_size;
            dbgprint("calling setjmp");
            if (SETJMP(tmp_jmpb) == 0) {  // activate control block
                dbgprint("calling longjmp");
                LONGJMP(p->rst_jmpb, 1);
            }
            dbgprint("returning");
            return p;
        }
        dbgprint("block not free, looking into next");
    }
    dbgprint("not enough stack");
    return NULL;  // not enough stack
}

void task_switchto(_Task *p) {
    if (SETJMP(current_task->jmpb) == 0) {  // save state
        current_task = p;
        LONGJMP(p->jmpb, 1);  // run next task
    }
}

void task_free(_Task *t) {
    t->used = false;  // mark as free
    if (t->next != NULL && !t->next->used) {
        t->size += t->next->size;  // merge with following block
        t->next = t->next->next;
    }

    _Task *p = main_task.next;  // loop through list
    if (p != t) {               // if not first block
        while (p->next != t) {  // locate previous block
            p = p->next;
        }
        if (!p->used) {          // if free
            p->size += t->size;  // then merge
            p->next = t->next;
        }
    }
}

void task_restart(_Task *p) {
    if (SETJMP(tmp_jmpb) == 0) {  // activate control block
        LONGJMP(p->rst_jmpb, 1);
    }
}

bool task_testoverflow(_Task *t) {
    if (!t->used || !t->next)
        return false;
    return t->next->guardbeef1 != DEADBEEF || t->next->guardbeef2 != DEADBEEF;
}

unsigned task_stackusage(_Task *t) {
    if (!t->used)
        return 0;
    if (!t->next)              // if unable to test
        return t->stack_size;  // then return requested size

    _Task *p = t->next;
    if (p->guardbeef1 != DEADBEEF ||  // if overflow
        p->guardbeef2 != DEADBEEF ||
        (p->prevbeef != NULL && dist(p, p->prevbeef) > EATFRAME_MAX))
        return t->size;  // then return actual size

    /* walk backwards -- if the beef are still there, that area is untouched */
    while (p->prevbeef != NULL && p->prevbeef->guardbeef1 == DEADBEEF &&
           p->prevbeef->guardbeef2 == DEADBEEF &&
           dist(p->prevbeef, p->prevbeef->prevbeef) <= EATFRAME_MAX)
        p = p->prevbeef;

    return dist(t, p);
}
// task.cc EOF

#define MAIN_STACK_SIZE (128 * 1024)  // Tkenv needs more than 64K

int g_tester1 = 0;
int g_tester2 = 0;

void tester1(void) {
    g_tester1 += 100;
    dbgprint("calling cCoroutine::switchToMain");
    cCoroutine::switchToMain();
}
void tester2(void) {
    g_tester2 += 10;
    dbgprint("calling cCoroutine::switchToMain");
    cCoroutine::switchToMain();
}

__attribute__((noinline)) void run_once() {
    g_tester1 = 0;
    g_tester2 = 0;
    volatile uintptr_t __rsp = 0;
    set_rsp_to_reg(__rsp);
    dbgprint("calleed, rsp is 0x%016lx", __rsp);
    dbgprint("      decrypted 0x%016lx", (uint64_t)decrypt_ca((void *)__rsp));
    dbgprint("       stripped 0x%016lx", strip_rsp_enc_bits(__rsp));

    dbgprint("calling cCoroutine::init()");
    cCoroutine::init(MAIN_STACK_SIZE + 4096, MAIN_STACK_SIZE);
    cCoroutine task1;
    cCoroutine task2;

    dbgprint("calling task1.setup()");
    task1.setup((CoroutineFnp)tester1, nullptr, 1024);
    dbgprint("calling task2.setup()");
    task2.setup((CoroutineFnp)tester2, nullptr, 1024);

    dbgprint("calling cCoroutine::switchTo(&task1)");
    cCoroutine::switchTo(&task1);
    dbgprint("calling cCoroutine::switchTo(&task2)");
    cCoroutine::switchTo(&task2);

    dbgprint("Done, checking asserts");
    ASSERT_EQ(g_tester1, 100);
    ASSERT_EQ(g_tester2, 10);
    set_rsp_to_reg(__rsp);
    dbgprint("         rsp is 0x%016lx", __rsp);
    dbgprint("      decrypted 0x%016lx", (uint64_t)decrypt_ca((void *)__rsp));
    dbgprint("       stripped 0x%016lx", strip_rsp_enc_bits(__rsp));
    dbgprint("A'okay, returning");
}

// Jump multiple stack frames back
TEST(LONGJMP_TASK, omnetpp17) { run_once(); }

//
// The following fail on native model, so will not debug further.
// Suspect that the stacks stack overflow into each-other.
//
// TEST(LONGJMP_TASK, omnetpp17_many_times_bad_one) {
//     int i = 256;
//     for (int j = 0; j < i; ++j) {
//         asm("push %rax");
//         asm("push %rax");
//     }
//     // fprintf(stderr, "%04d - ", i);
//     run_once();
//     for (int j = 0; j < i; ++j) {
//         asm("pop %rax");
//         asm("pop %rax");
//     }
// }
//
// TEST(LONGJMP_TASK, omnetpp17_many_times) {
//     // Try to cause mismatch by shifting the stack around a bit
//     for (int i = 1; i <= 1024; ++i) {
//         for (int j = 0; j < i; ++j) {
//             asm("push %rax");
//             asm("push %rax");
//         }
//         run_once();
//
//         for (int j = 0; j < i; ++j) {
//             asm("pop %rax");
//             asm("pop %rax");
//         }
//     }
// }

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
