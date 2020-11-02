cmake_minimum_required(VERSION 3.17)
set(CMAKE_CXX_COMPILER "dpcpp")

set(USER ${USER} juanp)

project(heterogeneous_parallel_for)

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_STANDARD 17)


if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(TBB_PREVIEW_FLOW_GRAPH_FEATURES 1)
set(TBB_PREVIEW_FLOW_GRAPH_NODES 1)

set(TBBROOT $ENV{TBBROOT})
set(CLROOT /opt/intel/opencl/SDK)
set(SYCL_INCLUDE /opt/intel/oneapi/compiler/latest/linux/include/sycl)

set(SOURCE_FILES
        lib/DataStructures/ProvidedDataStructures.h
        include/scheduler/GraphScheduler.h
        lib/Implementations/Engines/LogFitEngine.cpp
        include/scheduler/PipelineScheduler.h
        lib/Implementations/Engines/DynamicEngine.cpp
        lib/Interfaces/Engines/IEngine.cpp
        lib/Interfaces/Schedulers/IScheduler.cpp
        lib/Helpers/ConsoleUtils.h
        include/engine/EngineFactory.h
        lib/Interfaces/Bodies/IBody.cpp
        include/utils/Utils.h
        include/body/IGraphBody.h
        include/body/IPipelineBody.h
        include/scheduler/SchedulerFactory.h
        include/body/IOneApiBody.h
        include/scheduler/OneApiScheduler.h)

add_library(heterogeneous_parallel_for SHARED ${SOURCE_FILES})


if (CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_definitions(heterogeneous_parallel_for PUBLIC NDEBUG=1)
elseif (CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_definitions(heterogeneous_parallel_for PUBLIC NDEBUG=0)
endif ()

include_directories(${TBBROOT}/include ${CLROOT}/include ${SYCL_INCLUDE})
set_target_properties(heterogeneous_parallel_for PROPERTIES LINKER_LANGUAGE CXX)
link_directories(${TBBROOT}/build/linux_intel64_gcc_cc4.8_libc2.22_kernel4.4.103_release)
target_link_libraries(heterogeneous_parallel_for tbb OpenCL sycl tbbmalloc_proxy)

add_custom_command(
        TARGET heterogeneous_parallel_for POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/cmake-build-debug/libheterogeneous_parallel_for.so
        ${CMAKE_SOURCE_DIR}/libheterogeneous_parallel_for.so)
add_custom_command(
        TARGET heterogeneous_parallel_for PRE_BUILD
        COMMAND rm -f ${CMAKE_SOURCE_DIR}/cmake-build-debug/libheterogeneous_parallel_for.so || true)