add_executable(execution.test execution.cpp)
add_executable(pipe.test pipe.cpp)
add_executable(prime.test primes.cpp)

target_link_libraries(pipe.test PUBLIC basic_prj)
target_link_libraries(prime.test PUBLIC basic_prj)
target_link_libraries(execution.test PUBLIC basic_prj)
