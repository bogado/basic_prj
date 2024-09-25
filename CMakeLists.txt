cmake_minimum_required(VERSION 3.25)
project (basic LANGUAGES CXX VERSION 1.0)

enable_testing()

include(cmake/InstallDirs.cmake)
setup_install(basic)

set(basic_prj_headers 
    include/util/arrays.hpp
    include/util/buffer.hpp
    include/util/converters.hpp
    include/util/debug.hpp
    include/util/environment.hpp
    include/util/execution.hpp
    include/util/filesystem.hpp
    include/util/generator.hpp
    include/util/pipe.hpp
    include/util/string.hpp
    include/util/string_list.hpp
    include/util/system.hpp
    include/util/typeset.hpp
    include/util/xml.hpp
)

# link this 'library' to set the c++ standard / compile-time options requested
add_library(basic_options INTERFACE)

set_target_properties(basic_options PROPERTIES
    CMAKE_CXX_STANDARD 23
    CMAKE_CXX_EXTENSIONS False)

target_compile_features(basic_options INTERFACE cxx_std_23)


install(TARGETS basic_options
    EXPORT basic
)

export_target_library_interface(basic basic_options options)

add_library(basic_utils INTERFACE ${basic_prj_headers})

target_link_libraries(basic_utils INTERFACE basic::options)

target_include_directories(basic_utils
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_sources( basic_utils
    INTERFACE
        FILE_SET HEADERS
        TYPE HEADERS
        BASE_DIRS
            include
        FILES
            ${basic_prj_headers}
)

install(TARGETS basic_utils
    EXPORT basic
    FILE_SET HEADERS
    DESTINATION ${CMAKE_INSTALL_LIBDIR})

export_target_library_interface(basic basic_utils utils)

include(cmake/StandardProjectSettings.cmake)

# inlcude catch2 v3 setup
include(cmake/Catch2.cmake)

# standard compiler warnings
include(cmake/CompilerWarnings.cmake)
set_project_warnings(basic_options)

# enable cache system
include(cmake/Cache.cmake)

# sanitizer options if supported by compiler
include(cmake/Sanitizers.cmake)
enable_sanitizers(basic_options)

# allow for static analysis options
include(cmake/StaticAnalyzers.cmake)

set(UTILS_TEST_ENABLED FALSE CACHE BOOL "Enable building the tests for the utilities")

if(UTILS_TEST_ENABLED) 
    add_subdirectory(test)
endif()
