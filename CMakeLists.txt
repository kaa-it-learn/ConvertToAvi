cmake_minimum_required(VERSION 3.6)
project(ConvertToAvi)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

set(SOURCE_FILES main.cpp AviWriter.cpp AviWriter.h Common.h)
add_executable(ConvertToAvi ${SOURCE_FILES})