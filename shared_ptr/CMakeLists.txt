cmake_minimum_required(VERSION 3.15)

project(shared_ptr_testing)
include_directories(.)
add_subdirectory(gtest)

add_executable(shared_ptr_testing
        test/main.cpp
        src/shared_ptr.h
        src/shared_ptr.cpp
        test/test_object.cpp
        test/test_object.h src/shared_ptr.cpp)

set_property(TARGET shared_ptr_testing PROPERTY CXX_STANDARD 17)

target_link_libraries(shared_ptr_testing gtest)
