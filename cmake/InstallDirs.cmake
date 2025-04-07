include(CMakePackageConfigHelpers)

set(USER_DIR_INSTALL True CACHE BOOL "Install at the user directory")
set(cmakeModulesDir cmake)

if(USER_DIR_INSTALL)
    set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.local" CACHE PATH "install path for PREFIX" FORCE)
    set(CMAKE_INSTALL_SYSCONFDIR "$ENV{HOME}/.config/${CMAKE_PROJECT_NAME}" CACHE PATH "install path for SYSCONFDIR" FORCE)
    set(CMAKE_INSTALL_LIBDIR "${CMAKE_INSTALL_PREFIX}/lib" CACHE PATH "install path for LIBDIR" FORCE)
    set(CMAKE_INSTALL_INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include" CACHE PATH "install path for INCLUDEDIR" FORCE)
    set(CMAKE_INSTALL_BINDIR "$ENV{HOME}/bin" CACHE PATH "install path for Binaries" FORCE)

    message(STATUS "Using home dir as target : ${CMAKE_INSTALL_PREFIX} | ${CMAKE_INSTALL_BINDIR} | ${CMAKE_INSTALL_SYSCONFDIR}")
endif()
include(GNUInstallDirs)

function(setup_project_install project)
    configure_package_config_file(
        ${project}Config.cmake.in ${project}Config.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake
        PATH_VARS cmakeModulesDir
        NO_SET_AND_CHECK_MACRO
        NO_CHECK_REQUIRED_COMPONENTS_MACRO
    )

    write_basic_package_version_file(${project}ConfigVersion.cmake
        COMPATIBILITY SameMajorVersion)

    install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/${project}ConfigVersion.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/${project}Config.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${project}
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
