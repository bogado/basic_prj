find_package(Catch2 3 REQUIRED)

add_executable(basic_utils_test
    buffer.cpp
    pipe.cpp
    primes.cpp
    options.cpp
    execution.cpp
)

target_compile_definitions(basic_utils_test PRIVATE NDEBUG)

target_link_libraries(basic_utils_test PUBLIC basic_utils PRIVATE Catch2::Catch2WithMain)
