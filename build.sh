#!/bin/bash

# List of target ABIs
ABIS=("arm64-v8a" "armeabi-v7a" "x86" "x86_64")

# Clean previous builds
echo "Cleaning previous builds..."
rm -rf build
rm -rf libs
mkdir -p build
mkdir -p libs

# Build for each ABI
for abi in "${ABIS[@]}"; do
    echo "Building for ABI: $abi"
    mkdir -p build/$abi
    cd build/$abi
    cmake -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
          -DANDROID_PLATFORM=android-21 \
          -DANDROID_ABI=$abi \
          ../..
    cmake --build .
    cd ../..
done

# Copy binaries to the libs directory
echo "Copying binaries to libs directory..."
for abi in "${ABIS[@]}"; do
    mkdir -p libs/$abi
    cp build/$abi/build_$abi/core_task_optimizer_$abi libs/$abi/task_optimizer
done

echo "Build and copy completed for all ABIs."