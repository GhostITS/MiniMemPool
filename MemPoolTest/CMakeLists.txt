cmake_minimum_required(VERSION 3.14)

project("MemPoolTest")

set(CMAKE_BUILD_TYPE "Debug")

#设置源文件更目录并设置别名为DIR_ROOT
aux_source_directory(.  DIR_ROOT)

set(CMAKE_CXX_STANDARD  11)

add_executable(${PROJECT_NAME} MiniMemPool.cpp DoTest.cpp leetcode/leetcode.cpp)