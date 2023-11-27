add_executable(utils_test
    buffer.cpp
    pipe.cpp
    primes.cpp)

add_executable(execution.test execution.cpp)

target_compile_definitions(utils_test PRIVATE NDEBUG)

target_link_libraries(utils_test PUBLIC basic_prj PRIVATE Catch2::Catch2WithMain)
target_link_libraries(execution.test PUBLIC basic_prj PRIVATE Catch2::Catch2WithMain)
