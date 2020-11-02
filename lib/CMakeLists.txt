cmake_minimum_required(VERSION 3.17)

set(CMAKE_CXX_COMPILER "dpcpp")

project(heterogeneous_parallel_for)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
set(CMAKE_CXX_STANDARD 17)
set(USER ${USER} juanp)


if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(TBB_PREVIEW_FLOW_GRAPH_FEATURES 1)
set(TBB_PREVIEW_FLOW_GRAPH_NODES 1)

set(TBBROOT /home/juanp/Desktop/TFG/oneTBB-2020.3)
set(OPENCLROOT /opt/intel/opencl/SDK)
set(SYCL_INCLUDE /opt/intel/oneapi/compiler/latest/linux/include/sycl/)


set(SOURCE_FILES
        include/body/IGraphBody.h
        include/body/IPipelineBody.h
        include/engine/EngineFactory.h
        include/engine/DynamicEngine.h
        include/engine/LogFitEngine.h
        include/scheduler/GraphScheduler.h
        include/scheduler/PipelineScheduler.h
        include/scheduler/SchedulerFactory.h
        include/utils/Utils.h

        lib/DataStructures/ProvidedDataStructures.h
        lib/Helpers/ConsoleUtils.cpp
        lib/Helpers/Pipeline/Filter.cpp
        lib/Implementations/Engines/LogFitEngine.cpp
        lib/Implementations/Engines/DynamicEngine.cpp
        lib/Implementations/Schedulers/GraphScheduler.cpp
        lib/Implementations/Schedulers/PipelineScheduler.cpp
        lib/Interfaces/Engines/IEngine.cpp
        lib/Interfaces/Schedulers/IScheduler.cpp
        lib/Interfaces/Bodies/IBody.cpp
        )

add_library(heterogeneous_parallel_for SHARED ${SOURCE_FILES})


if (CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_definitions(heterogeneous_parallel_for PUBLIC NDEBUG=1)
elseif (CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_definitions(heterogeneous_parallel_for PUBLIC NDEBUG=0)
endif ()

include_directories(${TBBROOT}/include ${OPENCLROOT}/include)
set_target_properties(heterogeneous_parallel_for PROPERTIES LINKER_LANGUAGE CXX)
#link_directories(${TBBROOT}/lib/intel64/gcc4.8)
link_directories(${TBBROOT}/build/linux_intel64_gcc_cc4.8_libc2.22_kernel4.4.103_release)
target_link_libraries(heterogeneous_parallel_for tbb OpenCL tbbmalloc_proxy)

add_custom_command(
        TARGET heterogeneous_parallel_for POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/cmake-build-debug/libheterogeneous_parallel_for.so
        ${CMAKE_SOURCE_DIR}/libheterogeneous_parallel_for.so)
add_custom_command(
        TARGET heterogeneous_parallel_for PRE_BUILD
        COMMAND rm -f ${CMAKE_SOURCE_DIR}/cmake-build-debug/libheterogeneous_parallel_for.so || true)