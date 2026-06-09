cmake_minimum_required(VERSION 3.15)
project(xfawac CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DNOMINMAX /DWIN32_LEAN_AND_MEAN /D_CRT_SECURE_NO_WARNINGS")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Z7 /O2")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:CONSOLE /STACK:1048576")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
endif()

set(LLVM_DIR "D:/llvm-zero-dependency/lib/cmake/llvm")
find_package(LLVM REQUIRED CONFIG)
find_package(LLD REQUIRED CONFIG HINTS "D:/llvm-zero-dependency/lib/cmake/lld")

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

include_directories(
    ${CMAKE_SOURCE_DIR}/src/include
    ${LLVM_INCLUDE_DIRS}
)

separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

llvm_map_components_to_libnames(llvm_libs support core X86Info X86AsmParser X86CodeGen Analysis Passes TransformUtils ScalarOpts)

set(SOURCES
    src/main.cpp
    src/lexer/lexer.cpp
    src/parser/parser.cpp
    src/llvm/xfawa_llvm_codegen.cpp
    src/config/config.cpp
    src/xfawa_types.cpp
    src/mods/mods_system.cpp
    src/include/xfawa_xfw.cpp
)

add_library(xraphics STATIC
    src/runtime/xfawa_winrt.cpp
)

target_include_directories(xraphics PRIVATE
    ${CMAKE_SOURCE_DIR}/src/include
)

add_executable(xfawac ${SOURCES})

add_dependencies(xfawac xraphics)

add_custom_target(verify_no_clang
    COMMAND ${CMAKE_COMMAND} -DROOT_DIR=${CMAKE_SOURCE_DIR} -P ${CMAKE_SOURCE_DIR}/cmake/verify_no_clang.cmake
    COMMENT "Verifying that the compiler pipeline does not depend on clang"
)

if(BUILD_TESTING)
    add_test(
        NAME no_clang_dependency
        COMMAND ${CMAKE_COMMAND} -DROOT_DIR=${CMAKE_SOURCE_DIR} -P ${CMAKE_SOURCE_DIR}/cmake/verify_no_clang.cmake
    )
endif()

target_link_libraries(xfawac
    ${llvm_libs}
)

if(MSVC)
    target_link_libraries(xfawac
        lldCOFF
        ntdll.lib
        kernel32.lib
        user32.lib
        ole32.lib
        uuid.lib
        advapi32.lib
        Ws2_32.lib
        Version.lib
        Shell32.lib
        Shlwapi.lib
        DbgHelp.lib
        Psapi.lib
    )
    set_target_properties(xraphics PROPERTIES
        OUTPUT_NAME "xraphics"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/internal"
        ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug/internal"
        ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release/internal"
        ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/RelWithDebInfo/internal"
        ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/MinSizeRel/internal"
    )
else()
    target_link_libraries(xfawac
        lldCOFF
    )
endif()
