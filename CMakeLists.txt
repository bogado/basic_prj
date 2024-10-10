cmake_minimum_required(VERSION 3.25)
project (basic_prj LANGUAGES CXX VERSION 1.0)

enable_testing()

include(cmake/InstallDirs.cmake)
# inlcude catch2 v3 setup
include(cmake/Catch2.cmake)
# standard compiler warnings
include(cmake/CompilerWarnings.cmake)
# enable cache system
include(cmake/Cache.cmake)
# sanitizer options if supported by compiler
include(cmake/Sanitizers.cmake)
# allow for static analysis options
include(cmake/StaticAnalyzers.cmake)
# link this 'library' to set the c++ standard / compile-time options requested
include(cmake/StandardProjectSettings.cmake)

setup_project_install(basic_prj)

add_library(basic_options INTERFACE)
install(FILES
            cmake/Cache.cmake
            cmake/CompilerWarnings.cmake
            cmake/CPM.cmake
            cmake/Debug.cmake
            cmake/InstallDirs.cmake
            cmake/Sanitizers.cmake
            cmake/StandardProjectSettings.cmake
            cmake/StaticAnalyzers.cmake
            cmake/Utilities.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/basic_prj
)

set_target_properties(basic_options PROPERTIES
    CMAKE_CXX_STANDARD 23
    CMAKE_CXX_EXTENSIONS False)

target_compile_features(basic_options INTERFACE cxx_std_23)

install(TARGETS basic_options
    EXPORT basic_prj
)

export_target_library_interface(basic_prj basic_options options)

add_library(basic_utils INTERFACE)

target_link_libraries(basic_utils INTERFACE basic_options)

target_include_directories(basic_utils
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_sources(basic_utils
    INTERFACE
        FILE_SET HEADERS
        TYPE HEADERS
        BASE_DIRS
            include
        FILES
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

install(TARGETS basic_utils
    EXPORT basic_prj
    FILE_SET HEADERS
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

export_target_library_interface(basic_prj basic_utils utils)


set(UTILS_TEST_ENABLED FALSE CACHE BOOL "Enable building the tests for the utilities")

if(UTILS_TEST_ENABLED) 
    add_subdirectory(test)
    add_test(NAME BasicUtilsTests COMMAND basic_utils_test)
endif()
