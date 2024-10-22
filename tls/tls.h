#pragma once
#include "NonCopyable.h"
#if IPC_WIN32
#include <windows.h>
#endif
#include <cstdint>

typedef uintptr_t TLS_HANDLE;
static const uint32_t TLS_MinimumGuaranteedSlots = 100;
static constexpr uintptr_t TLS_InvalidHandle = UINTPTR_MAX;

TLS_HANDLE TLS_ALLOC(void);
int TLS_FREE(TLS_HANDLE handle);
int TLS_SET(TLS_HANDLE handle, uintptr_t value);
uintptr_t TLS_GET(TLS_HANDLE handle);

template<typename T>
class ThreadLocalStorage : NonCopyable
{
public:
    ThreadLocalStorage()
    {
        static_assert(sizeof(T) <= sizeof(uintptr_t), "Provided type is too large to be stored in ThreadLocalStorage");
        handle = TLS_ALLOC();
    }

    ~ThreadLocalStorage()
    {
        if (IsValid())
        {
            TLS_FREE(handle);
            handle = TLS_InvalidHandle;
        }
    }

    ThreadLocalStorage(ThreadLocalStorage&& other)
    {
        // ensure that we don't leak local handle
        if (handle != TLS_InvalidHandle)
            TLS_FREE(handle);
        handle = other.handle;
        other.handle = TLS_InvalidHandle;
    }

    // Check if variable is valid.
    // The only case when variable might be invalid is if it was moved to some other instance.
    inline bool IsValid() const
    {
        return handle != TLS_InvalidHandle;
    }

    // Resets value in all threads.
    void Reset()
    {
        TLS_FREE(handle);
        handle = TLS_ALLOC();
    }

    inline T operator=(T value)
    {
        TLS_SET(handle, (uintptr_t)value);
        return value;
    }

    inline ThreadLocalStorage<T>& operator=(ThreadLocalStorage&& other)
    {
        // swap values
        TLS_HANDLE t = handle;
        handle = other.handle;
        other.handle = t;
        return *this;
    }

    inline operator T() const
    {
        return (T)TLS_GET(handle);
    }

    inline T operator->() const
    {
        return (T)TLS_GET(handle);
    }

    inline T operator++()
    {
        *this = *this + 1;
        return *this;
    }

    inline T operator--()
    {
        *this = *this - 1;
        return *this;
    }

private:
    TLS_HANDLE handle = TLS_InvalidHandle;
};