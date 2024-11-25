set(CLANG_VERSION CACHE STRING "Clang version to use, implies using a versioned clang")

if(CLANG_VERSION)
    set (CLANG_SUFFIX "-${CLANG_VERSION}")
endif()

find_program(CLANG_PATH "clang${CLANG_SUFFIX}")
file(REAL_PATH "${CLANG_PATH}" CLANG_PATH)
find_program(CLANGXX_PATH "clang++${CLANG_SUFFIX}")
file(REAL_PATH "${CLANGXX_PATH}" CLANGXX_PATH)

set(CMAKE_C_COMPILER   "${CLANG_PATH}")
set(CMAKE_CXX_COMPILER "${CLANGXX_PATH}")
set(CMAKE_CXX_FLAGS_INIT -stdlib=libc++)

if(APPLE)
    cmake_path(REMOVE_FILENAME CLANG_PATH OUTPUT_VARIABLE CLANG_BIN)
    cmake_path(SET CLANG_LIB_PATH NORMALIZE "${CLANG_BIN}/../lib")
    set(CMAKE_EXE_LINKER_FLAGS_INIT "-L${CLANG_LIB_PATH}/c++ -lc++ -lc++abi")
    set(CMAKE_BUILD_RPATH "${CLANG_LIB_PATH}/c++")
endif()
