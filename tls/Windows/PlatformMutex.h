#include <windows.h>
#include <synchapi.h>

#include "../NonCopyable.h"

/**
 *  A platform/api specific mutex class. Always recursive (a single thread can lock multiple times).
 */
class PlatformMutex : public NonCopyable
{
    friend class Mutex;
protected:
    PlatformMutex();
    ~PlatformMutex();

    void Lock();
    void Unlock();
    bool TryLock();

private:

    CRITICAL_SECTION crit_sec;
};