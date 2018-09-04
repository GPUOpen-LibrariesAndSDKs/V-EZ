//
// Copyright (c) 2018 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#pragma once

#include "VEZ.h"

namespace vez
{
    class Instance;
    class PhysicalDevice;
    class Device;
    class Queue;
    class Swapchain;
    class CommandBuffer;
    class ShaderModule;
    class Buffer;
    class BufferView;
    class Image;
    class ImageView;
    class Fence;

    #define OBJECT_LOOKUP_DECLARATION(Handle, Impl)\
    extern vez::Impl* GetObjectImpl(Handle handle);\
    extern void AddObjectImpl(Handle handle, vez::Impl* object);\
    extern void RemoveObjectImpl(Handle handle);

    namespace ObjectLookup
    {
        OBJECT_LOOKUP_DECLARATION(VkInstance, Instance);
        OBJECT_LOOKUP_DECLARATION(VkPhysicalDevice, PhysicalDevice);
        OBJECT_LOOKUP_DECLARATION(VkDevice, Device);
        OBJECT_LOOKUP_DECLARATION(VkQueue, Queue);
        OBJECT_LOOKUP_DECLARATION(VkSwapchainKHR, Swapchain);
        OBJECT_LOOKUP_DECLARATION(VkCommandBuffer, CommandBuffer);
        OBJECT_LOOKUP_DECLARATION(VkShaderModule, ShaderModule);
        OBJECT_LOOKUP_DECLARATION(VkBuffer, Buffer);
        OBJECT_LOOKUP_DECLARATION(VkBufferView, BufferView);
        OBJECT_LOOKUP_DECLARATION(VkImage, Image);
        OBJECT_LOOKUP_DECLARATION(VkImageView, ImageView);
        OBJECT_LOOKUP_DECLARATION(VkFence, Fence);
    }    
}