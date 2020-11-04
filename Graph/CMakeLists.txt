cmake_minimum_required(VERSION 3.2)
set(CMAKE_CXX_COMPILER "dpcpp")

set(USER ${USER} juanp)

project(BarnesLogfit)

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")
set(CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CL_TARGET_OPENCL_VERSION 220)
list(APPEND CMAKE_PREFIX_PATH "/home/juanp/Desktop/TFG/Logfit/lib/cmake-build-debug/")

set(TBB_PREVIEW_FLOW_GRAPH_FEATURES 1)
set(TBB_PREVIEW_FLOW_GRAPH_NODES 1)

set(TBBROOT $ENV{TBBROOT})
set(HPFROOT ../)
set(CLROOT /opt/intel/opencl/SDK)
set(SYCL_INCLUDE /opt/intel/oneapi/compiler/latest/linux/include/sycl)

#set(MAIN_FILE source/cpp/mainBarnes.cpp)
#set(MAIN_FILE source/cpp/mainTest.cpp)
#include_directories(${TBBROOT}/include ${CLROOT}/include ${HPFROOT}/lib/include ${SYCL_INCLUDE})
include_directories(${TBBROOT}/include ${HPFROOT}/lib/include ${SYCL_INCLUDE})
set(MAIN_FILE source/cpp/mainOneApiTest.cpp)

set(SOURCE_FILES
#        source/cpp/mainBarnes.cpp
#        source/cpp/DataStructures/BarnesDataStructures.h
#        source/cpp/Implementations/Bodies/BarnesBody.h



        )
add_executable(
        LogfitGraph
        source/cpp/mainTest.cpp
        source/cpp/DataStructures/ClientDataStructures.h
        source/cpp/Implementations/Bodies/TestExecutionBody.h
        )
add_executable(
        LogfitOneApi
        source/cpp/Implementations/Bodies/TestOneApiBody.h
        source/cpp/mainOneApiTest.cpp
        )

if (CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_definitions(LogfitGraph PUBLIC NDEBUG=1)

    target_compile_definitions(LogfitOneApi PUBLIC NDEBUG=1)
elseif (CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_definitions(LogfitGraph PUBLIC NDEBUG=0)

    target_compile_definitions(LogfitOneApi PUBLIC NDEBUG=0)
endif ()

find_library(HPFLIB heterogeneous_parallel_for)
link_directories(${TBBROOT}/build/linux_intel64_gcc_cc4.8_libc2.22_kernel4.4.103_release ${HPFROOT}/lib)

target_link_libraries(LogfitGraph tbb OpenCL tbbmalloc_proxy ${HPFLIB})

target_link_libraries(LogfitOneApi tbb OpenCL tbbmalloc_proxy sycl ${HPFLIB})