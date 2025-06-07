# CoreTaskOptimizer

Adaptive core CPU optimizer for Android, designed for use as a [Magisk](https://github.com/topjohnwu/Magisk) module.  
This project provides a universal, cross-architecture binary for efficient task management, core affinity, and log management on rooted Android devices.

---

## Features

- **Universal ABI Support**: Automatically builds for `arm64-v8a`, `armeabi-v7a`, `x86`, and `x86_64` (Android NDK supported).
- **Magisk Module Ready**: Place binaries in the `libs/$ABI/task_optimizer` path for direct use in Magisk modules.
- **CMake-based Build**: Simple and portable build process for Android and Linux environments.
- **Automatic CI/CD**: GitHub Actions workflow automatically rebuilds and deploys binaries on changes to `src/main.cpp`.
- **Customizable Service**: Comes with service scripts and configuration files for advanced users.

---

## Directory Structure

```
.
├── .github/           # GitHub Actions workflows
├── common/            # Common headers/scripts (if any)
├── libs/              # Output: ABI-specific binaries (e.g., libs/arm64-v8a/task_optimizer)
├── src/               # Source code (main.cpp, etc.)
├── CMakeLists.txt     # CMake build script
├── build.sh           # Optional build helper script
├── customize.sh       # Customization script
├── service.sh         # Service starter script
├── uninstall.sh       # Uninstall helper script
├── module.prop        # Magisk module property file
├── LICENSE            # License file (MIT)
└── README.md
```

---

## Building

### Prerequisites

- [Android NDK](https://developer.android.com/ndk/downloads)
- [CMake](https://cmake.org/download/)
- A POSIX-compatible shell (Linux/macOS/WSL)

### Build Manually

```sh
# For a specific ABI:
ABI=arm64-v8a
mkdir -p build/$ABI
cd build/$ABI
cmake -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
      -DANDROID_ABI=$ABI \
      -DANDROID_PLATFORM=android-21 \
      ../..
cmake --build .
cd ../..
# Output: libs/$ABI/task_optimizer
```

Or use the provided workflow/build scripts to automate all ABIs.

---

## Continuous Integration

- **GitHub Actions**:  
  On changes to `src/main.cpp`, the workflow will:
  1. Build for all ABIs.
  2. Copy binaries to `libs/$ABI/task_optimizer`.
  3. Upload `libs/` as an artifact for easy download.

---

## Usage in Magisk Module

Copy the ABI-specific binaries from `libs/` into your Magisk module under the appropriate path (e.g. `system/bin` or similar as required by your module).

Example:
```
libs/arm64-v8a/task_optimizer       # For 64-bit ARM
libs/armeabi-v7a/task_optimizer    # For 32-bit ARM
libs/x86/task_optimizer            # For x86
libs/x86_64/task_optimizer         # For x86_64
```

---

## License

This project is licensed under the [MIT License](LICENSE).

---

## Contributing

Pull requests and issues are welcome!  
Please use [issues](https://github.com/c0d3h01/CoreTaskOptimizer/issues) for bug reports or feature requests.

---

## Author

**Harshal Sawant**  
[GitHub](https://github.com/c0d3h01) | [Buy me a coffee](https://www.buymeacoffee.com/c0d3h01)
