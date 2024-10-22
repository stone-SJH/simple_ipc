#include "../Process.h"

#include <iostream>
#include <algorithm>
#include <codecvt>

Process::Process(std::string appPath, std::vector<std::string> args) : 
	m_RefCount(1), m_AppPath(appPath), m_Args(args), m_IsKilled(false)
{
	memset(&m_Task, 0, sizeof(m_Task));
	m_Task.hProcess = INVALID_HANDLE_VALUE;
}

Process::~Process() {
    if (IsRunning() && !m_IsKilled)
        Shutdown();
    Cleanup();
}

inline std::wstring convert(const std::string& s)
{
    if (s.empty()) return std::wstring();
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring ws = converter.from_bytes(s);
    return ws;
}

inline std::string convert(const std::wstring& ws) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::string s = converter.to_bytes(ws);
    return s;
}

std::string getMsgByErrorCode(DWORD code) {
    LPWSTR msgBuf = NULL;
    if (!FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&msgBuf, 0, NULL)) {
        char buf[100];
        snprintf(buf, 100, "Unknown error [%i]", code);
        return buf;
    }
    else {
        std::string msg = convert(msgBuf);
        LocalFree(msgBuf);
        return msg;
    }
}


bool Process::Launch() {
	Mutex::AutoLock lock(m_KillMutex);
    if (m_IsKilled)
    {
        std::cout << "Cannot launch a process that has already been killed" << std::endl;
        return false;
    }
    else if (m_Task.hProcess != INVALID_HANDLE_VALUE)
    {
        std::cout << "Cannot launch an already running task" << std::endl;
        return false;
    }

    std::wstring wideAppPath = convert(m_AppPath);
    std::replace(wideAppPath.begin(), wideAppPath.end(), L'/', L'\\');
    
    // Now create a child process
    STARTUPINFOEXW siStartInfo;
    ZeroMemory(&m_Task, sizeof(m_Task));
    ZeroMemory(&siStartInfo, sizeof(siStartInfo));
    siStartInfo.StartupInfo.cb = sizeof(STARTUPINFOEXW);
    siStartInfo.StartupInfo.dwFlags = STARTF_USESHOWWINDOW;

    std::wstring cmd = L'\"' + wideAppPath + L'\"';
    for (int i = 0; i < m_Args.size(); i++) {
        cmd += L' ';
        std::wstring wideArgPath = convert(m_Args[i]);
        std::replace(wideArgPath.begin(), wideArgPath.end(), L'/', L'\\');
        cmd += wideArgPath;
    }
    wchar_t* wstr_cmd = _wcsdup(cmd.c_str());

    BOOL processResult = CreateProcessW(
        wideAppPath.c_str(), 
        wstr_cmd,
        NULL, 
        NULL,
        false, 
        0, 
        NULL,
        NULL,     // TODO[stone]: add working directory
        &siStartInfo.StartupInfo,
        &m_Task); 

    if (processResult == FALSE)
    {
        Cleanup();
        return false;
    }

    if (!CloseHandle(m_Task.hThread))
        std::cout << "Error closing external process " << m_AppPath << ": " << getMsgByErrorCode(GetLastError()) <<std::endl;

    return true;

}

bool Process::IsRunning() const {
    Mutex::AutoLock mut(m_KillMutex);
    if (m_IsKilled)
        return false;
    if (m_Task.hProcess == INVALID_HANDLE_VALUE)
        return false;
    DWORD result = WaitForSingleObject(m_Task.hProcess, 0);
    if (result == WAIT_FAILED) {
        std::cout << "Error checking running status of external process " << m_AppPath << ": " << getMsgByErrorCode(GetLastError()) << std::endl;
        return false;
    }
    return result == WAIT_TIMEOUT;
}

void Process::Shutdown() {
    Mutex::AutoLock mut(m_KillMutex);
    m_IsKilled = true;
    if (m_Task.hProcess != INVALID_HANDLE_VALUE)
    {
        // Make sure the process has exited. To avoid a race condition, do the operations in this order:
        //   1) Call TerminateProcess() whether or not the process is finished.
        //   2) Call GetLastError() if TerminateProcess() failed and cache the result. We may not need it.
        //   3) Call GetExitCodeProcess() and verify that the process is actually dead. If its exit code
        //      is STILL_ACTIVE, it's not terminated. Log the cached error if the process is still alive.

        DWORD cachedWin32Error = 0;

        if (!TerminateProcess(m_Task.hProcess, 1))
        {
            cachedWin32Error = GetLastError();
        }

        DWORD exitCode = 0;

        if (!GetExitCodeProcess(m_Task.hProcess, &exitCode))
        {
            std::cout << "Error querying external process" << m_AppPath << ": " << getMsgByErrorCode(GetLastError()) << std::endl;
        }
        else if (exitCode == STILL_ACTIVE && cachedWin32Error != ERROR_SUCCESS)
        {
            std::cout << "Error terminating external process" << m_AppPath << ": " << getMsgByErrorCode(GetLastError()) << std::endl;
        }

        CloseHandle(m_Task.hProcess);
        m_Task.hProcess = INVALID_HANDLE_VALUE;
    }
}

void Process::Detach() {
    Cleanup();
}

int Process::PId() const {
    return IsRunning() ? m_Task.dwProcessId : 0;
}

int Process::CachedPId() const {
    return m_Task.dwProcessId;
}

void Process::Cleanup() {
    if (m_Task.hProcess != INVALID_HANDLE_VALUE)
    {
        if (!CloseHandle(m_Task.hProcess))
        {
            std::cout << "Error cleaning up external process" << m_AppPath << ": " << getMsgByErrorCode(GetLastError()) << std::endl;
        }
        m_Task.hProcess = INVALID_HANDLE_VALUE;
    }
}
