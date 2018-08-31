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
    Compiler/GLSLCompiler.h
    Compiler/ResourceLimits.cpp
    Compiler/ResourceLimits.h
    Compiler/SPIRVReflection.cpp
    Compiler/SPIRVReflection.h
)

set(CORE_SOURCES
    Core/Buffer.cpp
    Core/Buffer.h
    Core/BufferView.cpp
    Core/BufferView.h
    Core/CommandBuffer.cpp
    Core/CommandBuffer.h
    Core/CommandPool.cpp
    Core/CommandPool.h
    Core/DescriptorPool.cpp
    Core/DescriptorPool.h
    Core/DescriptorSetLayout.cpp
    Core/DescriptorSetLayout.h
    Core/DescriptorSetLayoutCache.cpp
    Core/DescriptorSetLayoutCache.h
    Core/Device.cpp
    Core/Device.h
    Core/Fence.h
    Core/Framebuffer.cpp
    Core/Framebuffer.h
    Core/GraphicsState.cpp
    Core/GraphicsState.h
    Core/Image.cpp
    Core/Image.h
    Core/ImageView.cpp
    Core/ImageView.h
    Core/Instance.cpp
    Core/Instance.h
    Core/PhysicalDevice.h
    Core/Pipeline.cpp
    Core/Pipeline.h
    Core/PipelineBarriers.cpp
    Core/PipelineBarriers.h
    Core/PipelineCache.cpp
    Core/PipelineCache.h
    Core/Queue.cpp
    Core/Queue.h
    Core/RenderPassCache.cpp
    Core/RenderPassCache.h
    Core/ResourceBindings.cpp
    Core/ResourceBindings.h
    Core/ShaderModule.cpp
    Core/ShaderModule.h
    Core/StreamDecoder.cpp
    Core/StreamDecoder.h
    Core/StreamEncoder.cpp
    Core/StreamEncoder.h
    Core/Swapchain.cpp
    Core/Swapchain.h
    Core/SyncPrimitivesPool.cpp
    Core/SyncPrimitivesPool.h
    Core/VertexInputFormat.cpp
    Core/VertexInputFormat.h
)

set(UTILITY_SOURCES
    Utility/Macros.h
    Utility/MemoryStream.cpp
    Utility/MemoryStream.h
    Utility/ObjectLookup.cpp
    Utility/ObjectLookup.h
    Utility/SpinLock.h
    Utility/ThreadPool.cpp
    Utility/ThreadPool.h
    Utility/VkHelpers.h
)

set(ROOT_SOURCES
    VEZ.def
    VEZ.cpp
    VEZ.h
    VEZ_ext.cpp
    VEZ_ext.h
)

source_group("Compiler" FILES ${COMPILER_SOURCES})
source_group("Core" FILES ${CORE_SOURCES})
source_group("Utility" FILES ${UTILITY_SOURCES})
source_group("" FILES ${ROOT_SOURCES})

set(DEBUG_LIB_SUFFIX "")
if(WIN32)
    set(DEBUG_LIB_SUFFIX "d")
endif(WIN32)

add_library(VEZ SHARED ${COMPILER_SOURCES} ${CORE_SOURCES} ${UTILITY_SOURCES} ${ROOT_SOURCES})

include_directories(
    ${VEZ_ROOT_DIR}/Source
    ${VEZ_ROOT_DIR}/Libs/VulkanMemoryAllocator/include
    ${VEZ_ROOT_DIR}/External/SPIRV-Cross
    ${VEZ_ROOT_DIR}/External/glslang
    Vulkan::Vulkan
)

target_link_libraries(VEZ
    PUBLIC Vulkan::Vulkan
    PRIVATE glslang
    PRIVATE OGLCompiler
    PRIVATE OSDependent
    PRIVATE SPIRV
    PRIVATE SPVRemapper
    PRIVATE spirv-cross-core
    PRIVATE spirv-cross-glsl
)

if(NOT WIN32)
    target_link_libraries(VEZ
        PRIVATE dl
        PRIVATE pthread
    )
endif(NOT WIN32)

target_compile_features(VEZ PRIVATE cxx_std_14)

target_compile_definitions(VEZ PUBLIC _CRT_SECURE_NO_WARNINGS)
