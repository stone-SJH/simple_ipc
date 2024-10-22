#include "PlatformMutex.h"

PlatformMutex::PlatformMutex()
{
    InitializeCriticalSectionAndSpinCount(&crit_sec, 200);
}

PlatformMutex::~PlatformMutex()
{
    DeleteCriticalSection(&crit_sec);
}

void PlatformMutex::Lock()
{
    EnterCriticalSection(&crit_sec);
}

void PlatformMutex::Unlock()
{
    LeaveCriticalSection(&crit_sec);
}

bool PlatformMutex::TryLock()
{
    return TryEnterCriticalSection(&crit_sec) ? true : false;
}