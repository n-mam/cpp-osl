cmake_minimum_required(VERSION 3.15)

project(osl)

add_executable(TestOSL Test.cpp)

SET_PROPERTY(TARGET TestOSL PROPERTY CXX_STANDARD 17)

target_include_directories(
 TestOSL
 PRIVATE
 ${CMAKE_CURRENT_SOURCE_DIR}
 ${CMAKE_CURRENT_SOURCE_DIR}/INCLUDE
)