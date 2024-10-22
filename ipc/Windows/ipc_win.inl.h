#include <windows.h>
#include <winbase.h>
#include <string>
#include <stringapiset.h>
#include <chrono>
#include <handleapi.h>

namespace Win32NamedPipesIPC
{
    inline std::wstring convert(const std::string& as)
    {
        if (as.empty()) return std::wstring();
        size_t reqLength = ::MultiByteToWideChar(CP_UTF8, 0, as.c_str(), (int)as.length(), 0, 0);
        std::wstring ret(reqLength, L'\0');
        ::MultiByteToWideChar(CP_UTF8, 0, as.c_str(), (int)as.length(), &ret[0], (int)ret.length());
        return ret;
    }

    struct Stream
    {
        HANDLE hPipe;
        OVERLAPPED oOverlap;
        std::string pipeName;
        bool isListening;

        Stream() : pipeName() {}
    };

    const char kWindowsPipeNamePrefixArray[] = "\\\\.\\pipe\\Stone-";
    const std::string kWindowsPipeNamePrefix(kWindowsPipeNamePrefixArray);
    const int kMaximumWindowsPipeNameLength = 256;
    const int kDefaultPipeInputBufferSize = 65536;
    const int kDefaultPipeOutputBufferSize = 65536;

    static IPC_ErrorCode Win32ErrorToPALError(DWORD win32Error)
    {
        switch (win32Error)
        {
        case ERROR_SUCCESS:
            return IPC_Succeeded;
        case ERROR_INVALID_HANDLE:
        case ERROR_BROKEN_PIPE:
            return IPC_NotConnected;
        case ERROR_FILE_NOT_FOUND:
            return IPC_NameNotKnown;
        case ERROR_PIPE_BUSY:
            return IPC_Busy;
        default:
            return IPC_Failed;
        }
    }

    static IPC_ErrorCode InitializePipeWithName(Stream& stream, std::string name)
    {
        if (stream.hPipe != INVALID_HANDLE_VALUE)
            return IPC_AlreadyConnected;

        if (name.size() > IPC_MaxStreamNameLength)
            return IPC_NameTooLong;

        stream.pipeName = kWindowsPipeNamePrefix + name;
        return IPC_Succeeded;
    }

    static IPC_ErrorCode OpenServerPipeAlreadyInitialized(Stream& stream, bool isFirstInstance)
    {
        stream.isListening = true;

        DWORD accessFlags = PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED;
        if (isFirstInstance)
            accessFlags |= FILE_FLAG_FIRST_PIPE_INSTANCE;

        std::wstring widePipeName = convert(stream.pipeName);

        stream.hPipe = CreateNamedPipeW(widePipeName.c_str(),
            accessFlags,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
            PIPE_UNLIMITED_INSTANCES,
            kDefaultPipeOutputBufferSize,
            kDefaultPipeInputBufferSize,
            0, NULL
        );

        if (stream.hPipe == INVALID_HANDLE_VALUE)
        {
            DWORD error = GetLastError();
            if (error == ERROR_ACCESS_DENIED)
            {
                // Something is already listening on this endpoint
                return IPC_NameInUse;
            }

            return Win32ErrorToPALError(error);
        }

        stream.oOverlap.hEvent = CreateEvent(
            NULL,    // default security attribute
            TRUE,    // manual-reset event
            TRUE,    // initial state = signaled
            NULL);   // unnamed event object

        if (stream.oOverlap.hEvent == NULL)
        {
            CloseHandle(stream.hPipe);
            stream.hPipe = INVALID_HANDLE_VALUE;
            return IPC_Failed;
        }

        return IPC_Succeeded;
    }

    void Create(Stream& stream)
    {
        stream.hPipe = INVALID_HANDLE_VALUE;
        memset(&stream.oOverlap, 0, sizeof(OVERLAPPED));
        stream.isListening = false;
        stream.pipeName.clear();
    }

    IPC_ErrorCode Disconnect(Stream& stream)
    {
        if (stream.hPipe == INVALID_HANDLE_VALUE)
            return IPC_NotConnected;

        CloseHandle(stream.oOverlap.hEvent);

        if (!CloseHandle(stream.hPipe))
            return Win32ErrorToPALError(GetLastError());

        // Reset the stream
        Create(stream);

        return IPC_Succeeded;
    }

    IPC_ErrorCode ConnectTo(Stream& stream, std::string name)
    {
        IPC_ErrorCode result = InitializePipeWithName(stream, name);
        if (result != IPC_Succeeded)
            return result;

        stream.isListening = false;

        std::wstring widePipeName = convert(stream.pipeName);

        stream.hPipe = CreateFileW(widePipeName.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
            NULL);

        if (stream.hPipe == INVALID_HANDLE_VALUE)
        {
            DWORD error = GetLastError();
            if (error == ERROR_ACCESS_DENIED)
            {
                return IPC_ConnectionRefused;
            }
            return Win32ErrorToPALError(error);
        }

        stream.oOverlap.hEvent = CreateEvent(
            NULL,    // default security attribute
            TRUE,    // manual-reset event
            TRUE,    // initial state = signaled
            NULL);   // unnamed event object

        if (stream.oOverlap.hEvent == NULL)
        {
            CloseHandle(stream.hPipe);
            stream.hPipe = INVALID_HANDLE_VALUE;
            return IPC_Failed;
        }

        return IPC_Succeeded;
    }

    IPC_ErrorCode Listen(Stream& stream, std::string name)
    {
        IPC_ErrorCode result;
        if ((result = InitializePipeWithName(stream, name)) != IPC_Succeeded)
            return result;

        return OpenServerPipeAlreadyInitialized(stream, true);
    }

    void Destroy(Stream& stream)
    {
        Disconnect(stream);
    }

    IPC_ErrorCode GetPlatformSpecificPath(Stream& stream, const char*& result)
    {
        if (stream.hPipe == INVALID_HANDLE_VALUE)
        {
            result = NULL;
            return IPC_NameNotKnown;
        }

        result = stream.pipeName.c_str();
        return IPC_Succeeded;
    }

    // CancelIOEx can either cancel the IO operation or the operation can finish if the
    // cancellation was not called in time. Wait for the completion to know which one happened.
    // Returns true if cancelled, false if IO completed or cancellation failed.
    bool CancelIOAndWaitCompletion(Stream& stream, DWORD& finishedBytes, IPC_ErrorCode& ipcError)
    {
        finishedBytes = 0;
        ipcError = IPC_Succeeded;

        CancelIoEx(stream.hPipe, &stream.oOverlap);

        if (!GetOverlappedResult(stream.hPipe, &stream.oOverlap, &finishedBytes, TRUE))
        {
            DWORD error = GetLastError();

            // Cancelled successfully is the only error code that we expect here and
            // that will return success with zero bytes written
            if (error != ERROR_OPERATION_ABORTED)
            {
                ipcError = Win32ErrorToPALError(error);
                return false;
            }

            return true;
        }

        return false;
    }

    // Waits for currently pending IO operation until successfully completed or timeout.
    // In the case of timeout the IO operation will be cancelled, which might actually turn out as a completed operation too.
    IPC_ErrorCode WaitForPendingIO(Stream& stream, int timeout, DWORD& finishedBytes)
    {
        finishedBytes = 0;

        DWORD waitResult = WaitForSingleObject(stream.oOverlap.hEvent, (DWORD)(timeout));

        switch (waitResult)
        {
        case WAIT_OBJECT_0:
        {
            // Object was signaled so we can use blocking result check
            if (!GetOverlappedResult(stream.hPipe, &stream.oOverlap, &finishedBytes, TRUE))
            {
                DWORD error = GetLastError();
                return Win32ErrorToPALError(error);
            }

            return IPC_Succeeded;
        }

        case WAIT_TIMEOUT:
        {
            IPC_ErrorCode ipcError = IPC_Succeeded;

            // We return timeout only if the IO was cancelled successfully
            if (CancelIOAndWaitCompletion(stream, finishedBytes, ipcError))
            {
                return IPC_TimedOut;
            }

            // Will return success if the IO was actually completed
            return ipcError;
        }

        default:
            return IPC_Failed;
        }
    }

    IPC_ErrorCode Accept(Stream& stream, Stream** newConnection, bool synchronous = false, int timeout = 0)
    {
        //Assert(newConnection && !*newConnection);

        if (!stream.isListening)
            return IPC_NotListening;

        DWORD error = 0;

        // Pipe was created with PIPE_WAIT in overlapped mode. Connect will either return non-zero for
        // immediate success or zero with more info using GetLastError()
        if (ConnectNamedPipe(stream.hPipe, &stream.oOverlap) == 0)
        {
            error = GetLastError();

            // Three error codes are acceptable:
            // ERROR_PIPE_CONNECTED means there's an open connection that we can offload into newConnection
            // ERROR_IO_PENDING means the connection is pending
            // ERROR_NO_DATA means the client connected but disconnected before we could accept it - to match
            // the POSIX semantics, we will still treat this as a successful connection, but newConnection will
            // not be connected to anything.
            if (!(error == ERROR_PIPE_CONNECTED || error == ERROR_NO_DATA || error == ERROR_IO_PENDING))
                return Win32ErrorToPALError(error);
        }

        if (error == ERROR_IO_PENDING)
        {
            DWORD unusedArg = 0; // unused var needed for IO func call below

            IPC_ErrorCode ipcError = WaitForPendingIO(stream, timeout, unusedArg);

            if (!synchronous && ipcError == IPC_TimedOut)
                return IPC_NotConnected;
            else if (ipcError != IPC_Succeeded)
                return ipcError;
        }

        // Reaching here means that a connection was made successfully

        *newConnection = new Stream();
        Stream& newConn = **newConnection;

        newConn.hPipe = stream.hPipe;
        newConn.oOverlap = stream.oOverlap;
        newConn.isListening = false;
        newConn.pipeName = stream.pipeName;

        if (error == ERROR_NO_DATA)
        {
            // Clean up the broken pipe
            Disconnect(newConn);
        }

        // Open a new instance of the pipe for another client to connect to
        stream.hPipe = INVALID_HANDLE_VALUE;
        memset(&stream.oOverlap, 0, sizeof(OVERLAPPED));
        stream.oOverlap.hEvent = INVALID_HANDLE_VALUE;
        return OpenServerPipeAlreadyInitialized(stream, false);
    }

    IPC_ErrorCode AcceptSynchronous(Stream& stream, Stream** newConnection, float timeout)
    {
        return Accept(stream, newConnection, true, timeout);
    }

    bool IsListening(Stream& stream)
    {
        return stream.hPipe != INVALID_HANDLE_VALUE && stream.isListening;
    }

    IPC_ErrorCode IsConnected(Stream& stream, bool& result)
    {
        if (stream.isListening)
        {
            result = false;
            return IPC_AlreadyListening;
        }

        if (stream.hPipe == INVALID_HANDLE_VALUE)
        {
            result = false;
            return IPC_Succeeded;
        }

        // Check with Windows that the pipe is still connected
        if (PeekNamedPipe(stream.hPipe, NULL, 0, NULL, NULL, NULL))
        {
            result = true;
            return IPC_Succeeded;
        }

        result = false;
        DWORD error = GetLastError();
        if (error == ERROR_BROKEN_PIPE)
        {
            Disconnect(stream);
            return IPC_Succeeded;
        }
        return Win32ErrorToPALError(error);
    }

    IPC_ErrorCode Read(Stream& stream, void* buf, size_t maxLen, DWORD& bytesRead, bool synchronous = false, int timeout = 0)
    {
        if (stream.isListening)
            return IPC_AlreadyListening;

        bytesRead = 0;

        // Initiate async read op. This could either return true if the read succeeded
        // immediately, or false with further info in the error code
        if (!ReadFile(stream.hPipe, buf, maxLen, &bytesRead, &stream.oOverlap))
        {
            DWORD error = GetLastError();
            switch (error)
            {
            case ERROR_IO_PENDING: // Async read is pending
                break;
            case ERROR_BROKEN_PIPE:
                Disconnect(stream);
                return IPC_NotConnected;
            default:
                return Win32ErrorToPALError(error);
            }

            IPC_ErrorCode ipcError = WaitForPendingIO(stream, timeout, bytesRead);

            // Async read must succeed with zero bytes instead of timeout error code
            if (!synchronous && ipcError == IPC_TimedOut)
                ipcError = IPC_Succeeded;

            if (ipcError != IPC_Succeeded)
                return ipcError;
        }

        //Assert(bytesRead <= maxLen);

        return IPC_Succeeded;
    }

    IPC_ErrorCode Write(Stream& stream, const void* buf, size_t len, DWORD& bytesWritten)
    {
        if (stream.isListening)
            return IPC_AlreadyListening;

        bytesWritten = 0;

        // Initiate async write op. This could either return true if the write succeeded
        // immediately, or false with further info in the error code
        if (!WriteFile(stream.hPipe, buf, len, &bytesWritten, &stream.oOverlap))
        {
            DWORD error = GetLastError();
            switch (error)
            {
            case ERROR_IO_PENDING: // Async write is pending
                break;
            case ERROR_NO_DATA:
                bytesWritten = 0;
                Disconnect(stream);
                return IPC_NotConnected;
            default:
                return Win32ErrorToPALError(error);
            }

            // There seems to be no way to figure out if a write is blocking due to full buffer
            // vs just taking the time the write needs to complete asynchronously. Therefore we use
            // wait with a relatively large timeout compared to expected write operation duration.
            // This way we can make this func non-blocking on unexpectedly long operations.
            IPC_ErrorCode ipcError = WaitForPendingIO(stream, 60000.0f, bytesWritten);

            // Canceling write operations is not safe in the sense that there is no guarantee that
            // some bytes have not already been written to the pipe at the time of cancel. Therefore
            // we cannot return success with 0 bytes written and retry again, but instead we need to
            // actually fail the operation with timeout error when that happens.
            return ipcError;
        }

        //Assert(bytesWritten <= len);

        return IPC_Succeeded;
    }

    IPC_ErrorCode ReadSynchronous(Stream& stream, void* buf, size_t maxLen, DWORD& bytesRead, int timeout)
    {
        bytesRead = 0;

        auto start = std::chrono::high_resolution_clock::now();

        // Do blocking reads in a loop until either the requested data amount is read, timeout is hit or an error occurs
        do
        {
            DWORD bytesThisRead = 0;
            int timeSpent = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count();
            int timeLeft = timeout - timeSpent;
            IPC_ErrorCode error = Read(stream, buf, maxLen, bytesThisRead, true, timeLeft > 0.0 ? timeLeft : 0.0);
            if (error != IPC_Succeeded)
                return error;

            bytesRead += bytesThisRead;
            buf = (uint8_t*)buf + bytesThisRead;
            maxLen -= bytesThisRead;
        } while (maxLen != 0);

        return IPC_Succeeded;
    }

    IPC_ErrorCode GetServerProcessId(Stream& stream, int& peerProcessId)
    {
        unsigned long pid = 0;
        if (!GetNamedPipeServerProcessId(stream.hPipe, &pid))
        {
            return Win32ErrorToPALError(GetLastError());
        }
        if (pid == GetCurrentProcessId())
        {
            if (!GetNamedPipeClientProcessId(stream.hPipe, &pid))
            {
                return Win32ErrorToPALError(GetLastError());
            }
        }

        peerProcessId = pid;
        return IPC_Succeeded;
    }
}
