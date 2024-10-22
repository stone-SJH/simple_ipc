#import <Foundation/Foundation.h>
#include <iostream>
#include "../Process.h"

Process::Process(std::string appPath, std::vector<std::string> args) :
        m_Args(args), m_RefCount(1), m_AppPath(appPath), m_IsKilled(false)
{
}

Process::~Process(){
    if (IsRunning() && !m_IsKilled)
        Shutdown();
    Cleanup();
}

bool Process::Launch() {
    Mutex::AutoLock lock(m_KillMutex);
    if (m_IsKilled)
    {
        std::cout << "Cannot launch a process that has already been killed" << std::endl;
        return false;
    }
    else if (m_Task != NULL)
    {
        std::cout << "Cannot launch an already running task" << std::endl;
        return false;
    }

    std::cout << "Launching external process: " << m_AppPath << std::endl;
    @autoreleasepool
    {
        m_Task = [[NSTask alloc] init];
        m_Task.launchPath = [NSString stringWithUTF8String: m_AppPath.c_str()];

        //TODO[stone]: set m_Task.currentDirectoryPath

        m_PipeOut = [[NSPipe alloc] init];
        m_ReadHandle = [m_PipeOut fileHandleForReading];
        m_Task.standardOutput = m_PipeOut;

        m_PipeIn = [[NSPipe alloc] init];
        m_WriteHandle = [m_PipeIn fileHandleForWriting];
        m_Task.standardInput = m_PipeIn;

        // Set argument list
        NSMutableArray* nsargs = [NSMutableArray arrayWithCapacity: m_Args.size()];
        for (unsigned i = 0, n = m_Args.size(); i < n; ++i)
            [nsargs addObject: [NSString stringWithUTF8String: m_Args[i].c_str()]];
        m_Task.arguments = nsargs;

        @try
        {
            [m_Task launch];
        }
        @catch (NSException* e)
        {
            Cleanup();
            return false;
        }
    }
    return true;
}

bool Process::IsRunning() const{
    bool killed;
    {
        Mutex::AutoLock lock(m_KillMutex);
        killed = m_IsKilled;
    }
    return !killed && m_Task && [m_Task isRunning];
}

void Process::Shutdown() {
    Mutex::AutoLock alock(m_KillMutex);
    m_IsKilled = true;

    // Wait until exit is needed in order for the task to properly deallocate
    if (m_Task && [m_Task isRunning])
    {
        [m_Task terminate];

        // [m_Task waitUntilExit]; not good -> blocking
        float retryInSeconds = 0.1f; // how frequently we recheck the task status
        float terminateTimeoutSeconds = 5.0f;

        while (m_Task && [m_Task isRunning] && terminateTimeoutSeconds > 0)
        {
            usleep(retryInSeconds * 1000000);
            terminateTimeoutSeconds -= retryInSeconds;
        }
    }
}

void Process::Detach() {
    Cleanup();
}

int Process::PId() const {
    return IsRunning() ? [m_Task processIdentifier] : 0;
}

int Process::CachedPId() const {
    return [m_Task processIdentifier];
}

void Process::Cleanup() {
    [m_ReadHandle closeFile];
    m_ReadHandle = nil;
    [m_WriteHandle closeFile];
    m_WriteHandle = nil;
    m_PipeIn = nil;
    m_PipeIn = nil;
    m_Task = nil;
}


