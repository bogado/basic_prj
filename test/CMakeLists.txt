find_package(Catch2 3 REQUIRED)
find_package(fmt REQUIRED)

add_executable(utils_test
    buffer.cpp
    pipe.cpp
    primes.cpp
    execution.cpp
)

target_compile_definitions(utils_test PRIVATE NDEBUG)

target_link_libraries(utils_test PUBLIC basic_prj PRIVATE fmt::fmt Catch2::Catch2WithMain)
