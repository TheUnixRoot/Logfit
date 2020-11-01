cmake_minimum_required(VERSION 3.2)
set(CMAKE_CXX_STANDARD 11)

project(BarnesLogfit)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(TBB_PREVIEW_FLOW_GRAPH_FEATURES 1)
set(TBB_PREVIEW_FLOW_GRAPH_NODES 1)

set(BOOST_ROOT bin/boost_1_66_0)
set(TBBROOT $ENV{TBBROOT})
set(CLROOT /opt/opencl-sdk)
set(MAIN_FILE source/cpp/mainBarnes.cpp)
set(SOURCE_FILES
        source/cpp/mainBarnes.cpp
        source/cpp/DataStructures/BarnesDataStructures.h
        source/cpp/Implementations/Bodies/BarnesBody.h

        #source/cpp/mainTest.cpp
        #source/cpp/DataStructures/ClientDataStructures.h
        #source/cpp/Implementations/Bodies/TestExecutionBody.h

        source/cpp/DataStructures/ProvidedDataStructures.h
        source/cpp/Implementations/Scheduler/GraphScheduler.h
        source/cpp/Implementations/Engines/LogFitEngine.h
        source/cpp/Interfaces/Bodies/IBody.h
        source/cpp/Interfaces/Engines/IEngine.h
        source/cpp/Interfaces/Scheduler/IScheduler.h
        source/cpp/Utils/ConsoleUtils.h
        )
add_executable(Logfit ${SOURCE_FILES})

if (CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_definitions(Logfit PUBLIC NDEBUG=1)
elseif (CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_definitions(Logfit PUBLIC NDEBUG=0)
endif ()

include_directories(${TBBROOT}/include ${CLROOT}/include)

link_directories(${TBBROOT}/build/linux_intel64_gcc_cc4.8_libc2.22_kernel4.4.103_release)
target_link_libraries(Logfit tbb OpenCL tbbmalloc_proxy)