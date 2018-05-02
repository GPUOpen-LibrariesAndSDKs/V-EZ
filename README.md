# V-EZ

V-EZ is a cross-platform (Windows and Linux) wrapper intended to alleviate the inherent complexity and application responsibility of using the Vulkan API. V-EZ attempts to bridge the gap between traditional graphics APIs and Vulkan by providing similar semantics to Vulkan while lowering the barrier to entry and providing an easier to use API.

<img src="https://github.com/GPUOpen-LibrariesAndSDKs/V-EZ/blob/master/Docs/img/VulkanAPI.PNG" />

<img src="https://github.com/GPUOpen-LibrariesAndSDKs/V-EZ/blob/master/Docs/img/V-EZ.PNG" />

## Documentation

The documentation for V-EZ can be found [here](https://gpuopen-librariesandsdks.github.io/V-EZ/)
## Prerequisites

* Windows 7, 8.1, 10 64-bit
* Linux 64-bit (tested on Fedora, Ubuntu)
* Visual Studio&reg; 2015 and later
* GCC 6 and later
* CMake 3.8 and later
* LunarG Vulkan SDK 1.1.70

## Hardware Support

V-EZ is not hardware vendor specific and should work on non-AMD hardware.

## Build Instructions

### Windows

* Generate Visual Studio 2015 Solution

cmake -G "Visual Studio 14 2015 Win64"

### Linux

* Generate the Makefile and build

mkdir Build

cd Build

cmake -DCMAKE_BUILD_TYPE=(Release|Debug) ../

make


