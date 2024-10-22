cmake_minimum_required(VERSION 3.10)

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures for Mac OS X" FORCE)
elseif(WIN32)
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(astc_arm)

set(BUILD_SHARED_LIBS ON)

file(GLOB_RECURSE SOURCES Source/*.cpp)
file(GLOB_RECURSE HEADERS Source/*.h)

add_library(astc_arm SHARED ${SOURCES} ${HEADERS})

# Dll导出符号表 ASTCENC_DYNAMIC_LIBRARY
target_compile_definitions(astc_arm PRIVATE ASTCENC_DYNAMIC_LIBRARY)

target_include_directories(astc_arm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/ThirdParty)

target_include_directories(astc_arm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Source)

# 设置库文件和可执行文件的输出目录
if(WIN32)
    set_target_properties(astc_arm PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/builds/windows
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/builds/windows
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/builds/windows
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/builds/windows
        ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/builds/windows
        ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/builds/windows
    )
elseif(APPLE)
    set_target_properties(astc_arm PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/builds/macos
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/builds/macos
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/builds/macos
    )
    message(STATUS "mac output directory: ${CMAKE_SOURCE_DIR}/builds/macos")
endif()

# 配置安装规则，可选
# install(TARGETS astc_arm
#     RUNTIME DESTINATION bin
#     LIBRARY DESTINATION lib
#     ARCHIVE DESTINATION lib)
# install(FILES ${HEADERS} DESTINATION include)