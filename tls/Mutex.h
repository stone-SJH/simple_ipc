#pragma once

#include "NonCopyable.h"
#if IPC_WIN32
#include "Windows/PlatformMutex.h"
#elif IPC_OSX
#include "OSX/PlatformMutex.h"
#endif

class Mutex : NonCopyable
{
public:

    class AutoLock : NonCopyable
    {
        Mutex& m_Mutex;

    public:
        AutoLock(Mutex& mutex) : m_Mutex(mutex) { m_Mutex.Lock(); }
        ~AutoLock() { m_Mutex.Unlock(); }
    };

    Mutex();
    ~Mutex();

    void Lock();
    void Unlock();

    // Returns true if locking succeeded
    bool TryLock();
    void BlockUntilUnlocked();

private:
    PlatformMutex m_Mutex;
};
