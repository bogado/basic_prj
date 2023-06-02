add_library(basic_prj INTERFACE 
    include/util/arrays.hpp
    include/util/string.hpp
    include/util/typeset.hpp
    include/util/xml.hpp
)

target_include_directories(basic_prj INTERFACE include)
