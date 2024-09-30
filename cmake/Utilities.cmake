function(append_target_property target property)
    get_target_property(property_value ${target} ${property})
    list(APPEND property_value ${ARGV})
    set_target_properties(${target} PROPERTIES 
          "${property}" "${property_value}"
    )
endfunction()

function(update_target target type options)
  include(CMakeParseArguments)
  cmake_parse_arguments(
      arg
      "IMPORTED" "" ""
      ${ARGN}
  )

  if (arg_IMPORTED)
      append_target_property(${target} INTERFACE_${type}_OPTIONS "${options}")
  elseif(${type} STREQUAL COMPILER)
      target_compile_options(${target} INTERFACE ${options})
  elseif(${type} STREQUAL LINK)
      target_link_options(${target} INTERFACE "${options}")
  else()
      message(FATAL_ERROR "\"${type}\" is not supported by update_target")
  endif()
endfunction()
