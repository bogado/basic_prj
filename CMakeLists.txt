add_library(basic_prj INTERFACE 
    include/util/arrays.hpp
    include/util/string.hpp
    include/util/typeset.hpp
    include/util/xml.hpp
)

enable_testing()

include(cmake/StandardProjectSettings.cmake)

# link this 'library' to set the c++ standard / compile-time options requested
add_library(project_options INTERFACE)
target_compile_features(project_options INTERFACE cxx_std_20)

if(cmake_cxx_compiler_id MATCHES ".*clang")
  option(ENABLE_BUILD_WITH_TIME_TRACE "Enable -ftime-trace to generate time tracing .json files on clang" off)
  if (ENABLE_BUILD_WITH_TIME_TRACE)
    add_compile_definitions(project_options INTERFACE -ftime-trace)
  endif()
endif()

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
