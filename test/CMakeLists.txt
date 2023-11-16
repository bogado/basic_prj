find_package(Catch2 3 REQUIRED)

add_executable(execution.test execution.cpp)
add_executable(pipe.test pipe.cpp)
add_executable(prime.test primes.cpp)

target_link_libraries(pipe.test PUBLIC basic_prj PRIVATE Catch2::Catch2WithMain)
target_link_libraries(prime.test PUBLIC basic_prj PRIVATE Catch2::Catch2)
target_link_libraries(execution.test PUBLIC basic_prj PRIVATE Catch2::Catch2)
