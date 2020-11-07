cmake_minimum_required(VERSION 3.2)
set(CMAKE_CXX_COMPILER "dpcpp")

set(USER ${USER} juanp)

project(TFG)

add_subdirectory(TBB2020)
add_subdirectory(lib)
