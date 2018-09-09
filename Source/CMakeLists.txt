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

set(COMPILER_SOURCES
    Compiler/GLSLCompiler.cpp
    Compiler/ResourceLimits.cpp
    Compiler/SPIRVReflection.cpp
)

set(CORE_SOURCES
    Core/Buffer.cpp
    Core/BufferView.cpp
    Core/CommandBuffer.cpp
    Core/CommandPool.cpp
    Core/DescriptorPool.cpp
    Core/DescriptorSetLayout.cpp
    Core/DescriptorSetLayoutCache.cpp
    Core/Device.cpp
    Core/Framebuffer.cpp
    Core/GraphicsState.cpp
    Core/Image.cpp
    Core/ImageView.cpp
    Core/Instance.cpp
    Core/Pipeline.cpp
    Core/PipelineBarriers.cpp
    Core/PipelineCache.cpp
    Core/Queue.cpp
    Core/RenderPassCache.cpp
    Core/ResourceBindings.cpp
    Core/ShaderModule.cpp
    Core/StreamDecoder.cpp
    Core/StreamEncoder.cpp
    Core/Swapchain.cpp
    Core/SyncPrimitivesPool.cpp
    Core/VertexInputFormat.cpp
)

set(UTILITY_SOURCES
    Utility/MemoryStream.cpp
    Utility/ObjectLookup.cpp
    Utility/ThreadPool.cpp
)

set(ROOT_SOURCES
    VEZ.def
    VEZ.cpp
    VEZ_ext.cpp
)

source_group("Compiler" FILES ${COMPILER_SOURCES})
source_group("Core" FILES ${CORE_SOURCES})
source_group("Utility" FILES ${UTILITY_SOURCES})
source_group("" FILES ${ROOT_SOURCES})

set(DEBUG_LIB_SUFFIX "")
if(WIN32)
    set(DEBUG_LIB_SUFFIX "d")
endif(WIN32)

add_library(VEZ ${COMPILER_SOURCES} ${CORE_SOURCES} ${UTILITY_SOURCES} ${ROOT_SOURCES})

target_include_directories(VEZ
    PUBLIC ${CMAKE_SOURCE_DIR}/Source
    PUBLIC ${CMAKE_SOURCE_DIR}/External/SPIRV-Cross
    PUBLIC ${CMAKE_SOURCE_DIR}/Libs/VulkanMemoryAllocator/include
    PUBLIC ${CMAKE_SOURCE_DIR}/Libs/stb/include
)

target_link_libraries(VEZ
    Vulkan::Vulkan
    Threads::Threads
    glslang
    OGLCompiler
    OSDependent
    SPIRV
    SPVRemapper
    spirv-cross-core
    spirv-cross-glsl
)

set_property(TARGET VEZ PROPERTY CXX_STANDARD 14)
set_property(TARGET VEZ PROPERTY CXX_STANDARD_REQUIRED TRUE)

target_compile_definitions(VEZ PUBLIC _CRT_SECURE_NO_WARNINGS)
