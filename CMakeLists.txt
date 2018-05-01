#######################################################################################################################
#
#  Copyright (c) 2017-2018 Advanced Micro Devices, Inc. All Rights Reserved.
#
#  Permission is hereby granted, free of charge, to any person obtaining a copy
#  of this software and associated documentation files (the "Software"), to deal
#  in the Software without restriction, including without limitation the rights
#  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#  copies of the Software, and to permit persons to whom the Software is
#  furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice shall be included in all
#  copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
#  SOFTWARE.
# #######################################################################################################################

cmake_minimum_required(VERSION 3.8)
project(VEZ_Samples VERSION 1.1.0 LANGUAGES CXX)

set(OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Bin/x86_64")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIRECTORY}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIRECTORY}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}")

set(LIB_SUFFIX "")

if(WIN32)
    set(PLATFORM_LIB win64)
    file(COPY ./Samples/3rdParty/Assimp/bin/win64/assimp-vc140-mt.dll DESTINATION ${OUTPUT_DIRECTORY})
    file(COPY ./Lib/win64/VEZ.dll DESTINATION ${OUTPUT_DIRECTORY})
elseif(UNIX)
    set(PLATFORM_LIB linux64)
    file(COPY Lib/linux64/libVEZ.so DESTINATION ${OUTPUT_DIRECTORY})
endif(WIN32)

include_directories(
    ./Include
    ./Samples/3rdParty/assimp/include
    ./Samples/3rdParty/glfw/include
    ./Samples/3rdParty/glm/include
    ./Samples/3rdParty/stb/include
    ./Samples/3rdParty/vulkan/include
)

link_directories(
    ./Lib/${PLATFORM_LIB}
    ./Samples/3rdParty/assimp/lib/${PLATFORM_LIB}
    ./Samples/3rdParty/glfw/lib/${PLATFORM_LIB}
    ./Samples/3rdParty/vulkan/lib/${PLATFORM_LIB}
)

add_subdirectory(Samples/MultiWindow)
add_subdirectory(Samples/OcclusionCulling)
add_subdirectory(Samples/ShadowMapping)
add_subdirectory(Samples/SimpleCompute)
add_subdirectory(Samples/SimpleQuad)
add_subdirectory(Samples/SoftwareRasterization)
add_subdirectory(Samples/Subpasses)
add_subdirectory(Samples/VulkanMemory)

