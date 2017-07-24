cmake_minimum_required(VERSION 3.2)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_library(pd-miniparser STATIC
  sources/pd-miniparser.cc)
target_include_directories(pd-miniparser
  PUBLIC sources)

add_executable(pd-miniparser-main
  sources/pd-miniparser-main.cc)
target_link_libraries(pd-miniparser-main
  pd-miniparser)
set_target_properties(pd-miniparser-main PROPERTIES
  OUTPUT_NAME pd-miniparser)
