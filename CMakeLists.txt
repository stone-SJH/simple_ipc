cmake_minimum_required (VERSION 3.8)
set(CMAKE_CXX_STANDARD 14)

# Add source to this project's executable.
if (IPC_WIN32)
	PROJECT(ext_worker LANGUAGES CXX)
	add_definitions(-DIPC_WIN32)
	set(IPC_SRCS "${CMAKE_SOURCE_DIR}/ipc/ipc.h" "${CMAKE_SOURCE_DIR}/ipc/Windows/ipc_win.inl.h" "${CMAKE_SOURCE_DIR}/ipc/Windows/ipc_win.cpp")
	set(IPCUtility_SRCS "${CMAKE_SOURCE_DIR}/tls/IPCUtility.h" "${CMAKE_SOURCE_DIR}/tls/IPCUtility.cpp")
	set(TLS_SRCS "${CMAKE_SOURCE_DIR}/tls/tls.h" "${CMAKE_SOURCE_DIR}/tls/Windows/tls.cpp" "${CMAKE_SOURCE_DIR}/tls/ThreadSpecificValue.h" "${CMAKE_SOURCE_DIR}/tls/ThreadSpecificValue.cpp"
	"${CMAKE_SOURCE_DIR}/tls/NonCopyable.h" "${CMAKE_SOURCE_DIR}/tls/Mutex.h" "${CMAKE_SOURCE_DIR}/tls/Mutex.cpp" "${CMAKE_SOURCE_DIR}/tls/Windows/PlatformMutex.h" "${CMAKE_SOURCE_DIR}/tls/Windows/PlatformMutex.cpp"
	"${CMAKE_SOURCE_DIR}/tls/Process.h" "${CMAKE_SOURCE_DIR}/tls/Windows/Process.cpp")
elseif(IPC_OSX)
	PROJECT(ext_worker LANGUAGES CXX OBJCXX)
	add_definitions(-DIPC_OSX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=10.9")
	set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")
	set(IPC_SRCS "${CMAKE_SOURCE_DIR}/ipc/ipc.h" "${CMAKE_SOURCE_DIR}/ipc/OSX/ipc_osx.inl.h" "${CMAKE_SOURCE_DIR}/ipc/OSX/ipc_osx.cpp")
	set(IPCUtility_SRCS "${CMAKE_SOURCE_DIR}/tls/IPCUtility.h" "${CMAKE_SOURCE_DIR}/tls/IPCUtility.cpp")
	set(TLS_SRCS "${CMAKE_SOURCE_DIR}/tls/tls.h" "${CMAKE_SOURCE_DIR}/tls/OSX/tls.cpp" "${CMAKE_SOURCE_DIR}/tls/ThreadSpecificValue.h" "${CMAKE_SOURCE_DIR}/tls/ThreadSpecificValue.cpp"
			"${CMAKE_SOURCE_DIR}/tls/NonCopyable.h" "${CMAKE_SOURCE_DIR}/tls/Mutex.h" "${CMAKE_SOURCE_DIR}/tls/Mutex.cpp" "${CMAKE_SOURCE_DIR}/tls/OSX/PlatformMutex.h"
			"${CMAKE_SOURCE_DIR}/tls/OSX/PlatformMutex.cpp" "${CMAKE_SOURCE_DIR}/tls/Process.h" "${CMAKE_SOURCE_DIR}/tls/OSX/Process.mm")
endif()
add_library (ext_worker SHARED ${IPC_SRCS} ${IPCUtility_SRCS} ${TLS_SRCS} "${CMAKE_SOURCE_DIR}/export.h" "${CMAKE_SOURCE_DIR}/export.cpp")
add_executable (ipc-client  ${IPC_SRCS} ${IPCUtility_SRCS} ${TLS_SRCS} "${CMAKE_SOURCE_DIR}/client/client.cpp")

if (IPC_WIN32)

elseif (IPC_OSX)
	set_property(TARGET ext_worker APPEND_STRING PROPERTY COMPILE_FLAGS "-fno-objc-arc")
	target_link_libraries(ext_worker PRIVATE "-framework Foundation")
	set_property(TARGET ipc-client APPEND_STRING PROPERTY COMPILE_FLAGS "-fno-objc-arc")
	target_link_libraries(ipc-client PRIVATE "-framework Foundation")
endif()