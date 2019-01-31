cmake_minimum_required(VERSION 3.2)
set (CMAKE_CXX_STANDARD 11)

project(BarnesLogfit)

set(MODE debug)

set(TBBROOT $ENV{TBBROOT})
set(CLROOT /opt/AMDAPPSDK-3.0-0-Beta)
set(MAIN_FILE source/cpp/main.cpp)
set(SOURCE_FILES source/cpp/main.cpp)
add_executable(Logfit ${SOURCE_FILES})
add_definitions( "-Wno-ignored-attributes -Wno-format-security -O3 -DNDEBUG" )

include_directories(${TBBROOT}/include ${CLROOT}/include)

link_directories(${CLROOT}/lib/x86_64)
target_link_libraries(Logfit tbb OpenCL)
