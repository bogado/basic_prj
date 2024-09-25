include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(USER_DIR_INSTALL True CACHE BOOL "Install at the user directory")
set(cmakeModulesDir cmake)

if(USER_DIR_INSTALL)
    set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.local" CACHE PATH "Install prefix" FORCE)
    set(CMAKE_INSTALL_BINDIR "$ENV{HOME}/bin" CACHE PATH "Binary install prefix")
    set(CMAKE_INSTALL_SYSCONFDIR "$ENV{HOME}/.config/${CMAKE_PROJECT_NAME}" CACHE PATH "Configuration prefix")

    message(STATUS "Using home dir as target : ${CMAKE_INSTALL_PREFIX} | ${CMAKE_INSTALL_BINDIR} | ${CMAKE_INSTALL_SYSCONFDIR}")
endif()

function(setup_install name)
    configure_package_config_file(
        ${name}Config.cmake.in ${name}Config.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake
        PATH_VARS cmakeModulesDir
        NO_SET_AND_CHECK_MACRO
        NO_CHECK_REQUIRED_COMPONENTS_MACRO
    )

    write_basic_package_version_file(${name}ConfigVersion.cmake
        COMPATIBILITY SameMajorVersion)

    install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/${name}ConfigVersion.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/${name}Config.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${name}
    )
endfunction()

function(export_target_library_interface project target exported_name)
    add_library("${project}::${exported_name}" ALIAS ${target})

    set_target_properties(${target} PROPERTIES
        OUTPUT_NAME ${target} 
        EXPORT_NAME ${exported_name}
    )


    install(EXPORT ${project} DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${project} 
        NAMESPACE "${project}::"

        EXPORT_LINK_INTERFACE_LIBRARIES
    )
endfunction()
