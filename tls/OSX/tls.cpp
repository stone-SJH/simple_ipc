#include "../tls.h"
#include <pthread.h>

// On Darwin we can get TLS slot table pointer directly.
#if defined(__arm__)
#include <arm/arch.h>
#endif

#if defined(__i386__) || defined(__x86_64__)

// "Annotating a pointer with address space #256 causes it to be code generated relative to the X86 GS segment register"
// from https://opensource.apple.com/source/clang/clang-137/src/tools/clang/docs/LanguageExtensions.html
#define TLS_Darwin_SlotTable() ((void* __attribute__((address_space(256)))*) NULL)

#elif defined(__arm__) && defined(_ARM_ARCH_7)

static inline void** TLS_Darwin_SlotTable(void)
{
    return (void**)(__builtin_arm_mrc(15, 0, 13, 0, 3) & (~0x3u));
}

#elif defined(__aarch64__)

static inline void** TLS_Darwin_SlotTable(void)
{
    uint64_t tsd;
    __asm__ ("mrs %0, TPIDRRO_EL0" : "=r" (tsd));
    return (void**)(tsd & (~0x7ull));
}

#else

#error TLS_Darwin_SlotTable is not implemented on this platform.

#endif

TLS_HANDLE TLS_ALLOC() {
    pthread_key_t r;
    int rc = pthread_key_create(&r, NULL);
    if (rc != 0)
        return TLS_InvalidHandle;
    return static_cast<TLS_HANDLE>(r);
}

int TLS_FREE(TLS_HANDLE handle) {
    int rc = pthread_key_delete(static_cast<pthread_key_t>(handle));
    return rc == 0;
}

int TLS_SET(TLS_HANDLE handle, uintptr_t value) {
    TLS_Darwin_SlotTable()[handle] = (void*)value;
    return 1;
}

uintptr_t TLS_GET(TLS_HANDLE handle) {
    return (uintptr_t)TLS_Darwin_SlotTable()[handle];
}