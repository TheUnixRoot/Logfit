cmake_minimum_required(VERSION 3.2)
set(CMAKE_CXX_COMPILER "dpcpp")

set(USER ${USER} juanp)

project(Logfit)

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CL_TARGET_OPENCL_VERSION 220)
list(APPEND CMAKE_PREFIX_PATH "../lib/cmake-build-debug/")

set(TBB_PREVIEW_FLOW_GRAPH_FEATURES 1)
set(TBB_PREVIEW_FLOW_GRAPH_NODES 1)

set(TBBROOT $ENV{TBBROOT})
set(HPFROOT ../)
set(CLROOT /opt/intel19/system_studio_2019/opencl/SDK)
set(SYCL_INCLUDE /opt/intel/oneapi/compiler/latest/linux/include/sycl)

#set(MAIN_FILE source/cpp/mainBarnes.cpp)
#set(MAIN_FILE source/cpp/mainTest.cpp)
#include_directories(${TBBROOT}/include ${CLROOT}/include ${HPFROOT}/lib/include ${SYCL_INCLUDE})
include_directories(${TBBROOT}/include ${HPFROOT}/lib/include ${SYCL_INCLUDE})
set(MAIN_FILE source/cpp/mainTriaddOneApiGraph.cpp)

add_executable(
        BarnesGraph
        source/cpp/mainBarnes.cpp
        source/cpp/DataStructures/BarnesDataStructures.h
        source/cpp/Implementations/Bodies/BarnesBody.h
#        source/cpp/Implementations/Tests/TBarnesBodyTest.h
)
add_executable(
        BarnesOneApi
        source/cpp/mainBarnesOneApiGraph.cpp
        source/cpp/DataStructures/BarnesOneApiDataStructures.h
        source/cpp/Implementations/Bodies/BarnesOneApiBody.h
        #        source/cpp/Implementations/Tests/TBarnesBodyTest.h
)
add_executable(
        BarnesOnePipeline
        source/cpp/mainBarnesOneApiPipeline.cpp
        source/cpp/DataStructures/BarnesOneApiDataStructures.h
        source/cpp/Implementations/Bodies/BarnesOneApiBody.h
        #        source/cpp/Implementations/Tests/TBarnesBodyTest.h
)
add_executable(
        BarnesPipeline
        source/cpp/mainBarnesPipeline.cpp
        source/cpp/DataStructures/BarnesPipelineDataStructures.h
        source/cpp/Implementations/Bodies/BarnesPipelineBody.h
        #        source/cpp/Implementations/Tests/TBarnesBodyTest.h
)
add_executable(
        LogfitGraph
        source/cpp/mainTriaddGraph.cpp
        source/cpp/DataStructures/TriaddGraphDataStructures.h
        source/cpp/Implementations/Bodies/TriaddGraphBody.h
        source/cpp/Implementations/Tests/TriaddGraphBodyTest.h
)
add_executable(
        Devices
        source/cpp/mainGetDevices.cpp
)
add_executable(
        LogfitPipeline
        source/cpp/mainTriaddPipeline.cpp
        source/cpp/DataStructures/TriaddGraphDataStructures.h
        source/cpp/Implementations/Bodies/TriaddPipelineBody.h
        source/cpp/Implementations/Tests/TriaddPipelineBodyTest.h
        source/cpp/DataStructures/TriaddPipelineDataStructures.h)
add_executable(
        LogfitOneApi
        source/cpp/mainTriaddOneApiGraph.cpp
        source/cpp/Implementations/Bodies/TriaddOneApiBody.h
        source/cpp/Implementations/Tests/TriaddOneApiBodyTest.h
)
add_executable(
        LogfitOnePipeline
        source/cpp/mainTriaddOneApiPipeline.cpp
        source/cpp/Implementations/Bodies/TriaddOneApiBody.h
        source/cpp/Implementations/Tests/TriaddOneApiBodyTest.h
)

if (CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_definitions(LogfitGraph PUBLIC NDEBUG=1)

    target_compile_definitions(BarnesGraph PUBLIC NDEBUG=1)

    target_compile_definitions(BarnesOneApi PUBLIC NDEBUG=1)

    target_compile_definitions(BarnesPipeline PUBLIC NDEBUG=1)

    target_compile_definitions(BarnesOnePipeline PUBLIC NDEBUG=1)

    target_compile_definitions(LogfitOneApi PUBLIC NDEBUG=1)

    target_compile_definitions(LogfitOnePipeline PUBLIC NDEBUG=1)

    target_compile_definitions(LogfitPipeline PUBLIC NDEBUG=1)

    target_compile_definitions(Devices PUBLIC NDEBUG=1)
elseif (CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_definitions(LogfitGraph PUBLIC NDEBUG=0)

    target_compile_definitions(BarnesGraph PUBLIC NDEBUG=0)

    target_compile_definitions(BarnesOneApi PUBLIC NDEBUG=0)

    target_compile_definitions(BarnesPipeline PUBLIC NDEBUG=0)

    target_compile_definitions(BarnesOnePipeline PUBLIC NDEBUG=0)

    target_compile_definitions(LogfitOneApi PUBLIC NDEBUG=0)

    target_compile_definitions(LogfitOnePipeline PUBLIC NDEBUG=0)

    target_compile_definitions(LogfitPipeline PUBLIC NDEBUG=0)

    target_compile_definitions(Devices PUBLIC NDEBUG=0)
endif ()

find_library(HPFLIB heterogeneous_parallel_for)
link_directories($${HPFROOT}/lib)

target_link_libraries(LogfitGraph tbb OpenCL tbbmalloc_proxy ${HPFLIB})

target_link_libraries(BarnesGraph tbb OpenCL tbbmalloc_proxy ${HPFLIB})

target_link_libraries(LogfitPipeline tbb OpenCL tbbmalloc_proxy ${HPFLIB})

target_link_libraries(BarnesPipeline tbb OpenCL tbbmalloc_proxy ${HPFLIB})

target_link_libraries(LogfitOneApi tbb OpenCL tbbmalloc_proxy sycl ${HPFLIB})

target_link_libraries(BarnesOneApi tbb OpenCL tbbmalloc_proxy sycl ${HPFLIB})

target_link_libraries(LogfitOnePipeline tbb OpenCL tbbmalloc_proxy sycl ${HPFLIB})

target_link_libraries(BarnesOnePipeline tbb OpenCL tbbmalloc_proxy sycl ${HPFLIB})

target_link_libraries(Devices OpenCL ${HPFLIB})
