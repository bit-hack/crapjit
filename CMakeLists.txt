cmake_minimum_required(VERSION 3.3)
project(crapjit)

if(NOT MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -m32")
endif()

set(SOURCE_FILES
    chunks.cpp
    chunks.h
    crapjit.cpp
    crapjit.h
    main.cpp
    out.h
    system.cpp
    system.h)

add_executable(crapjit ${SOURCE_FILES})