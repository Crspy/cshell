cmake_minimum_required(VERSION 3.00)
project(cshell)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)


include_directories(.)
add_executable(cshell
        CMD.cpp
        CMD.hpp
        Job.cpp
        Job.hpp
        main.cpp
        Shell.cpp
        Shell.hpp
        Util.cpp
        Util.hpp)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s -O2")