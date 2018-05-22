cmake_minimum_required(VERSION 3.2)
set (CMAKE_CXX_STANDARD 11)

project(BarnesLogfit)

set(MODE debug)

set(BOOST_ROOT bin/boost_1_66_0)
set(TBBROOT $ENV{TBBROOT})
set(CLROOT /opt/intel/opencl-sdk)
set(MAIN_FILE source/cpp/main.cpp)
set(SOURCE_FILES source/cpp/main.cpp source/cpp/ConsoleUtils.h source/cpp/DataStructures.h source/cpp/BarnesHut.h
            source/cpp/Body.h source/cpp/Scheduler.h source/cpp/LogFit.h)
add_executable(Logfit ${SOURCE_FILES})


include_directories(${TBBROOT}/include ${CLROOT}/include ${BOOST_ROOT}/boost)

link_directories(${TBBROOT}/build/linux_intel64_gcc_cc4.8_libc2.22_kernel4.4.103_${MODE})
target_link_libraries(Logfit tbb_debug OpenCL)
