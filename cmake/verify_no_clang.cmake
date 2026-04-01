set(_files
    "${ROOT_DIR}/CMakeLists.txt"
    "${ROOT_DIR}/src/main.cpp"
    "${ROOT_DIR}/src/include/xfawa_llvm_codegen.h"
    "${ROOT_DIR}/src/llvm/xfawa_llvm_codegen.cpp"
)

set(_forbidden_patterns
    "system\\s*\\([^\\n\\r]*clang"
    "clang_rt"
    "find_program\\s*\\([^\\n\\r]*clang"
    "execute_process\\s*\\([^\\n\\r]*clang"
    "add_custom_command\\s*\\([^\\n\\r]*clang"
)

foreach(_file IN LISTS _files)
    if(NOT EXISTS "${_file}")
        message(FATAL_ERROR "Missing file for no-clang verification: ${_file}")
    endif()

    file(READ "${_file}" _content)
    foreach(_pattern IN LISTS _forbidden_patterns)
        if(_content MATCHES "${_pattern}")
            message(FATAL_ERROR "Forbidden clang dependency found in ${_file} matching pattern: ${_pattern}")
        endif()
    endforeach()
endforeach()

message(STATUS "No clang dependency detected in compiler build/link pipeline sources.")
