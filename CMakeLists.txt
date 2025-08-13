cmake_minimum_required(VERSION 3.18.1)
project(CoreTaskOptimizer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(ANDROID)
    set(ANDROID_STL c++_shared)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -flto")

add_executable(task_optimizer src/main.cpp)

set_target_properties(task_optimizer PROPERTIES
    OUTPUT_NAME "task_optimizer"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)