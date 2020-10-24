cmake_minimum_required(VERSION 3.2)
set(CMAKE_CXX_STANDARD 11)

project(BarnesLogfit)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

list(APPEND CMAKE_PREFIX_PATH "/home/juanp/Desktop/TFG/Logfit/lib/cmake-build-debug/")

set(TBB_PREVIEW_FLOW_GRAPH_FEATURES 1)
set(TBB_PREVIEW_FLOW_GRAPH_NODES 1)

set(BOOST_ROOT bin/boost_1_66_0)
set(TBBROOT $ENV{TBBROOT})
set(HPFROOT ../)
set(CLROOT /opt/opencl-sdk)
set(MAIN_FILE source/cpp/mainBarnes.cpp)

set(SOURCE_FILES
        source/cpp/mainBarnes.cpp
        source/cpp/DataStructures/BarnesDataStructures.h
        source/cpp/Implementations/Bodies/BarnesBody.h

        #source/cpp/mainTest.cpp
        #source/cpp/DataStructures/ClientDataStructures.h
        #source/cpp/Implementations/Bodies/TestExecutionBody.h
        )
add_executable(Logfit ${SOURCE_FILES})

if (CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_definitions(Logfit PUBLIC NDEBUG=1)
elseif (CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_definitions(Logfit PUBLIC NDEBUG=0)
endif ()

include_directories(${TBBROOT}/include ${CLROOT}/include ${HPFROOT}/lib)

find_library(HPFLIB heterogeneous_parallel_for)
link_directories(${TBBROOT}/build/linux_intel64_gcc_cc4.8_libc2.22_kernel4.4.103_release ${HPFROOT}/lib)
target_link_libraries(Logfit tbb OpenCL tbbmalloc_proxy ${HPFLIB})