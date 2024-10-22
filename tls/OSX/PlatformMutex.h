#include "../NonCopyable.h"
#include <pthread.h>

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

    pthread_mutex_t mutex;
};