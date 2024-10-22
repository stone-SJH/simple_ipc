cmake_minimum_required(VERSION 3.14)
project(YahahaTextureCompression01)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (WIN32)
	add_definitions(-DIPC_WIN32)
	set(IPC_SRCS "../ipc/ipc.h" "../ipc/Windows/ipc_win.inl.h" "../ipc/Windows/ipc_win.cpp")
	set(IPCUtility_SRCS "../tls/IPCUtility.h" "../tls/IPCUtility.cpp")
elseif(APPLE)
	add_definitions(-DIPC_OSX)
	set(IPC_SRCS "../ipc/ipc.h" "../ipc/OSX/ipc_osx.inl.h" "../ipc/OSX/ipc_osx.cpp")
	set(IPCUtility_SRCS "../tls/IPCUtility.h" "../tls/IPCUtility.cpp")
endif()

add_executable(YaTCompress ${IPC_SRCS} ${IPCUtility_SRCS} main.cpp)

set_target_properties(YaTCompress PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin)

target_include_directories(YaTCompress PRIVATE ${CMAKE_SOURCE_DIR}/external/ISPC/builds)
# target_include_directories(YaTCompress PRIVATE ${CMAKE_SOURCE_DIR}/external/astcenc/builds)
target_include_directories(YaTCompress PRIVATE ${CMAKE_SOURCE_DIR}/external/astcenc/Source)
target_include_directories(YaTCompress PRIVATE ${CMAKE_SOURCE_DIR}/external/stb)
if(WIN32)
    target_link_directories(YaTCompress PRIVATE ${CMAKE_SOURCE_DIR}/external/ISPC/builds/windows)
    target_link_directories(YaTCompress PRIVATE ${CMAKE_SOURCE_DIR}/external/astcenc/builds/windows)
    target_link_libraries(YaTCompress ispc_texcomp astc_arm)

    # 拷贝DLL到输出目录
    add_custom_command(TARGET YaTCompress POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/external/ISPC/builds/windows/ispc_texcomp.dll"
        "${CMAKE_SOURCE_DIR}/bin")
    add_custom_command(TARGET YaTCompress POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/external/astcenc/builds/windows/astc_arm.dll"
        "${CMAKE_SOURCE_DIR}/bin")
elseif(APPLE)
    target_link_directories(YaTCompress PRIVATE ${CMAKE_SOURCE_DIR}/external/ISPC/builds/macos)
    target_link_directories(YaTCompress PRIVATE ${CMAKE_SOURCE_DIR}/external/astcenc/builds/macos)

    # 设置 RPATH
    # set_target_properties(YaTCompress PROPERTIES
    #     BUILD_WITH_INSTALL_RPATH TRUE
    #     INSTALL_RPATH "@executable_path/."
    #)

    target_link_libraries(YaTCompress ispc_texcomp astc_arm)

    add_custom_command(TARGET YaTCompress POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/external/ISPC/builds/macos/libispc_texcomp.dylib"
        "${CMAKE_SOURCE_DIR}/bin")
    add_custom_command(TARGET YaTCompress POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/external/astcenc/builds/macos/libastc_arm.dylib"
        "${CMAKE_SOURCE_DIR}/bin")

    # add_custom_command(TARGET YaTCompress POST_BUILD
    #     COMMAND install_name_tool -change @executable_path/../Frameworks/libispc_texcomp.dylib @executable_path/libispc_texcomp.dylib $<TARGET_FILE:YaTCompress>
    # )
endif()

# stb库剥离
# target_link_directories(YaTCompress PRIVATE ${CMAKE_SOURCE_DIR}/external/stb/windows)
# target_link_libraries(YaTCompress stb_lib)
# add_custom_command(TARGET YaTCompress POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_if_different
#     "${CMAKE_SOURCE_DIR}/external/stb/windows/stb_lib.dll"
#     "${CMAKE_SOURCE_DIR}/bin")

