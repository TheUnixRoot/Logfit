cmake_minimum_required(VERSION 3.17)
project(OneApi)

set(CMAKE_CXX_STANDARD 17)

add_library(OneApi SHARED IOneApiBody.h
        OneApiScheduler.h
        OneApiScheduler.cpp
        lib/IBody.cpp
        lib/IScheduler.cpp)