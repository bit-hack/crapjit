cmake_minimum_required(VERSION 3.3)
project(crapjit)

if(NOT MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -m32")
endif()

add_library(libcj
    libcj/crapjit.cpp
    libcj/crapjit.h
    libcj/runasm.h
    libcj/runasm.cpp
)

add_executable(crapjit
  main.cpp
)

target_link_libraries(crapjit libcj)

