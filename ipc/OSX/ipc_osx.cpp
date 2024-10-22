#include "../ipc.h"
#include "ipc_osx.inl.h"

#include <sys/ucred.h>
#include <sys/socket.h>

struct IPC_Stream
{
private:
    PosixDomainSocketsIPC::Stream _str;
public:
    IPC_Stream()
    {
        PosixDomainSocketsIPC::Create(_str);
    }

    ~IPC_Stream()
    {
        PosixDomainSocketsIPC::Destroy(_str);
    }

    operator PosixDomainSocketsIPC::Stream& () { return _str; }
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
    *error = PosixDomainSocketsIPC::ConnectTo(*stream, name);
}

bool IPC_IsConnected(IPC_Stream* stream, IPC_ErrorCode* error)
{
    bool result = false;
    *error = PosixDomainSocketsIPC::IsConnected(*stream, result);
    return result;
}

void IPC_Disconnect(IPC_Stream* stream, IPC_ErrorCode* error)
{
    *error = PosixDomainSocketsIPC::Disconnect(*stream);
}

void IPC_Listen(IPC_Stream* stream, const char* name, IPC_ErrorCode* error)
{
    *error = PosixDomainSocketsIPC::Listen(*stream, name);
}

bool IPC_IsListening(IPC_Stream* stream, IPC_ErrorCode* error)
{
    bool result = false;
    *error = PosixDomainSocketsIPC::IsListening(*stream, result);
    return result;
}

const char* IPC_GetPlatformSpecificPath(IPC_Stream* stream, IPC_ErrorCode* error)
{
    const char* result = NULL;
    *error = PosixDomainSocketsIPC::GetSocketPath(*stream, result);
    return result;
}

IPC_Stream* IPC_Accept(IPC_Stream* serverStream, IPC_ErrorCode* error)
{
    IPC_Stream* accepted = IPC_Create(error);
    if (*error != IPC_Succeeded)
        return NULL;

    *error = PosixDomainSocketsIPC::Accept(*serverStream, *accepted);
    if (*error != IPC_Succeeded)
    {
        IPC_ErrorCode ec2;
        IPC_Destroy(accepted, &ec2);
        accepted = NULL;
    }

    return accepted;
}

IPC_Stream* IPC_AcceptSynchronous(IPC_Stream* serverStream, IPC_TimeVal timeout, IPC_ErrorCode* error)
{
    IPC_Stream* accepted = IPC_Create(error);
    if (*error != IPC_Succeeded)
        return NULL;

    *error = PosixDomainSocketsIPC::AcceptSync(*serverStream, timeout, *accepted);
    if (*error != IPC_Succeeded)
    {
        IPC_ErrorCode ec2;
        IPC_Destroy(accepted, &ec2);
        accepted = NULL;
    }

    return accepted;
}

size_t IPC_Read(IPC_Stream* stream, void* buf, size_t maxLen, IPC_ErrorCode* error)
{
    size_t bytesRead = 0;
    *error = PosixDomainSocketsIPC::Read(*stream, buf, maxLen, bytesRead);
    return bytesRead;
}

size_t IPC_ReadSynchronous(IPC_Stream* stream, void* buf, size_t maxLen, IPC_TimeVal timeout, IPC_ErrorCode* error)
{
    size_t bytesRead = 0;
    *error = PosixDomainSocketsIPC::ReadSync(*stream, buf, maxLen, timeout, bytesRead);
    return bytesRead;
}

size_t IPC_Write(IPC_Stream* stream, const void* buf, size_t len, IPC_ErrorCode* error)
{
    size_t bytesWritten = 0;
    *error = PosixDomainSocketsIPC::Write(*stream, buf, len, bytesWritten);
    return bytesWritten;
}

int IPC_GetConnectedPeerProcessId(IPC_Stream* pal_stream, IPC_ErrorCode* error)
{
    PosixDomainSocketsIPC::Stream& stream = *pal_stream;

    pid_t pid = 0;
    socklen_t so_pid = sizeof(int);
    if (getsockopt(stream.socket, 0, LOCAL_PEERPID, &pid, &so_pid) != 0 ||
        so_pid != sizeof(pid) || pid == 0)
    {
        switch (errno)
        {
        case ECONNRESET:
        case EPIPE:
        case EBADF:
            *error = IPC_NotConnected;
            return 0;
        }
        // error: getsockopt(LOCAL_PEERPID) failed
        *error = PosixDomainSocketsIPC::PosixErrorToPALError(errno);
        return 0;
    }

    *error = IPC_Succeeded;
    return pid;
}
