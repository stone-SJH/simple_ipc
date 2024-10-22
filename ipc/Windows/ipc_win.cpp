#include "../ipc.h"
#include "ipc_win.inl.h"

struct IPC_Stream
{
private:
    Win32NamedPipesIPC::Stream _str;
public:
    IPC_Stream()
    {
        Win32NamedPipesIPC::Create(_str);
    }

    ~IPC_Stream()
    {
        Win32NamedPipesIPC::Destroy(_str);
    }

    operator Win32NamedPipesIPC::Stream& () { return _str; }
};

IPC_Stream* IPC_Create(IPC_ErrorCode* error)
{
    IPC_Stream* stream = new IPC_Stream();
    *error = IPC_Succeeded;
    return stream;
}

void IPC_Destroy(IPC_Stream* stream, IPC_ErrorCode* error)
{
    if (stream)
        stream->~IPC_Stream();
    stream = NULL;

    *error = IPC_Succeeded;
}

void IPC_ConnectTo(IPC_Stream* stream, const char* name, IPC_ErrorCode* error)
{
    *error = Win32NamedPipesIPC::ConnectTo(*stream, name);
}

bool IPC_IsConnected(IPC_Stream* stream, IPC_ErrorCode* error)
{
    bool result = false;
    *error = Win32NamedPipesIPC::IsConnected(*stream, result);
    return result;
}

void IPC_Disconnect(IPC_Stream* stream, IPC_ErrorCode* error)
{
    *error = Win32NamedPipesIPC::Disconnect(*stream);
}


void IPC_Listen(IPC_Stream* stream, const char* name, IPC_ErrorCode* error)
{
    *error = Win32NamedPipesIPC::Listen(*stream, name);
}

bool IPC_IsListening(IPC_Stream* stream, IPC_ErrorCode* error)
{
    *error = IPC_Succeeded;
    return Win32NamedPipesIPC::IsListening(*stream);
}

const char* IPC_GetPlatformSpecificPath(IPC_Stream* stream, IPC_ErrorCode* error)
{
    const char* result = NULL;
    *error = Win32NamedPipesIPC::GetPlatformSpecificPath(*stream, result);
    return result;
}

IPC_Stream* IPC_Accept(IPC_Stream* serverStream, IPC_ErrorCode* error)
{
    IPC_Stream* acceptedStream = NULL;
    *error = Win32NamedPipesIPC::Accept(*serverStream, reinterpret_cast<Win32NamedPipesIPC::Stream**>(&acceptedStream));
    return static_cast<IPC_Stream*>(acceptedStream);
}

IPC_Stream* IPC_AcceptSynchronous(IPC_Stream* stream, IPC_TimeVal timeout, IPC_ErrorCode* error)
{
    IPC_Stream* acceptedStream = NULL;
    *error = Win32NamedPipesIPC::AcceptSynchronous(*stream, reinterpret_cast<Win32NamedPipesIPC::Stream**>(&acceptedStream), timeout);
    return static_cast<IPC_Stream*>(acceptedStream);
}

size_t IPC_Read(IPC_Stream* stream, void* buf, size_t maxLen, IPC_ErrorCode* error)
{
    DWORD bytesRead = 0;
    *error = Win32NamedPipesIPC::Read(*stream, buf, maxLen, bytesRead);
    return bytesRead;
}

size_t IPC_Write(IPC_Stream* stream, const void* buf, size_t len, IPC_ErrorCode* error)
{
    DWORD bytesWritten = 0;
    *error = Win32NamedPipesIPC::Write(*stream, buf, len, bytesWritten);
    return bytesWritten;
}

size_t IPC_ReadSynchronous(IPC_Stream* stream, void* buf, size_t maxLen, IPC_TimeVal timeout, IPC_ErrorCode* error)
{
    DWORD bytesRead = 0;
    *error = Win32NamedPipesIPC::ReadSynchronous(*stream, buf, maxLen, bytesRead, timeout);
    return bytesRead;
}

int IPC_GetConnectedPeerProcessId(IPC_Stream* stream, IPC_ErrorCode* error)
{
    int peerProcessId = 0;
    *error = Win32NamedPipesIPC::GetServerProcessId(*stream, peerProcessId);
    return peerProcessId;
}
