#pragma once
#include "../tls.h"
#include <processthreadsapi.h>
#include <errhandlingapi.h>

TLS_HANDLE TLS_ALLOC() {
	DWORD r = TlsAlloc();
	if (r == TLS_OUT_OF_INDEXES)
		return TLS_InvalidHandle;
	return static_cast<TLS_HANDLE>(r);
}

int TLS_FREE(TLS_HANDLE handle) {

	BOOL success = TlsFree(static_cast<DWORD>(handle));
	return success;
}

int TLS_SET(TLS_HANDLE handle, uintptr_t value) {
	BOOL success = TlsSetValue((DWORD)handle, (void*)value);
	return success;
}

uintptr_t TLS_GET(TLS_HANDLE handle) {
	void* result = TlsGetValue((DWORD)handle);
	if (!((result != NULL) || (GetLastError() == 0L)))
		__debugbreak(); //>
	return (uintptr_t)result;
}