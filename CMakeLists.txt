cmake_minimum_required(VERSION 3.10)

# Project name and version
project(CoreTaskOptimizer VERSION 1.0 LANGUAGES CXX)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Android NDK toolchain (ensure the NDK path is set in your environment)
set(CMAKE_TOOLCHAIN_FILE ${ANDROID_NDK}/build/cmake/android.toolchain.cmake)

# Specify the Android platform (minimum API level)
set(ANDROID_PLATFORM android-21)

# Enable static linking
set(BUILD_SHARED_LIBS OFF)

# Add the source files
set(SOURCES
    src/main.cpp
)

# Function to configure and build for a specific ABI
function(build_for_abi ABI)
    message(STATUS "Configuring for ABI: ${ABI}")

    # Create a build directory for the ABI
    set(BINARY_DIR "${CMAKE_BINARY_DIR}/build_${ABI}")
    file(MAKE_DIRECTORY ${BINARY_DIR})

    # Configure the build for the current ABI
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_DIR})

    # Set the ABI-specific flags
    set(CMAKE_ANDROID_ARCH_ABI ${ABI})

    # Add the executable
    add_executable(core_task_optimizer_${ABI} ${SOURCES})

    # Link statically
    target_link_libraries(core_task_optimizer_${ABI} -static-libstdc++ -static-libgcc)

    # Install the binary to the appropriate directory
    install(TARGETS core_task_optimizer_${ABI}
            DESTINATION ${CMAKE_INSTALL_PREFIX}/libs/${ABI})
endfunction()

# List of target ABIs
set(TARGET_ABIS arm64-v8a armeabi-v7a x86 x86_64)

# Build for each ABI
foreach(abi IN LISTS TARGET_ABIS)
    build_for_abi(${abi})
endforeach()

# Output the build configuration
message(STATUS "Building for ABIs: ${TARGET_ABIS}")