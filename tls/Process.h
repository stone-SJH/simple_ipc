#include <string>
#include <vector>
#include "Mutex.h"

#if IPC_WIN32
#include <processthreadsapi.h>
#elif IPC_OSX
#ifdef __OBJC__
@class NSTask;
@class NSFileHandle;
@class NSPipe;
#else
typedef struct objc_object NSTask;
typedef struct objc_object NSFileHandle;
typedef struct objc_object NSPipe;
#endif
#endif

class Process {
public:
	Process(std::string appPath, std::vector<std::string> args);
	~Process();
	bool Launch();
	bool IsRunning() const;
	void Shutdown();
	void Detach();    
    int PId() const;
    int CachedPId() const;

    int GetRefCount() { return m_RefCount; }
    void AddRef() { m_RefCount++; }
    void ReleaseRef()
    {
        m_RefCount--;
        if (m_RefCount <= 0)
        {
            Process* ptr = this;
            if (ptr)
                ptr->~Process();
            ptr = NULL;
        }
    }

private:
    void Cleanup();
    
	std::string m_AppPath;
	std::vector<std::string> m_Args;
	mutable Mutex m_KillMutex;
	bool m_IsKilled;
	volatile int m_RefCount;

#if IPC_WIN32
    PROCESS_INFORMATION m_Task;
#elif IPC_OSX
    NSFileHandle* m_ReadHandle;
    NSFileHandle* m_WriteHandle;
    NSPipe* m_PipeIn;
    NSPipe* m_PipeOut;
    NSTask* m_Task;
#endif
};