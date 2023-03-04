find_program(CONAN_EXECUTABLE conan DOC "Conan package manager executable location" REQUIRED)

function(conan_exec)
    message(STATUS "Executing conan ${ARGN}")
    execute_process(
        COMMAND ${CONAN_EXECUTABLE} ${ARGN}
    )
endfunction()

macro(conan)
    MESSAGE(STATUS "Conan configuration")
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR})
    list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_BINARY_DIR})

    if(ARG0)
        set(conan_file ${ARG0})
    else()
        set(conan_file "${CMAKE_CURRENT_SOURCE_DIR}/conanfile.txt")
    endif()

    if(EXISTS ${conan_file})
        message(STATUS "Using ${conan_file}")
    else()
        message(FATAL_ERROR "${conan_file} does not exists")
    endif()

    get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if (isMultiConfig)
        foreach(TYPE ${CMAKE_CONFIGURATION_TYPES})
            conan_exec(
                install ${conan_file} --build missing -s "build_type=${TYPE}" --output-folder ${CMAKE_CURRENT_BINARY_DIR}
            )
        endforeach()
    else()
        conan_exec(
            install ${conan_file} --build missing -s "build_type=${CMAKE_BUILD_TYPE}" --output-folder ${CMAKE_CURRENT_BINARY_DIR}
        )
    endif()

endmacro()
