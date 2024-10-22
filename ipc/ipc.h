#pragma once
#include <stdint.h>

#if IPC_OSX
#include <inttypes.h>
#include <cstddef>
#endif

struct IPC_Stream;

typedef uint32_t IPC_ErrorCode;

typedef int IPC_TimeVal;

static const size_t IPC_MaxStreamNameLength = 50;

static const IPC_ErrorCode IPC_Succeeded = 0;
static const IPC_ErrorCode IPC_Failed = 0x80000000;
static const IPC_ErrorCode IPC_NameTooLong = 0x80000001;
static const IPC_ErrorCode IPC_NameNotKnown = 0x80000002;
static const IPC_ErrorCode IPC_NameInUse = 0x80000003;
static const IPC_ErrorCode IPC_NotConnected = 0x80000004;
static const IPC_ErrorCode IPC_AlreadyConnected = 0x80000005;
static const IPC_ErrorCode IPC_NotListening = 0x80000006;
static const IPC_ErrorCode IPC_AlreadyListening = 0x80000007;
static const IPC_ErrorCode IPC_TimedOut = 0x80000008;
static const IPC_ErrorCode IPC_Busy = 0x80000009;
static const IPC_ErrorCode IPC_ConnectionRefused = 0x8000000A;

#ifdef __cplusplus
extern "C" {
#endif

	IPC_Stream* IPC_Create(IPC_ErrorCode* error);
	void IPC_Destroy(IPC_Stream* stream, IPC_ErrorCode* error);

	void IPC_ConnectTo(IPC_Stream* stream, const char* name, IPC_ErrorCode* error);
	bool IPC_IsConnected(IPC_Stream* stream, IPC_ErrorCode* error);
	void IPC_Disconnect(IPC_Stream* stream, IPC_ErrorCode* error);

	void IPC_Listen(IPC_Stream* stream, const char* name, IPC_ErrorCode* error);
	bool IPC_IsListening(IPC_Stream* stream, IPC_ErrorCode* error);

	const char* IPC_GetPlatformSpecificPath(IPC_Stream* stream, IPC_ErrorCode* error);

	IPC_Stream* IPC_Accept(IPC_Stream* serverStream, IPC_ErrorCode* error);
	IPC_Stream* IPC_AcceptSynchronous(IPC_Stream* serverStream, IPC_TimeVal timeout, IPC_ErrorCode* error);

	size_t IPC_Read(IPC_Stream* stream, void* buf, size_t maxLen, IPC_ErrorCode* error);
	size_t IPC_ReadSynchronous(IPC_Stream* stream, void* buf, size_t maxLen, IPC_TimeVal timeout, IPC_ErrorCode* error);

	size_t IPC_Write(IPC_Stream* stream, const void* buf, size_t len, IPC_ErrorCode* error);

	int IPC_GetConnectedPeerProcessId(IPC_Stream* stream, IPC_ErrorCode* error);

#ifdef __cplusplus
}
#endif
