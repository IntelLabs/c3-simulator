#ifndef UNIT_TESTS_INCLUDE_UNIT_TESTS_COMMON_H_
#define UNIT_TESTS_INCLUDE_UNIT_TESTS_COMMON_H_

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <atomic>
#include <stdexcept>

#define kprintf printf
#include "malloc/cc_globals.h"
#include "malloc/try_box.h"

#define RSP_ID_SIZE 6
#define RSP_CIPHERTEXT_SIZE 32
#define RSP_ID_VAL 0x3e
#define RSP_OFFSET_3b 3
#define RSP_FIXED_VAL_SIZE 23
#define RSP_OFFSET_SIZE 22

#define FMASK 0xFFFFFFFFFFFFFFFFULL
#define TOP_CANONICAL_BIT_OFFSET 47

#ifndef C3_MODEL
#define C3_MODEL unspecified
#endif
#define define_to_str(val) _define_to_str(val)
#define _define_to_str(val) #val

constexpr const char *const c3_model = define_to_str(C3_MODEL);

constexpr static inline bool is_model(const char *const str) {
    return std::strcmp(str, c3_model) == 0;
}

constexpr static inline bool model_has_zts() { return is_model("zts"); }

typedef struct {
    uint64_t do_not_use : (64 - RSP_ID_SIZE - RSP_CIPHERTEXT_SIZE);
    uint64_t ciphertext : RSP_CIPHERTEXT_SIZE;
    uint64_t rsp_id : RSP_ID_SIZE;
} rsp_encoded_t;

#define TEST_PASSED printf("Test passed\n")
#define TEST_FAILED printf("Test failed\n")

#define loop_wrapper(func, loop_count)                                         \
    for (int i = 0; i < loop_count; i++) {                                     \
        func;                                                                  \
    }

#define time_wrapper(func)                                                     \
    clock_t start, end;                                                        \
    double cpu_time_used;                                                      \
    start = clock();                                                           \
    func;                                                                      \
    end = clock();                                                             \
    cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;           \
    printf("Execution time %f ms\n", cpu_time_used);

#ifndef MAGIC
#define MAGIC(n)                                                               \
    do {                                                                       \
        int simics_magic_instr_dummy;                                          \
        __asm__ __volatile__("cpuid"                                           \
                             : "=a"(simics_magic_instr_dummy)                  \
                             : "a"(0x4711 | ((unsigned)(n) << 16))             \
                             : "ecx", "edx", "ebx");                           \
    } while (0)
#endif
__attribute__((deprecated))  // use cc_isa_encptr instead!

static inline bool
have_cc_heap_encoding() {
    void *tmp = malloc(1);
    bool r = is_encoded_cc_ptr(tmp);
    free(tmp);
    return r;
}

static inline bool have_cc_stack_encoding() {
    int val;
    void *tmp = (void *)&val;
    bool r = is_encoded_cc_ptr(tmp);
    return r;
}

// __attribute__((deprecated))  // Use cc_isa_encptr(T ptr, size_t size) instead
static inline void *encrypt_la(void *ptr, uint64_t size) {
    return cc_isa_encptr(ptr, size);
}

static inline void *encrypt_la_shared(void *ptr, uint64_t size) {
    ptr_metadata_t md = {0};
    int r = try_box((const uint64_t)ptr, size, &md);
    assert(r && "Try box failed");
    md.unused_ = 1;
    return cc_isa_encptr(ptr, &md);
}

static inline void fill_str(char *str, size_t len) {
    for (auto i = 0UL, end = len - 1; i < end; ++i)
        str[i] = (char)rand();
    str[len - 1] = '\0';
}

/* Debug printers
    Define DEBUG to enable dbgprint
*/

#define errprintf(f, ...) fprintf(stderr, f "\n", ##__VA_ARGS__)
#ifdef DEBUG
#define IFDEBUG(...)                                                           \
    do {                                                                       \
        __VA_ARGS__;                                                           \
    } while (0);
#define dbgprint(f, ...)                                                       \
    errprintf("(%s:%d pid:%d) %s: " f, __FILE__, __LINE__, getpid(), __func__, \
              ##__VA_ARGS__)
#define dbg_dump_uint64(var) dbgprint("%-20s: 0x%016lx", #var, (uint64_t)var);
#else
#define IFDEBUG(...)
#define dbgprint(f, ...)
#define dbg_dump_uint64(...)
#endif  // !DEBUG

/* Helper class to clean up and set up shared memory*/
template <typename T> class sharedmem {
 protected:
    bool created_shm = false;
    void *ptr = nullptr;

 public:
    inline T *operator*() { return static_cast<T *>(ptr); }
    inline uint64_t uint64() { return (uint64_t)ptr; }
    constexpr inline size_t size() { return sizeof(T); }
    virtual T *map(void *) = 0;
};

template <typename T> class sharedmem_posix final : public sharedmem<T> {
    const char *shm_name;
    int fd = -1;

 public:
    sharedmem_posix(const char *shm_name, int oflag = O_RDWR | O_CREAT,
                    mode_t mode = S_IRUSR | S_IWUSR)
        : shm_name(shm_name) {
        dbgprint("shm_open %s, %d, %d", shm_name, oflag, mode);
        fd = shm_open(shm_name, oflag, mode);
        if (fd == -1) {
            throw std::runtime_error("shmget failed");
        }

        /* Resize memory object if we created it */
        if (oflag & O_CREAT) {
            this->created_shm = true;
            if (ftruncate(fd, sizeof(T)) != 0) {
                throw std::runtime_error("ftruncate failed");
            }
        }
    }

    T *map(void *addr = nullptr) override {
        this->ptr = mmap(addr, sizeof(T), PROT_WRITE | PROT_READ, MAP_SHARED,
                         fd, 0);

        if (this->ptr == (void *)-1) {
            throw std::runtime_error("mmap failed");
        }
        return (T *)this->ptr;
    }

    ~sharedmem_posix() {
        if (this->ptr != NULL)
            munmap(this->ptr, sizeof(T));
        if (this->created_shm && fd != -1)
            shm_unlink(shm_name);
    }
};

template <typename T> class sharedmem_systemv final : public sharedmem<T> {
    int shmid = -1;

 public:
    sharedmem_systemv(key_t key, int shmflag = 0666) {
        dbgprint("shmget %d, %lu, %d", key, sizeof(T), shmflag);
        shmid = shmget(key, sizeof(T), shmflag);
        if (shmid < 0) {
            throw std::runtime_error("shmget failed");
        }

        this->created_shm = (shmflag & IPC_CREAT) != 0;
    }

    T *map(void *addr = nullptr) override {
        this->ptr = shmat(shmid, addr, 0);
        if (this->ptr == (void *)-1) {
            throw std::runtime_error("shmat failed");
        }
        return (T *)this->ptr;
    }

    ~sharedmem_systemv() {
        if (this->ptr != nullptr)
            shmdt(this->ptr);
        if (this->created_shm && shmid >= 0)
            shmctl(shmid, IPC_RMID, 0);
    }
};

class atomic_counter {
    std::atomic<uint64_t> m_count;

 public:
    explicit atomic_counter(uint64_t count) : m_count(count) {}

    inline int get() { return m_count; }
    inline void set(int i) { m_count = i; }

    inline void dec(int i = 1) { m_count -= i; }

    inline void inc(int i = 1) { m_count += i; }

    inline bool wait_for_zero() {
        static const timespec ns = {0, 1000};
        for (int i = 0; m_count != 0U; ++i) {
            if (i > 5000)
                return false;
            IFDEBUG({ dbgprint("waiting: %d", i); })
            nanosleep(&ns, NULL);
        }
        dbgprint("returning true");
        return true;
    }
};

__attribute__((deprecated))  // Use is_encoded_cc_stack_ptr instead!
inline bool
is_encoded_stack_ptr(const uint64_t ptr) {
    return is_encoded_cc_stack_ptr(ptr);
}

__attribute__((deprecated))  // use cc_isa_encptr instead!
static inline void *
decrypt_ptr(void *ptr) {
    return cc_isa_decptr(ptr);
}

__attribute__((deprecated))  // use cc_isa_encptr instead!
static inline void *
encrypt_ptr(void *ptr, uint64_t size) {
    return cc_isa_encptr(ptr, size);
}

__attribute__((deprecated))  // use cc_dec_if_encded_ptr instead!
static inline void *
decrypt_ca(void *ptr) {
    return cc_dec_if_encoded_ptr(ptr);
}

#endif  // UNIT_TESTS_INCLUDE_UNIT_TESTS_COMMON_H_
