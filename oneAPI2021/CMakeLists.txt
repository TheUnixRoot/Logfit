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

set(TBB_PREVIEW_FLOW_GRAPH_FEATURES 1)
set(TBB_PREVIEW_FLOW_GRAPH_NODES 1)

set(TBBROOT $ENV{TBBROOT})
set(HPFROOT ./)
set(SYCL_INCLUDE /opt/intel/oneapi/compiler/latest/linux/include/sycl)

include_directories(${TBBROOT}/include ${HPFROOT}/lib/include ${SYCL_INCLUDE})
set(MAIN_FILE source/cpp/mainTriaddOneApiGraph.cpp)

set(SOURCE_FILES
#        source/cpp/mainBarnes.cpp
#        source/cpp/DataStructures/BarnesDataStructures.h
#        source/cpp/Implementations/Bodies/BarnesBody.h
        )

add_executable(
        Devices
        source/cpp/mainGetDevices.cpp
)
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

    target_compile_definitions(LogfitOneApi PUBLIC NDEBUG=1)

    target_compile_definitions(LogfitOnePipeline PUBLIC NDEBUG=1)

    target_compile_definitions(Devices PUBLIC NDEBUG=1)

elseif (CMAKE_BUILD_TYPE MATCHES Release)

    target_compile_definitions(LogfitOneApi PUBLIC NDEBUG=0)

    target_compile_definitions(LogfitOnePipeline PUBLIC NDEBUG=0)

    target_compile_definitions(Devices PUBLIC NDEBUG=0)

endif ()

find_library(HPFLIB heterogeneous_parallel_for)
link_directories($${HPFROOT}/lib)

target_link_libraries(LogfitOneApi tbb OpenCL tbbmalloc_proxy sycl ${HPFLIB})

target_link_libraries(LogfitOnePipeline tbb OpenCL tbbmalloc_proxy sycl ${HPFLIB})

target_link_libraries(Devices OpenCL ${HPFLIB})
