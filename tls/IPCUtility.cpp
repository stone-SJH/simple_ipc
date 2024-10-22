#include "IPCUtility.h"
#include <chrono>

bool Write(IPC_Stream* ipcStream, const void* buffer, size_t numBytes, IPCData& data, uint32_t writeTimeoutMS){
    data.m_TimeOut = false;
    IPC_ErrorCode error = IPC_Succeeded;
    const bool useTimeout = (writeTimeoutMS > 0);
    auto start = std::chrono::high_resolution_clock::now();
    size_t totalBytesWritten = 0, totalBytesRemaining = numBytes;
    const char* p = (const char*)buffer;
    while (totalBytesWritten < numBytes)
    {
        int timeSpent = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count();
        int timeLeft = writeTimeoutMS - timeSpent;
        if (useTimeout && timeLeft < 0)
        {
            error = IPC_TimedOut;
            data.m_TimeOut = true;
            break;
        }
        size_t bytesToWrite = (totalBytesRemaining < kMaxWriteSize) ? totalBytesRemaining : kMaxWriteSize;
        size_t bytesWritten = IPC_Write(ipcStream, p, bytesToWrite, &error);
        if (error != IPC_Succeeded)
            break;
        if (bytesWritten == 0)
        {
            // pipe is full
            continue;
        }
        p += bytesWritten;
        totalBytesWritten += bytesWritten;
        totalBytesRemaining -= bytesWritten;
    }

    bool succeeded = (error == IPC_Succeeded) && (totalBytesWritten == numBytes);
    if (!succeeded)
    {
        data.m_ErrorID = error;
        data.m_BytesCompleted = totalBytesWritten;
        data.m_BytesTotal = numBytes;
    }

    return succeeded;
}

bool Read(IPC_Stream* ipcStream, void* buffer, size_t numBytes, IPCData& data, uint32_t readTimeoutMS) {
    data.m_TimeOut = false;
    IPC_ErrorCode error = IPC_Succeeded;
    size_t bytesRead = 0;

    if (readTimeoutMS > 0)
    {
        bytesRead = IPC_ReadSynchronous(ipcStream, buffer, numBytes, readTimeoutMS, &error);
    }
    else
    {
        // For infinite waiting read we use sync read in a loop with a relatively large timeout.
        // However, we need to handle the case where this arbitrary timeout could happen in the middle of a read.
        char* p = (char*)buffer;
        size_t bytesToRead = numBytes;
        do
        {
            size_t justRead = IPC_ReadSynchronous(ipcStream, buffer, bytesToRead, 3600.f, &error);
            bytesRead += justRead;
            p += justRead;
            bytesToRead -= justRead;
        } while (bytesRead < numBytes && (error == IPC_Succeeded || error == IPC_TimedOut));
    }

    data.m_TimeOut = (error == IPC_TimedOut);

    bool succeeded = (error == IPC_Succeeded) && (bytesRead == numBytes);
    if (!succeeded)
    {
        data.m_ErrorID = error;
        data.m_BytesCompleted = bytesRead;
        data.m_BytesTotal = numBytes;
    }
    return succeeded;
}

void WriteData(IPC_Stream* ipcStream, const void* buffer, size_t numBytes, IPCData& data) {
    if (!Write(ipcStream, &numBytes, sizeof(numBytes), data))
        throw IPCException("Failed to send packet size", data);
    if (numBytes > 0 && !Write(ipcStream, buffer, numBytes, data))
        throw IPCException("Failed to send packet", data);
}

void WriteLine(IPC_Stream* ipcStream, const std::string& msg) {
    IPCData ipcData;
    WriteData(ipcStream, msg.c_str(), msg.length(), ipcData);
}

void ReadLine(IPC_Stream* ipcStream, std::string& outmsg) {
    IPCData ipcData;
    size_t len = 0;
    if (!Read(ipcStream, &len, sizeof(len), ipcData))
        throw IPCException("Failed to receive packet size", ipcData);
    outmsg.resize(len);
    if (len > 0 && !Read(ipcStream, &outmsg[0], len, ipcData))
        throw IPCException("Failed to receive packet", ipcData);
}

void WriteBuffer(IPC_Stream* ipcStream, const void* data, size_t length) {
    IPCData ipcData;
    WriteData(ipcStream, data, length, ipcData);
}

void ReadBuffer(IPC_Stream* ipcStream, std::vector<uint8_t>& buffer) {
    IPCData ipcData;
    size_t len = 0;
    if (!Read(ipcStream, &len, sizeof(len), ipcData))
        throw IPCException("Failed to receive packet size", ipcData);
    buffer.resize(len, 0);
    if (len > 0 && !Read(ipcStream, buffer.data(), len, ipcData))
        throw IPCException("Failed to receive packet", ipcData);
}

void WriteString(IPC_Stream* ipcStream, const std::string& s) {
    WriteLine(ipcStream, s);
}
void WriteUInt32(IPC_Stream* ipcStream, uint32_t i) {
    IPCData ipcData;
    if (!Write(ipcStream, &i, sizeof(i), ipcData)) 
        throw IPCException("Failed to write UInt32", ipcData);
}
void WriteInt(IPC_Stream* ipcStream, int i) {
    WriteUInt32(ipcStream, *reinterpret_cast<uint32_t*>(&i));
}
void WriteBool(IPC_Stream* ipcStream, bool b) {
    WriteUInt32(ipcStream, b ? 1 : 0);
}

std::string ReadString(IPC_Stream* ipcStream) {
    std::string s;
    ReadLine(ipcStream, s);
    return s;
}

uint32_t ReadUInt32(IPC_Stream* ipcStream) {
    IPCData ipcData;
    uint32_t i;
    if (!Read(ipcStream, &i, sizeof(i), ipcData)) 
        throw IPCException("Failed to read UInt32", ipcData);
    return i;
}

int ReadInt(IPC_Stream* ipcStream) {
    uint32_t i = ReadUInt32(ipcStream);
    return *reinterpret_cast<int*>(&i);
}

bool ReadBool(IPC_Stream* ipcStream) {
    uint32_t i = ReadUInt32(ipcStream);
    return i == 1;
}

bool SafeRead(IPC_Stream* ipcStream, std::string& msg) {
    msg.clear();
    ReadLine(ipcStream, msg);
    if (msg.empty()) {
        IPC_ErrorCode error;;
        if (ipcStream && IPC_IsConnected(ipcStream, &error))
            return false;
        return true;
    }
    return true;
}

bool SafeRead(IPC_Stream* ipcStream, char** data) {
    bool ok;
    std::string msg;
    try {
        ok = SafeRead(ipcStream, msg);
        *data = new char[msg.size()];
        strcpy(*data, msg.c_str());
    }
    catch (IPCException& ex) {
        return false;
    }
    return ok;
}