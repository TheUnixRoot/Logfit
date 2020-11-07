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

#add_compile_definitions(USE_NEW_PIPELINE)

set(TBBROOT $ENV{TBBROOT})
set(HPFROOT ./)
set(SYCL_INCLUDE /opt/intel/oneapi/compiler/latest/linux/include/sycl)

include_directories(${TBBROOT}/include ${HPFROOT}/lib/include ${SYCL_INCLUDE})
set(MAIN_FILE source/cpp/mainTriaddOneApiGraph.cpp)

set(SOURCE_FILES
        lib/Implementations/Engines/LogFitEngine.cpp
        lib/Implementations/Engines/DynamicEngine.cpp
        lib/Interfaces/Engines/IEngine.cpp
        lib/Interfaces/Schedulers/IScheduler.cpp
        lib/Helpers/ConsoleUtils.h
        include/engine/EngineFactory.h
        lib/Interfaces/Bodies/IBody.cpp
        include/utils/Utils.h
        include/scheduler/SchedulerFactory.h
        include/body/IOneApiBody.h
        include/scheduler/OneApiScheduler.h
        include/scheduler/OnePipelineScheduler.h
        lib/Helpers/Pipeline/OneFilter.cpp
        )
add_library(heterogeneous_parallel_for SHARED ${SOURCE_FILES})

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
target_include_directories(LogfitOneApi PRIVATE
        include
        )
target_include_directories(LogfitOnePipeline PRIVATE
        include
        )
link_directories($${HPFROOT}/lib)

target_link_libraries(LogfitOneApi tbb OpenCL tbbmalloc_proxy sycl heterogeneous_parallel_for)

target_link_libraries(LogfitOnePipeline tbb OpenCL tbbmalloc_proxy sycl heterogeneous_parallel_for)

target_link_libraries(Devices OpenCL)
