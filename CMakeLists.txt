project (basic_prj LANGUAGES CXX)

add_library(basic_prj INTERFACE 
    include/util/arrays.hpp
    include/util/string.hpp
    include/util/typeset.hpp
    include/util/xml.hpp
)

enable_testing()

# link this 'library' to set the c++ standard / compile-time options requested
add_library(project_options INTERFACE)
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

add_subdirectory(test)
