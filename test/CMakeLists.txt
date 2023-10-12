add_executable(basic_prj.test main.cpp)
add_executable(prime.test primes.cpp)

target_link_libraries(basic_prj.test PUBLIC basic_prj)
target_link_libraries(prime.test PUBLIC basic_prj)
