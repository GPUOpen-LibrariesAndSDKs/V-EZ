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
project(VEZ VERSION 1.1.0 LANGUAGES CXX)

find_package(Vulkan REQUIRED)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)

option(VEZ_COMPILE_SAMPLES "Compile sample demos" OFF)
set(VEZ_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Bin/x86_64" CACHE PATH "directory where .lib and .dlls are placed")

set(VEZ_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(OUTPUT_DIRECTORY "${VEZ_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIRECTORY}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIRECTORY}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    
include(External/SPIRV-Cross.cmake)
set_target_properties(spirv-cross-core PROPERTIES FOLDER External/SPIRV-Cross)
set_target_properties(spirv-cross-glsl PROPERTIES FOLDER External/SPIRV-Cross)

include(External/glslang.cmake)
set_target_properties(glslang PROPERTIES FOLDER External/glslang)
set_target_properties(OGLCompiler PROPERTIES FOLDER External/glslang)
set_target_properties(OSDependent PROPERTIES FOLDER External/glslang)
set_target_properties(SPIRV PROPERTIES FOLDER External/glslang)
set_target_properties(SPVRemapper PROPERTIES FOLDER External/glslang)

add_subdirectory(Source)

if(VEZ_COMPILE_SAMPLES)
    add_subdirectory(Samples)
endif()