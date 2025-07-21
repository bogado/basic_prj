# Set a default build type if none was specified
message(STATUS "Setting up standard project setting")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(
    STATUS "Setting build type to 'RelWithDebInfo' as none was specified.")
  set(CMAKE_BUILD_TYPE
      RelWithDebInfo
      CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui, ccmake
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
                                               "MinSizeRel" "RelWithDebInfo")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    option(USE_LIBCXX_STDLIB "Use libc++ instead of libstdc++" on)
    if (USE_LIBCXX_STDLIB)
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-stdlib=libc++>)
        target_compile_options(basic_options INTERFACE $<$<COMPILE_LANGUAGE:CXX>:-stdlib=libc++>)
        add_link_options(-stdlib=libc++)
        target_link_options(${target} INTERFACE -stdlib=libc++)
    endif()

    option(ENABLE_BUILD_WITH_TIME_TRACE "Enable -ftime-trace to generate time tracing .json files on clang" off)
    if (ENABLE_BUILD_WITH_TIME_TRACE)
        target_compile_options(${target} INTERFACE -ftime-trace)
    endif()
endif()

# Generate compile_commands.json to make it easier to work with clang based
# tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_IPO
       "Enable Interprocedural Optimization, aka Link Time Optimization (LTO)"
       OFF)

if(ENABLE_IPO)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT result OUTPUT output)
  if(result)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
  else()
    message(SEND_ERROR "IPO is not supported: ${output}")
  endif()
endif()

function(setup_target target type)
    set_project_warnings(${target} ${type})
    enable_sanitizers(${target} ${type})
    set_target_properties(${target} PROPERTIES
        CMAKE_CXX_STANDARD 23
        CMAKE_CXX_EXTENSIONS False)
    target_compile_features(${target} ${type} cxx_std_23)
endfunction()
