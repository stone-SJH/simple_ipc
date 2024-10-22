#include "../ipc.h"
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <errno.h>
#include <unistd.h>
#include <string>
#include <chrono>

namespace PosixDomainSocketsIPC
{
    struct Stream
    {
        int socket;
        int lockfile;
        std::string socketName;
        bool isListening;

        Stream() : socketName() {}
    };

    const char kSocketPrefix[] = "/tmp/Stone-";
    const char kSocketPostfix[] = ".sock";
    const char kLocklPostfix[] = ".lock";

    static IPC_ErrorCode PosixErrorToPALError(int err)
    {
        switch (err)
        {
        case ENOENT:
            return IPC_NameNotKnown;
        default:
        {
            return IPC_Failed;
        }
        }
    }

    void GetLockFileNameFromSocketName(const std::string& socketName, std::string& lockfilePath)
    {
        lockfilePath = socketName;
        lockfilePath.erase(lockfilePath.size() - strlen(kSocketPostfix));
        lockfilePath.append(kLocklPostfix);
    }

    void Create(Stream& stream)
    {
        stream.socket = -1;
        stream.lockfile = -1;
        stream.isListening = false;
    }

    IPC_ErrorCode Disconnect(Stream& stream)
    {
        if (stream.socket == -1)
            return IPC_NotConnected;

        if (stream.isListening)
        {
            int socketCleanupResult = unlink(stream.socketName.c_str());

            // From `man 2 unlink`:
            // If one or more process have the file open when the last link is removed, the link is removed, but the
            // removal of the file is delayed until all references to it have been closed.
            //
            // This should mean that calling unlink *before* close avoids a race condition with other threads/processes
            // opening the lockfile at the same time.
            std::string lockfileName;
            GetLockFileNameFromSocketName(stream.socketName, lockfileName);
            int lockCleanupResult = unlink(lockfileName.c_str());
            int closeResult = close(stream.lockfile);

            if (socketCleanupResult < 0)
                return PosixErrorToPALError(socketCleanupResult);
            if (lockCleanupResult < 0)
                return PosixErrorToPALError(lockCleanupResult);
            if (closeResult < 0)
                return PosixErrorToPALError(closeResult);
        }

        if (close(stream.socket) != 0)
            return PosixErrorToPALError(errno);

        stream.socket = -1;
        stream.socketName.clear();
        return IPC_Succeeded;
    }

    static IPC_ErrorCode SetSocketFlags(Stream& stream)
    {
        int flags = fcntl(stream.socket, F_GETFL, 0);
        if (flags < 0)
            return PosixErrorToPALError(errno);

        if ((flags & O_NONBLOCK) == 0)
        {
            if (fcntl(stream.socket, F_SETFL, flags | O_NONBLOCK) < 0)
                return PosixErrorToPALError(errno);
        }

        return IPC_Succeeded;
    }

    static IPC_ErrorCode InitializeSocketWithName(Stream& stream, std::string name, sockaddr_un& addr)
    {
        if (stream.socket != -1)
            return IPC_AlreadyConnected;

        if (name.size() > IPC_MaxStreamNameLength)
            return IPC_NameTooLong;

        stream.socketName = kSocketPrefix + name + kSocketPostfix;

        stream.socket = socket(PF_LOCAL, SOCK_STREAM, 0);
        if (stream.socket < 0)
        {
            return PosixErrorToPALError(errno);
        }

        IPC_ErrorCode errcode = SetSocketFlags(stream);
        if (errcode != IPC_Succeeded)
            return errcode;

        addr.sun_family = AF_LOCAL;
        strcpy(addr.sun_path, stream.socketName.c_str());
        return IPC_Succeeded;
    }

    IPC_ErrorCode GetSocketPath(Stream& stream, const char*& result)
    {
        if (stream.socket == -1)
        {
            result = NULL;
            return IPC_NameNotKnown;
        }

        result = stream.socketName.c_str();
        return IPC_Succeeded;
    }

    IPC_ErrorCode ConnectTo(Stream& stream, std::string name)
    {
        IPC_ErrorCode result;
        sockaddr_un addr;
        result = InitializeSocketWithName(stream, name, addr);
        if (result != IPC_Succeeded)
            return result;

        if (connect(stream.socket, reinterpret_cast<sockaddr*>(&addr), SUN_LEN(&addr)) != 0)
        {
            if (errno == ECONNREFUSED)
                result = IPC_ConnectionRefused;
            else
                result = PosixErrorToPALError(errno);

            // Clean up
            Disconnect(stream);

            return result;
        }

        return IPC_Succeeded;
    }

    IPC_ErrorCode Listen(Stream& stream, std::string name)
    {
        IPC_ErrorCode result;
        sockaddr_un addr;
        result = InitializeSocketWithName(stream, name, addr);
        if (result != IPC_Succeeded)
            return result;

        // Acquire the lockfile next to the socket first
        std::string lockfileName;
        GetLockFileNameFromSocketName(stream.socketName, lockfileName);
        stream.lockfile = open(lockfileName.c_str(), O_RDONLY | O_CREAT, 0600);
        if (stream.lockfile == -1)
            return IPC_NameInUse;

        int ret = flock(stream.lockfile, LOCK_EX | LOCK_NB);
        if (ret != 0)
            return IPC_NameInUse;

        // Delete the socket if it was left over from before
        if (access(stream.socketName.c_str(), F_OK) != -1)
            unlink(stream.socketName.c_str());

        if (bind(stream.socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
            return PosixErrorToPALError(errno);

        if (listen(stream.socket, 1) != 0)
        {
            return PosixErrorToPALError(errno);
        }

        stream.isListening = true;
        return IPC_Succeeded;
    }

    IPC_ErrorCode IsListening(Stream& stream, bool& result)
    {
        result = stream.socket != -1 && stream.isListening;
        return IPC_Succeeded;
    }

    IPC_ErrorCode IsConnected(Stream& stream, bool& result)
    {
        if (stream.socket == -1)
        {
            result = false;
            return IPC_Succeeded;
        }

        if (stream.isListening)
        {
            result = false;
            return IPC_AlreadyListening;
        }

        // Peek on the socket to make sure we are still connected
        char dummy;
        int bytesRead;
        if ((bytesRead = recvfrom(stream.socket, &dummy, 1, MSG_PEEK, NULL, NULL)) == -1)
        {
            switch (errno)
            {
                // We failed to peek - why?
            case EAGAIN:
            case EINTR:
                // No data to read right now - that's fine though, we are still connected
                result = true;
                return IPC_Succeeded;

            case ECONNRESET:
                // Connection is dead
                Disconnect(stream);
                result = false;
                return IPC_Succeeded;

            default:
                // Something else went wrong
                return PosixErrorToPALError(errno);
            }
        }

        // The man page says: For TCP sockets, the return value 0 means the peer has closed its half side of the connection.
        // It looks like this is true for AF_LOCAL sockets too, at least on Darwin.
        if (bytesRead == 0)
        {
            Disconnect(stream);
            result = false;
            return IPC_Succeeded;
        }

        // We successfully peeked, so the connection is still open (or at least, still has data to read)
        result = true;
        return IPC_Succeeded;
    }

    void Destroy(Stream& stream)
    {
        Disconnect(stream);
    }

    IPC_ErrorCode Accept(Stream& serverStream, Stream& newStream)
    {
        if (!serverStream.isListening)
            return IPC_NotListening;

        sockaddr_un remoteAddr;
        socklen_t remoteAddrSize = sizeof(remoteAddr);
        int newSocket = accept(serverStream.socket, reinterpret_cast<sockaddr*>(&remoteAddr), &remoteAddrSize);
        if (newSocket < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
                // Nobody connected right now
                return IPC_NotConnected;

            return PosixErrorToPALError(errno);
        }

        newStream.socket = newSocket;
        newStream.socketName = serverStream.socketName;
        newStream.isListening = false;

        // From `man 2 accept`:
        // On Linux, the new socket returned by accept() does not inherit file status flags such as O_NONBLOCK and
        // O_ASYNC from the listening socket. This behavior differs from the canonical BSD sockets implementation.
        // Portable programs should not rely on inheritance or noninheritance of file status flags and always explicitly
        // set all required flags on the socket returned from accept().
        IPC_ErrorCode result = SetSocketFlags(newStream);
        if (result != IPC_Succeeded)
            return result;

        return IPC_Succeeded;
    }

    IPC_ErrorCode AcceptSync(Stream& stream, float timeout, Stream& newStream)
    {
        if (!stream.isListening)
            return IPC_NotListening;

        fd_set acceptset;
        FD_ZERO(&acceptset);
        FD_SET(stream.socket, &acceptset);

        timeval tv;
        tv.tv_sec = (int)timeout;
        tv.tv_usec = (timeout - tv.tv_sec) * 1000000;

    retry:
        int result = select(stream.socket + 1, &acceptset, NULL, NULL, &tv);
        if (result < 0)
        {
            if (errno == EINTR)
                goto retry;

            return PosixErrorToPALError(errno);
        }

        if (result == 0)
            return IPC_TimedOut;

        return Accept(stream, newStream);
    }

    IPC_ErrorCode Read(Stream& stream, void* buf, size_t maxLen, size_t& bytesRead)
    {
        bytesRead = 0;

        if (stream.socket == -1)
            return IPC_NotConnected;
        if (stream.isListening)
            return IPC_AlreadyListening;

        int result = recvfrom(stream.socket, buf, maxLen, 0, NULL, NULL);
        if (result < 0)
        {
            switch (errno)
            {
            case EWOULDBLOCK:
            case EINTR:
                // No data available to read right now
                return IPC_Succeeded;

            case ECONNRESET:
                // The peer disconnected, so disconnect us too
                Disconnect(stream);
                return IPC_NotConnected;

            default:
                return PosixErrorToPALError(errno);
            }
        }

        // As noted above, if recvfrom returned 0 bytes (instead of -1 and EWOULDBLOCK) then it means the connection closed
        if (result == 0)
        {
            Disconnect(stream);
            return IPC_NotConnected;
        }

        bytesRead = result;
        return IPC_Succeeded;
    }

    IPC_ErrorCode ReadSync(Stream& stream, void* buf, size_t len, float timeout, size_t& bytesRead)
    {
        bytesRead = 0;
        auto start = std::chrono::high_resolution_clock::now();

        if (stream.socket == -1)
            return IPC_NotConnected;
        if (stream.isListening)
            return IPC_AlreadyListening;

        fd_set readset;
        timeval tv;

        while (len > 0)
        {
            FD_ZERO(&readset);
            FD_SET(stream.socket, &readset);

            tv.tv_sec = (int)timeout;
            tv.tv_usec = (timeout - tv.tv_sec) * 1000000;

            float timeSpent = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count();

            int result = select(stream.socket + 1, &readset, NULL, NULL, &tv);

            timeout -= timeSpent;
            if (timeout < 0)
                timeout = 0;

            if (result < 0)
            {
                if (errno == EINTR)
                    continue;

                return PosixErrorToPALError(errno);
            }
            if (result == 0)
                return IPC_TimedOut;

            size_t bytesThisRead = 0;
            IPC_ErrorCode readResult = Read(stream, buf, len, bytesThisRead);
            if (readResult != IPC_Succeeded)
                return readResult;

            bytesRead += bytesThisRead;
            buf = static_cast<char*>(buf) + bytesThisRead;
            len -= bytesThisRead;
        }

        return IPC_Succeeded;
    }

    IPC_ErrorCode Write(Stream& stream, const void* buf, size_t len, size_t& bytesWritten)
    {
        bytesWritten = 0;

        if (stream.isListening)
            return IPC_AlreadyListening;

        int result = send(stream.socket, buf, len, 0);
        if (result < 0)
        {
            switch (errno)
            {
            case EAGAIN:
            case EINTR:
                return IPC_Succeeded;

            case ECONNRESET:
            case EPIPE:
            case EBADF:
                // The peer disconnected, so disconnect us too
                Disconnect(stream);
                return IPC_NotConnected;

            default:
                return PosixErrorToPALError(errno);
            }

            return PosixErrorToPALError(errno);
        }

        bytesWritten = result;
        return IPC_Succeeded;
    }
}
