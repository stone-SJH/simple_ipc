#pragma once
#include "tls.h"

#if IPC_OSX
#include <stddef.h>
#endif

class RegisterRuntimeInitializeAndCleanup
{
public:
    typedef void CallbackFunction(void* data);
    RegisterRuntimeInitializeAndCleanup(CallbackFunction* Initialize, CallbackFunction* Cleanup, int order = 0, void* data = 0);
    ~RegisterRuntimeInitializeAndCleanup();

    static void ExecuteInitializations();
    static void ExecuteCleanup();

private:

    void Register(CallbackFunction* Initialize, CallbackFunction* Cleanup, int order = 0, void* data = 0);
    void Unregister();
    static bool Sort(const RegisterRuntimeInitializeAndCleanup* lhs, const RegisterRuntimeInitializeAndCleanup* rhs);

    int                                                     m_Order;
    void* m_UserData;
    RegisterRuntimeInitializeAndCleanup::CallbackFunction* m_Init;
    RegisterRuntimeInitializeAndCleanup::CallbackFunction* m_Cleanup;
    bool                                                    m_InitCalled;
    RegisterRuntimeInitializeAndCleanup* m_Next;
    RegisterRuntimeInitializeAndCleanup* m_Prev;

    static RegisterRuntimeInitializeAndCleanup* s_LastRegistered;
};

struct RuntimeInitializeAndCleanupHandler
{
    RuntimeInitializeAndCleanupHandler() { RegisterRuntimeInitializeAndCleanup::ExecuteInitializations(); }
    ~RuntimeInitializeAndCleanupHandler() { RegisterRuntimeInitializeAndCleanup::ExecuteCleanup(); }
};

template<class T>
class ThreadSpecificValue : public ThreadLocalStorage<T>
{
public:
    ThreadSpecificValue()
        : m_Registration(NULL, &ThreadSpecificValue<T>::ReinitializeTLS, -1000000, this)
    {
    }

    ThreadSpecificValue(const ThreadSpecificValue& other) = delete;
    ThreadSpecificValue& operator=(const ThreadSpecificValue& other) = delete;

    ThreadSpecificValue(ThreadSpecificValue&& other) = delete;
    ThreadSpecificValue<T>& operator=(ThreadSpecificValue&& other) = delete;

    using ThreadLocalStorage<T>::operator=;
    using ThreadLocalStorage<T>::operator T;
    using ThreadLocalStorage<T>::operator->;
    using ThreadLocalStorage<T>::operator++;
    using ThreadLocalStorage<T>::operator--;

private:
    static void ReinitializeTLS(void* ptr)
    {
        static_cast<ThreadSpecificValue<T>*>(ptr)->Reset();
    }

    RegisterRuntimeInitializeAndCleanup m_Registration;
};