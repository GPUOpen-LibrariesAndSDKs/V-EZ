//
// Copyright (c) 2017 Advanced Micro Devices, Inc. All rights reserved.
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
//
// @author Sean O'Connell (Sean.OConnell@amd.com)
//
#include "VEZ_Loader.h"

#ifdef VKEZ_NO_PROTOTYPES

#define ENTRY_POINT_DEFINITION(name) PFN_##name name = nullptr

ENTRY_POINT_DEFINITION(vkCreateInstance);
ENTRY_POINT_DEFINITION(vkDestroyInstance);
ENTRY_POINT_DEFINITION(vkEnumerateInstanceExtensionProperties);
ENTRY_POINT_DEFINITION(vkEnumerateInstanceLayerProperties);
ENTRY_POINT_DEFINITION(vkCreateSurface);
ENTRY_POINT_DEFINITION(vkDestroySurface);
ENTRY_POINT_DEFINITION(vkEnumeratePhysicalDevices);
ENTRY_POINT_DEFINITION(vkGetPhysicalDeviceProperties);
ENTRY_POINT_DEFINITION(vkGetPhysicalDeviceFeatures);
ENTRY_POINT_DEFINITION(vkGetPhysicalDeviceFormatProperties);
ENTRY_POINT_DEFINITION(vkGetPhysicalDeviceImageFormatProperties);
ENTRY_POINT_DEFINITION(vkGetPhysicalDeviceQueueFamilyProperties);
ENTRY_POINT_DEFINITION(vkGetPhysicalDeviceSurfaceFormats);
ENTRY_POINT_DEFINITION(vkGetPhysicalDevicePresentSupport);
ENTRY_POINT_DEFINITION(vkEnumerateDeviceExtensionProperties);
ENTRY_POINT_DEFINITION(vkEnumerateDeviceLayerProperties);
ENTRY_POINT_DEFINITION(vkCreateDevice);
ENTRY_POINT_DEFINITION(vkDestroyDevice);
ENTRY_POINT_DEFINITION(vkDeviceWaitIdle);
ENTRY_POINT_DEFINITION(vkDeviceSetVSync);
ENTRY_POINT_DEFINITION(vkGetDeviceQueue);
ENTRY_POINT_DEFINITION(vkGetDeviceGraphicsQueue);
ENTRY_POINT_DEFINITION(vkGetDeviceComputeQueue);
ENTRY_POINT_DEFINITION(vkGetDeviceTransferQueue);
ENTRY_POINT_DEFINITION(vkQueueSubmit);
ENTRY_POINT_DEFINITION(vkQueuePresent);
ENTRY_POINT_DEFINITION(vkQueueWaitIdle);
ENTRY_POINT_DEFINITION(vkDestroyFence);
ENTRY_POINT_DEFINITION(vkGetFenceStatus);
ENTRY_POINT_DEFINITION(vkWaitForFences);
ENTRY_POINT_DEFINITION(vkDestroySemaphore);
ENTRY_POINT_DEFINITION(vkCreateEvent);
ENTRY_POINT_DEFINITION(vkDestroyEvent);
ENTRY_POINT_DEFINITION(vkGetEventStatus);
ENTRY_POINT_DEFINITION(vkSetEvent);
ENTRY_POINT_DEFINITION(vkResetEvent);
ENTRY_POINT_DEFINITION(vkCreateQueryPool);
ENTRY_POINT_DEFINITION(vkDestroyQueryPool);
ENTRY_POINT_DEFINITION(vkGetQueryPoolResults);
ENTRY_POINT_DEFINITION(vkCreateShaderModule);
ENTRY_POINT_DEFINITION(vkDestroyShaderModule);
ENTRY_POINT_DEFINITION(vkGetShaderModuleInfoLog);
ENTRY_POINT_DEFINITION(vkCreateGraphicsPipeline);
ENTRY_POINT_DEFINITION(vkCreateComputePipeline);
ENTRY_POINT_DEFINITION(vkDestroyPipeline);
ENTRY_POINT_DEFINITION(vkEnumeratePipelineResources);
ENTRY_POINT_DEFINITION(vkGetPipelineResource);
ENTRY_POINT_DEFINITION(vkCreateVertexInputFormat);
ENTRY_POINT_DEFINITION(vkDestroyVertexInputFormat);
ENTRY_POINT_DEFINITION(vkCreateSampler);
ENTRY_POINT_DEFINITION(vkDestroySampler);
ENTRY_POINT_DEFINITION(vkCreateBuffer);
ENTRY_POINT_DEFINITION(vkDestroyBuffer);
ENTRY_POINT_DEFINITION(vkBufferSubData);
ENTRY_POINT_DEFINITION(vkMapBuffer);
ENTRY_POINT_DEFINITION(vkUnmapBuffer);
ENTRY_POINT_DEFINITION(vkFlushMappedBufferRanges);
ENTRY_POINT_DEFINITION(vkInvalidateMappedBufferRanges);
ENTRY_POINT_DEFINITION(vkCreateBufferView);
ENTRY_POINT_DEFINITION(vkDestroyBufferView);
ENTRY_POINT_DEFINITION(vkCreateImage);
ENTRY_POINT_DEFINITION(vkDestroyImage);
ENTRY_POINT_DEFINITION(vkImageSubData);
ENTRY_POINT_DEFINITION(vkCreateImageView);
ENTRY_POINT_DEFINITION(vkDestroyImageView);
ENTRY_POINT_DEFINITION(vkCreateFramebuffer);
ENTRY_POINT_DEFINITION(vkDestroyFramebuffer);
ENTRY_POINT_DEFINITION(vkAllocateCommandBuffers);
ENTRY_POINT_DEFINITION(vkFreeCommandBuffers);
ENTRY_POINT_DEFINITION(vkBeginCommandBuffer);
ENTRY_POINT_DEFINITION(vkEndCommandBuffer);
ENTRY_POINT_DEFINITION(vkResetCommandBuffer);
ENTRY_POINT_DEFINITION(vkCmdBeginRenderPass);
ENTRY_POINT_DEFINITION(vkCmdNextSubpass);
ENTRY_POINT_DEFINITION(vkCmdEndRenderPass);
ENTRY_POINT_DEFINITION(vkCmdBindPipeline);
ENTRY_POINT_DEFINITION(vkCmdPushConstants);
ENTRY_POINT_DEFINITION(vkCmdBindBuffer);
ENTRY_POINT_DEFINITION(vkCmdBindBufferView);
ENTRY_POINT_DEFINITION(vkCmdBindImageView);
ENTRY_POINT_DEFINITION(vkCmdBindSampler);
ENTRY_POINT_DEFINITION(vkCmdBindVertexBuffers);
ENTRY_POINT_DEFINITION(vkCmdBindIndexBuffer);
ENTRY_POINT_DEFINITION(vkCmdSetVertexInputFormat);
ENTRY_POINT_DEFINITION(vkCmdSetViewportState);
ENTRY_POINT_DEFINITION(vkCmdSetInputAssemblyState);
ENTRY_POINT_DEFINITION(vkCmdSetRasterizationState);
ENTRY_POINT_DEFINITION(vkCmdSetMultisampleState);
ENTRY_POINT_DEFINITION(vkCmdSetDepthStencilState);
ENTRY_POINT_DEFINITION(vkCmdSetColorBlendState);
ENTRY_POINT_DEFINITION(vkCmdSetViewport);
ENTRY_POINT_DEFINITION(vkCmdSetScissor);
ENTRY_POINT_DEFINITION(vkCmdSetLineWidth);
ENTRY_POINT_DEFINITION(vkCmdSetDepthBias);
ENTRY_POINT_DEFINITION(vkCmdSetBlendConstants);
ENTRY_POINT_DEFINITION(vkCmdSetDepthBounds);
ENTRY_POINT_DEFINITION(vkCmdSetStencilCompareMask);
ENTRY_POINT_DEFINITION(vkCmdSetStencilWriteMask);
ENTRY_POINT_DEFINITION(vkCmdSetStencilReference);
ENTRY_POINT_DEFINITION(vkCmdDraw);
ENTRY_POINT_DEFINITION(vkCmdDrawIndexed);
ENTRY_POINT_DEFINITION(vkCmdDrawIndirect);
ENTRY_POINT_DEFINITION(vkCmdDrawIndexedIndirect);
ENTRY_POINT_DEFINITION(vkCmdDispatch);
ENTRY_POINT_DEFINITION(vkCmdDispatchIndirect);
ENTRY_POINT_DEFINITION(vkCmdCopyBuffer);
ENTRY_POINT_DEFINITION(vkCmdCopyImage);
ENTRY_POINT_DEFINITION(vkCmdBlitImage);
ENTRY_POINT_DEFINITION(vkCmdCopyBufferToImage);
ENTRY_POINT_DEFINITION(vkCmdCopyImageToBuffer);
ENTRY_POINT_DEFINITION(vkCmdUpdateBuffer);
ENTRY_POINT_DEFINITION(vkCmdFillBuffer);
ENTRY_POINT_DEFINITION(vkCmdClearColorImage);
ENTRY_POINT_DEFINITION(vkCmdClearDepthStencilImage);
ENTRY_POINT_DEFINITION(vkCmdClearAttachments);
ENTRY_POINT_DEFINITION(vkCmdResolveImage);
ENTRY_POINT_DEFINITION(vkCmdSetEvent);
ENTRY_POINT_DEFINITION(vkCmdResetEvent);

ENTRY_POINT_DEFINITION(vkCreateDebugReportCallbackEXT);
ENTRY_POINT_DEFINITION(vkDestroyDebugReportCallbackEXT);
ENTRY_POINT_DEFINITION(vkDebugReportMessageEXT);

static void* LoadVulkanEZLibrary()
{
    return ::LoadLibrary("VulkanEZ.dll");
}

template <typename T>
inline T GetFunctionAddress(void* lib, const char *name)
{
    return reinterpret_cast<T>(::GetProcAddress((HMODULE)lib, name));
}

VkResult InitVulkanEZ()
{
    // Load the VulkanEZ inary from disk.
    auto lib = LoadVulkanEZLibrary();
    if (!lib) return VK_ERROR_INITIALIZATION_FAILED;

    // Utility macros.
    #define STRINGIFYBASE(x) #x
    #define STRINGIFY(x) STRINGIFYBASE(x)
    #define GET_ENTRY_POINT(x) x = GetFunctionAddress<decltype(x)>(lib, STRINGIFY(x))
    
    // Get entry points for all the API functions.
    GET_ENTRY_POINT(vkCreateInstance);
    GET_ENTRY_POINT(vkDestroyInstance);
    GET_ENTRY_POINT(vkEnumerateInstanceExtensionProperties);
    GET_ENTRY_POINT(vkEnumerateInstanceLayerProperties);
    GET_ENTRY_POINT(vkCreateSurface);
    GET_ENTRY_POINT(vkDestroySurface);
    GET_ENTRY_POINT(vkEnumeratePhysicalDevices);
    GET_ENTRY_POINT(vkGetPhysicalDeviceProperties);
    GET_ENTRY_POINT(vkGetPhysicalDeviceFeatures);
    GET_ENTRY_POINT(vkGetPhysicalDeviceFormatProperties);
    GET_ENTRY_POINT(vkGetPhysicalDeviceImageFormatProperties);
    GET_ENTRY_POINT(vkGetPhysicalDeviceQueueFamilyProperties);
    GET_ENTRY_POINT(vkGetPhysicalDeviceSurfaceFormats);
    GET_ENTRY_POINT(vkGetPhysicalDevicePresentSupport);
    GET_ENTRY_POINT(vkEnumerateDeviceExtensionProperties);
    GET_ENTRY_POINT(vkEnumerateDeviceLayerProperties);
    GET_ENTRY_POINT(vkCreateDevice);
    GET_ENTRY_POINT(vkDestroyDevice);
    GET_ENTRY_POINT(vkDeviceWaitIdle);
    GET_ENTRY_POINT(vkDeviceSetVSync);
    GET_ENTRY_POINT(vkGetDeviceQueue);
    GET_ENTRY_POINT(vkGetDeviceGraphicsQueue);
    GET_ENTRY_POINT(vkGetDeviceComputeQueue);
    GET_ENTRY_POINT(vkGetDeviceTransferQueue);
    GET_ENTRY_POINT(vkQueueSubmit);
    GET_ENTRY_POINT(vkQueuePresent);
    GET_ENTRY_POINT(vkQueueWaitIdle);
    GET_ENTRY_POINT(vkDestroyFence);
    GET_ENTRY_POINT(vkGetFenceStatus);
    GET_ENTRY_POINT(vkWaitForFences);
    GET_ENTRY_POINT(vkDestroySemaphore);
    GET_ENTRY_POINT(vkCreateEvent);
    GET_ENTRY_POINT(vkDestroyEvent);
    GET_ENTRY_POINT(vkGetEventStatus);
    GET_ENTRY_POINT(vkSetEvent);
    GET_ENTRY_POINT(vkResetEvent);
    GET_ENTRY_POINT(vkCreateQueryPool);
    GET_ENTRY_POINT(vkDestroyQueryPool);
    GET_ENTRY_POINT(vkGetQueryPoolResults);
    GET_ENTRY_POINT(vkCreateShaderModule);
    GET_ENTRY_POINT(vkDestroyShaderModule);
    GET_ENTRY_POINT(vkGetShaderModuleInfoLog);
    GET_ENTRY_POINT(vkCreateGraphicsPipeline);
    GET_ENTRY_POINT(vkCreateComputePipeline);
    GET_ENTRY_POINT(vkDestroyPipeline);
    GET_ENTRY_POINT(vkEnumeratePipelineResources);
    GET_ENTRY_POINT(vkGetPipelineResource);
    GET_ENTRY_POINT(vkCreateVertexInputFormat);
    GET_ENTRY_POINT(vkDestroyVertexInputFormat);
    GET_ENTRY_POINT(vkCreateSampler);
    GET_ENTRY_POINT(vkDestroySampler);
    GET_ENTRY_POINT(vkCreateBuffer);
    GET_ENTRY_POINT(vkDestroyBuffer);
    GET_ENTRY_POINT(vkBufferSubData);
    GET_ENTRY_POINT(vkMapBuffer);
    GET_ENTRY_POINT(vkUnmapBuffer);
    GET_ENTRY_POINT(vkFlushMappedBufferRanges);
    GET_ENTRY_POINT(vkInvalidateMappedBufferRanges);
    GET_ENTRY_POINT(vkCreateBufferView);
    GET_ENTRY_POINT(vkDestroyBufferView);
    GET_ENTRY_POINT(vkCreateImage);
    GET_ENTRY_POINT(vkDestroyImage);
    GET_ENTRY_POINT(vkImageSubData);
    GET_ENTRY_POINT(vkCreateImageView);
    GET_ENTRY_POINT(vkDestroyImageView);
    GET_ENTRY_POINT(vkCreateFramebuffer);
    GET_ENTRY_POINT(vkDestroyFramebuffer);
    GET_ENTRY_POINT(vkAllocateCommandBuffers);
    GET_ENTRY_POINT(vkFreeCommandBuffers);
    GET_ENTRY_POINT(vkBeginCommandBuffer);
    GET_ENTRY_POINT(vkEndCommandBuffer);
    GET_ENTRY_POINT(vkResetCommandBuffer);
    GET_ENTRY_POINT(vkCmdBeginRenderPass);
    GET_ENTRY_POINT(vkCmdNextSubpass);
    GET_ENTRY_POINT(vkCmdEndRenderPass);
    GET_ENTRY_POINT(vkCmdExecuteCommands);
    GET_ENTRY_POINT(vkCmdBindPipeline);
    GET_ENTRY_POINT(vkCmdPushConstants);
    GET_ENTRY_POINT(vkCmdBindBuffer);
    GET_ENTRY_POINT(vkCmdBindBufferView);
    GET_ENTRY_POINT(vkCmdBindImageView);
    GET_ENTRY_POINT(vkCmdBindSampler);
    GET_ENTRY_POINT(vkCmdBindVertexBuffers);
    GET_ENTRY_POINT(vkCmdBindIndexBuffer);
    GET_ENTRY_POINT(vkCmdSetVertexInputFormat);
    GET_ENTRY_POINT(vkCmdSetViewportState);
    GET_ENTRY_POINT(vkCmdSetInputAssemblyState);
    GET_ENTRY_POINT(vkCmdSetRasterizationState);
    GET_ENTRY_POINT(vkCmdSetMultisampleState);
    GET_ENTRY_POINT(vkCmdSetDepthStencilState);
    GET_ENTRY_POINT(vkCmdSetColorBlendState);
    GET_ENTRY_POINT(vkCmdSetViewport);
    GET_ENTRY_POINT(vkCmdSetScissor);
    GET_ENTRY_POINT(vkCmdSetLineWidth);
    GET_ENTRY_POINT(vkCmdSetDepthBias);
    GET_ENTRY_POINT(vkCmdSetBlendConstants);
    GET_ENTRY_POINT(vkCmdSetDepthBounds);
    GET_ENTRY_POINT(vkCmdSetStencilCompareMask);
    GET_ENTRY_POINT(vkCmdSetStencilWriteMask);
    GET_ENTRY_POINT(vkCmdSetStencilReference);
    GET_ENTRY_POINT(vkCmdDraw);
    GET_ENTRY_POINT(vkCmdDrawIndexed);
    GET_ENTRY_POINT(vkCmdDrawIndirect);
    GET_ENTRY_POINT(vkCmdDrawIndexedIndirect);
    GET_ENTRY_POINT(vkCmdDispatch);
    GET_ENTRY_POINT(vkCmdDispatchIndirect);
    GET_ENTRY_POINT(vkCmdCopyBuffer);
    GET_ENTRY_POINT(vkCmdCopyImage);
    GET_ENTRY_POINT(vkCmdBlitImage);
    GET_ENTRY_POINT(vkCmdCopyBufferToImage);
    GET_ENTRY_POINT(vkCmdCopyImageToBuffer);
    GET_ENTRY_POINT(vkCmdUpdateBuffer);
    GET_ENTRY_POINT(vkCmdFillBuffer);
    GET_ENTRY_POINT(vkCmdClearColorImage);
    GET_ENTRY_POINT(vkCmdClearDepthStencilImage);
    GET_ENTRY_POINT(vkCmdClearAttachments);
    GET_ENTRY_POINT(vkCmdResolveImage);
    GET_ENTRY_POINT(vkCmdSetEvent);
    GET_ENTRY_POINT(vkCmdResetEvent);

    GET_ENTRY_POINT(vkCreateDebugReportCallbackEXT);
    GET_ENTRY_POINT(vkDestroyDebugReportCallbackEXT);
    GET_ENTRY_POINT(vkDebugReportMessageEXT);

    return VK_SUCCESS;
}

#endif