cmake_minimum_required(VERSION 3.25)
project (basic_prj LANGUAGES CXX)

add_library(basic_prj INTERFACE 
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

enable_testing()

# link this 'library' to set the c++ standard / compile-time options requested
add_library(project_options INTERFACE)
set_target_properties(project_options PROPERTIES
    CMAKE_CXX_STANDARD 23
    CMAKE_CXX_EXTENSIONS False)
target_compile_features(project_options INTERFACE cxx_std_23)

include(cmake/StandardProjectSettings.cmake)

# inlcude catch2 v3 setup
include(cmake/Catch2.cmake)

# standard compiler warnings
include(cmake/CompilerWarnings.cmake)
set_project_warnings(project_options)

# enable cache system
include(cmake/Cache.cmake)

# sanitizer options if supported by compiler
include(cmake/Sanitizers.cmake)
enable_sanitizers(project_options)

# allow for static analysis options
include(cmake/StaticAnalyzers.cmake)

target_include_directories(basic_prj INTERFACE include)
target_link_libraries(basic_prj INTERFACE project_options)

set(UTILS_TEST_ENABLED FALSE CACHE BOOL "Enable building the tests for the utilities")

install(TARGETS basic_prj)

if(UTILS_TEST_ENABLED) 
    add_subdirectory(test)
endif()
