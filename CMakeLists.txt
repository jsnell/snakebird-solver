cmake_minimum_required(VERSION 2.8)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O3 -g3 -Wall -Werror -Wno-sign-compare -march=native -Isrc")

add_executable(snakebird
  src/main.cc
  src/third-party/cityhash/city.cc
  src/third-party/snappy/snappy.cc
  src/third-party/snappy/snappy-sinksource.cc)

enable_testing()
# Usual workaround for the broken test build dependency handling in CMake.
# Every test must depend on this dummy target.
add_test(build_test_code "${CMAKE_COMMAND}" --build ${CMAKE_BINARY_DIR} --target all)

macro(define_test name)
  add_executable(${name}.testbin
    src/test/${name}.cc)
  add_test(${name} bin/${name}.testbin)
  set_tests_properties(${name} PROPERTIES DEPENDS
    build_test_code)
endmacro()

