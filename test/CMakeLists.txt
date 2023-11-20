add_executable(execution.test execution.cpp)
add_executable(buffer.test buffer.cpp)
add_executable(prime.test primes.cpp)

target_link_libraries(buffer.test PUBLIC basic_prj PRIVATE Catch2::Catch2WithMain)
target_link_libraries(prime.test PUBLIC basic_prj PRIVATE Catch2::Catch2)
target_link_libraries(execution.test PUBLIC basic_prj PRIVATE Catch2::Catch2)
