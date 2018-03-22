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
#ifndef VEZ_LOADER_H_
#define VEZ_LOADER_H_

#include "VEZ.h"

#ifdef VKEZ_NO_PROTOTYPES

typedef VkResult(VKAPI_PTR *PFN_vkCreateInstance)(const VkInstanceCreateInfo* pCreateInfo,  VkInstance* pInstance);
typedef void(VKAPI_PTR *PFN_vkDestroyInstance)(VkInstance instance);
typedef VkResult(VKAPI_PTR *PFN_vkEnumerateInstanceExtensionProperties)(const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties);
typedef VkResult(VKAPI_PTR *PFN_vkEnumerateInstanceLayerProperties)(uint32_t* pPropertyCount, VkLayerProperties* pProperties);
typedef VkResult(VKAPI_PTR *PFN_vkCreateSurface)(VkInstance instance, const VkSurfaceCreateInfo* pCreateInfo, VkSurface* pSurface);
typedef void(VKAPI_PTR *PFN_vkDestroySurface)(VkInstance instance, VkSurface surface);
typedef VkResult(VKAPI_PTR *PFN_vkEnumeratePhysicalDevices)(VkInstance instance, uint32_t* pPhysicalDeviceCount, VkPhysicalDevice* pPhysicalDevices);
typedef void(VKAPI_PTR *PFN_vkGetPhysicalDeviceProperties)(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties* pProperties);
typedef void(VKAPI_PTR *PFN_vkGetPhysicalDeviceFeatures)(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures* pFeatures);
typedef void(VKAPI_PTR *PFN_vkGetPhysicalDeviceFormatProperties)(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties* pFormatProperties);
typedef VkResult(VKAPI_PTR *PFN_vkGetPhysicalDeviceImageFormatProperties)(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkImageFormatProperties* pImageFormatProperties);
typedef void(VKAPI_PTR *PFN_vkGetPhysicalDeviceQueueFamilyProperties)(VkPhysicalDevice physicalDevice, uint32_t* pQueueFamilyPropertyCount, VkQueueFamilyProperties* pQueueFamilyProperties);
typedef VkResult(VKAPI_PTR *PFN_vkGetPhysicalDeviceSurfaceFormats)(VkPhysicalDevice, VkSurface surface, uint32_t* pSurfaceFormatCount, VkSurfaceFormat* pSurfaceFormats);
typedef VkResult(VKAPI_PTR *PFN_vkGetPhysicalDevicePresentSupport)(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, VkSurface surface, VkBool32* pSupported);
typedef VkResult(VKAPI_PTR *PFN_vkEnumerateDeviceExtensionProperties)(VkPhysicalDevice physicalDevice, const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties);
typedef VkResult(VKAPI_PTR *PFN_vkEnumerateDeviceLayerProperties)(VkPhysicalDevice physicalDevice, uint32_t* pPropertyCount, VkLayerProperties* pProperties);
typedef VkResult(VKAPI_PTR *PFN_vkCreateDevice)(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, VkDevice* pDevice);
typedef void(VKAPI_PTR *PFN_vkDestroyDevice)(VkDevice device);
typedef VkResult(VKAPI_PTR *PFN_vkDeviceWaitIdle)(VkDevice device);
typedef VkResult(VKAPI_PTR *PFN_vkDeviceSetVSync)(VkDevice device, VkBool32 enabled);
typedef void(VKAPI_PTR *PFN_vkGetDeviceQueue)(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue* pQueue);
typedef void(VKAPI_PTR *PFN_vkGetDeviceGraphicsQueue)(VkDevice device, uint32_t queueIndex, VkQueue* pQueue);
typedef void(VKAPI_PTR *PFN_vkGetDeviceComputeQueue)(VkDevice device, uint32_t queueIndex, VkQueue* pQueue);
typedef void(VKAPI_PTR *PFN_vkGetDeviceTransferQueue)(VkDevice device, uint32_t queueIndex, VkQueue* pQueue);
typedef VkResult(VKAPI_PTR *PFN_vkQueueSubmit)(VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence* pFence);
typedef VkResult(VKAPI_PTR *PFN_vkQueuePresent)(VkQueue queue, const VkPresentInfo* pPresentInfo);
typedef VkResult(VKAPI_PTR *PFN_vkQueueWaitIdle)(VkQueue queue);
typedef void(VKAPI_PTR *PFN_vkDestroyFence)(VkDevice device, VkFence fence);
typedef VkResult(VKAPI_PTR *PFN_vkGetFenceStatus)(VkDevice device, VkFence fence);
typedef VkResult(VKAPI_PTR *PFN_vkWaitForFences)(VkDevice device, uint32_t fenceCount, const VkFence* pFences, VkBool32 waitAll, uint64_t timeout);
typedef void(VKAPI_PTR *PFN_vkDestroySemaphore)(VkDevice device, VkSemaphore semaphore);
typedef VkResult(VKAPI_PTR *PFN_vkCreateEvent)(VkDevice device, VkEvent* pEvent);
typedef void(VKAPI_PTR *PFN_vkDestroyEvent)(VkDevice device, VkEvent event);
typedef VkResult(VKAPI_PTR *PFN_vkGetEventStatus)(VkDevice device, VkEvent event);
typedef VkResult(VKAPI_PTR *PFN_vkSetEvent)(VkDevice device, VkEvent event);
typedef VkResult(VKAPI_PTR *PFN_vkResetEvent)(VkDevice device, VkEvent event);
typedef VkResult(VKAPI_PTR *PFN_vkCreateQueryPool)(VkDevice device, const VkQueryPoolCreateInfo* pCreateInfo, VkQueryPool* pQueryPool);
typedef void(VKAPI_PTR *PFN_vkDestroyQueryPool)(VkDevice device, VkQueryPool queryPool);
typedef VkResult(VKAPI_PTR *PFN_vkGetQueryPoolResults)(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, size_t dataSize, void* pData, VkDeviceSize stride, VkQueryResultFlags flags);
typedef VkResult(VKAPI_PTR *PFN_vkCreateShaderModule)(VkDevice device, const VkShaderModuleCreateInfo* pCreateInfo, VkShaderModule* pShaderModule);
typedef void(VKAPI_PTR *PFN_vkDestroyShaderModule)(VkDevice device, VkShaderModule shaderModule);
typedef void(VKAPI_PTR *PFN_vkGetShaderModuleInfoLog)(VkShaderModule shaderModule, uint32_t* pLength, char* pInfoLog);
typedef VkResult(VKAPI_PTR *PFN_vkCreateGraphicsPipeline)(VkDevice device, const VkGraphicsPipelineCreateInfo* pCreateInfo, VkPipeline* pPipeline);
typedef VkResult(VKAPI_PTR *PFN_vkCreateComputePipeline)(VkDevice device, const VkComputePipelineCreateInfo* pCreateInfo, VkPipeline* pPipeline);
typedef void(VKAPI_PTR *PFN_vkDestroyPipeline)(VkDevice device, VkPipeline pipeline);
typedef VkResult(VKAPI_PTR *PFN_vkEnumeratePipelineResources)(VkPipeline pipeline, uint32_t* pResourceCount, VkPipelineResource* pResources);
typedef VkResult(VKAPI_PTR *PFN_vkGetPipelineResource)(VkPipeline pipeline, const char* name, VkPipelineResource* pResource);
typedef VkResult(VKAPI_PTR *PFN_vkCreateVertexInputFormat)(VkDevice device, const VkVertexInputFormatCreateInfo* pCreateInfo, VkVertexInputFormat* pFormat);
typedef void(VKAPI_PTR *PFN_vkDestroyVertexInputFormat)(VkDevice device, VkVertexInputFormat format);
typedef VkResult(VKAPI_PTR *PFN_vkCreateSampler)(VkDevice device, const VkSamplerCreateInfo* pCreateInfo, VkSampler* pSampler);
typedef void(VKAPI_PTR *PFN_vkDestroySampler)(VkDevice device, VkSampler sampler);
typedef VkResult(VKAPI_PTR *PFN_vkCreateBuffer)(VkDevice device, VkMemoryFlags memFlags, const VkBufferCreateInfo* pCreateInfo, VkBuffer* pBuffer);
typedef void(VKAPI_PTR *PFN_vkDestroyBuffer)(VkDevice device, VkBuffer buffer);
typedef VkResult(VKAPI_PTR *PFN_vkBufferSubData)(VkDevice device, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, const void* pData);
typedef VkResult(VKAPI_PTR *PFN_vkMapBuffer)(VkDevice device, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, void** ppData);
typedef void(VKAPI_PTR *PFN_vkUnmapBuffer)(VkDevice device, VkBuffer buffer);
typedef VkResult(VKAPI_PTR *PFN_vkFlushMappedBufferRanges)(VkDevice device, uint32_t bufferRangeCount, const VkMappedBufferRange* pBufferRanges);
typedef VkResult(VKAPI_PTR *PFN_vkInvalidateMappedBufferRanges)(VkDevice device, uint32_t bufferRangeCount, const VkMappedBufferRange* pBufferRanges);
typedef VkResult(VKAPI_PTR *PFN_vkCreateBufferView)(VkDevice device, const VkBufferViewCreateInfo* pCreateInfo, VkBufferView* pView);
typedef void(VKAPI_PTR *PFN_vkDestroyBufferView)(VkDevice device, VkBufferView bufferView);
typedef VkResult(VKAPI_PTR *PFN_vkCreateImage)(VkDevice device, VkMemoryFlags memFlags, const VkImageCreateInfo* pCreateInfo, VkImage* pImage);
typedef void(VKAPI_PTR *PFN_vkDestroyImage)(VkDevice device, VkImage image);
typedef VkResult(VKAPI_PTR *PFN_vkImageSubData)(VkDevice device, VkImage image, const VkImageSubDataInfo* pSubDataInfo, const void* pData);
typedef VkResult(VKAPI_PTR *PFN_vkCreateImageView)(VkDevice device, const VkImageViewCreateInfo* pCreateInfo, VkImageView* pView);
typedef void(VKAPI_PTR *PFN_vkDestroyImageView)(VkDevice device, VkImageView imageView);
typedef VkResult(VKAPI_PTR *PFN_vkCreateFramebuffer)(VkDevice device, const VkFramebufferCreateInfo* pCreateInfo, VkFramebuffer* pFramebuffer);
typedef void(VKAPI_PTR *PFN_vkDestroyFramebuffer)(VkDevice device, VkFramebuffer framebuffer);
typedef VkResult(VKAPI_PTR *PFN_vkAllocateCommandBuffers)(VkDevice device, const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers);
typedef void(VKAPI_PTR *PFN_vkFreeCommandBuffers)(VkDevice device, uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers);
typedef VkResult(VKAPI_PTR *PFN_vkBeginCommandBuffer)(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags flags);
typedef VkResult(VKAPI_PTR *PFN_vkEndCommandBuffer)(VkCommandBuffer commandBuffer);
typedef VkResult(VKAPI_PTR *PFN_vkResetCommandBuffer)(VkCommandBuffer commandBuffer);
typedef void(VKAPI_PTR *PFN_vkCmdBeginRenderPass)(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo* pBeginInfo);
typedef void(VKAPI_PTR *PFN_vkCmdNextSubpass)(VkCommandBuffer commandBuffer);
typedef void(VKAPI_PTR *PFN_vkCmdEndRenderPass)(VkCommandBuffer commandBuffer);
typedef void(VKAPI_PTR *PFN_vkCmdBindPipeline)(VkCommandBuffer commandBuffer, VkPipeline pipeline);
typedef void(VKAPI_PTR *PFN_vkCmdPushConstants)(VkCommandBuffer commandBuffer, uint32_t offset, uint32_t size, const void* pValues);
typedef void(VKAPI_PTR *PFN_vkCmdBindBuffer)(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range, uint32_t set, uint32_t binding, uint32_t arrayElement);
typedef void(VKAPI_PTR *PFN_vkCmdBindBufferView)(VkCommandBuffer commandBuffer, VkBufferView bufferView, uint32_t set, uint32_t binding, uint32_t arrayElement);
typedef void(VKAPI_PTR *PFN_vkCmdBindImageView)(VkCommandBuffer commandBuffer, VkImageView imageView, VkSampler sampler, uint32_t set, uint32_t binding, uint32_t arrayElement);
typedef void(VKAPI_PTR *PFN_vkCmdBindSampler)(VkCommandBuffer commandBuffer, VkSampler sampler, uint32_t set, uint32_t binding, uint32_t arrayElement);
typedef void(VKAPI_PTR *PFN_vkCmdBindVertexBuffers)(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets);
typedef void(VKAPI_PTR *PFN_vkCmdBindIndexBuffer)(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType);
typedef void(VKAPI_PTR *PFN_vkCmdSetVertexInputFormat)(VkCommandBuffer commandBuffer, VkVertexInputFormat format);
typedef void(VKAPI_PTR *PFN_vkCmdSetViewportState)(VkCommandBuffer commandBuffer, uint32_t viewportCount);
typedef void(VKAPI_PTR *PFN_vkCmdSetInputAssemblyState)(VkCommandBuffer commandBuffer, const VkPipelineInputAssemblyState* pStateInfo);
typedef void(VKAPI_PTR *PFN_vkCmdSetRasterizationState)(VkCommandBuffer commandBuffer, const VkPipelineRasterizationState* pStateInfo);
typedef void(VKAPI_PTR *PFN_vkCmdSetMultisampleState)(VkCommandBuffer commandBuffer, const VkPipelineMultisampleState* pStateInfo);
typedef void(VKAPI_PTR *PFN_vkCmdSetDepthStencilState)(VkCommandBuffer commandBuffer, const VkPipelineDepthStencilState* pStateInfo);
typedef void(VKAPI_PTR *PFN_vkCmdSetColorBlendState)(VkCommandBuffer commandBuffer, const VkPipelineColorBlendState* pStateInfo);
typedef void(VKAPI_PTR *PFN_vkCmdSetViewport)(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkViewport* pViewports);
typedef void(VKAPI_PTR *PFN_vkCmdSetScissor)(VkCommandBuffer commandBuffer, uint32_t firstScissor, uint32_t scissorCount, const VkRect2D* pScissors);
typedef void(VKAPI_PTR *PFN_vkCmdSetLineWidth)(VkCommandBuffer commandBuffer, float lineWidth);
typedef void(VKAPI_PTR *PFN_vkCmdSetDepthBias)(VkCommandBuffer commandBuffer, float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor);
typedef void(VKAPI_PTR *PFN_vkCmdSetBlendConstants)(VkCommandBuffer commandBuffer, const float blendConstants[4]);
typedef void(VKAPI_PTR *PFN_vkCmdSetDepthBounds)(VkCommandBuffer commandBuffer, float minDepthBounds, float maxDepthBounds);
typedef void(VKAPI_PTR *PFN_vkCmdSetStencilCompareMask)(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t compareMask);
typedef void(VKAPI_PTR *PFN_vkCmdSetStencilWriteMask)(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t writeMask);
typedef void(VKAPI_PTR *PFN_vkCmdSetStencilReference)(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t reference);
typedef void(VKAPI_PTR *PFN_vkCmdDraw)(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
typedef void(VKAPI_PTR *PFN_vkCmdDrawIndexed)(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance);
typedef void(VKAPI_PTR *PFN_vkCmdDrawIndirect)(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride);
typedef void(VKAPI_PTR *PFN_vkCmdDrawIndexedIndirect)(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride);
typedef void(VKAPI_PTR *PFN_vkCmdDispatch)(VkCommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
typedef void(VKAPI_PTR *PFN_vkCmdDispatchIndirect)(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset);
typedef void(VKAPI_PTR *PFN_vkCmdCopyBuffer)(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy* pRegions);
typedef void(VKAPI_PTR *PFN_vkCmdCopyImage)(VkCommandBuffer commandBuffer, VkImage srcImage, VkImage dstImage, uint32_t regionCount, const VkImageCopy* pRegions);
typedef void(VKAPI_PTR *PFN_vkCmdBlitImage)(VkCommandBuffer commandBuffer, VkImage srcImage, VkImage dstImage, uint32_t regionCount, const VkImageBlit* pRegions, VkFilter filter);
typedef void(VKAPI_PTR *PFN_vkCmdCopyBufferToImage)(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage, uint32_t regionCount, const VkBufferImageCopy* pRegions);
typedef void(VKAPI_PTR *PFN_vkCmdCopyImageToBuffer)(VkCommandBuffer commandBuffer, VkImage srcImage, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy* pRegions);
typedef void(VKAPI_PTR *PFN_vkCmdUpdateBuffer)(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void* pData);
typedef void(VKAPI_PTR *PFN_vkCmdFillBuffer)(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data);
typedef void(VKAPI_PTR *PFN_vkCmdClearColorImage)(VkCommandBuffer commandBuffer, VkImage image, const VkClearColorValue* pColor, uint32_t rangeCount, const VkImageSubresourceRange* pRanges);
typedef void(VKAPI_PTR *PFN_vkCmdClearDepthStencilImage)(VkCommandBuffer commandBuffer, VkImage image, const VkClearDepthStencilValue* pDepthStencil, uint32_t rangeCount, const VkImageSubresourceRange* pRanges);
typedef void(VKAPI_PTR *PFN_vkCmdClearAttachments)(VkCommandBuffer commandBuffer, uint32_t attachmentCount, const VkClearAttachment* pAttachments, uint32_t rectCount, const VkClearRect* pRects);
typedef void(VKAPI_PTR *PFN_vkCmdResolveImage)(VkCommandBuffer commandBuffer, VkImage srcImage, VkImage dstImage, uint32_t regionCount, const VkImageResolve* pRegions);
typedef void(VKAPI_PTR *PFN_vkCmdSetEvent)(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask);
typedef void(VKAPI_PTR *PFN_vkCmdResetEvent)(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask);
typedef VkResult(VKAPI_CALL *PFN_vkCreateDebugReportCallbackEXT)(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, VkDebugReportCallbackEXT* pCallback);
typedef void(VKAPI_CALL *PFN_vkDestroyDebugReportCallbackEXT)(VkInstance instance, VkDebugReportCallbackEXT callback);
typedef void(VKAPI_CALL *PFN_vkDebugReportMessageEXT)(VkInstance instance, VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage);

#define ENTRY_POINT_DECLARATION(name) extern PFN_##name name

ENTRY_POINT_DECLARATION(vkCreateInstance);
ENTRY_POINT_DECLARATION(vkDestroyInstance);
ENTRY_POINT_DECLARATION(vkEnumerateInstanceExtensionProperties);
ENTRY_POINT_DECLARATION(vkEnumerateInstanceLayerProperties);
ENTRY_POINT_DECLARATION(vkCreateSurface);
ENTRY_POINT_DECLARATION(vkDestroySurface);
ENTRY_POINT_DECLARATION(vkEnumeratePhysicalDevices);
ENTRY_POINT_DECLARATION(vkGetPhysicalDeviceProperties);
ENTRY_POINT_DECLARATION(vkGetPhysicalDeviceFeatures);
ENTRY_POINT_DECLARATION(vkGetPhysicalDeviceFormatProperties);
ENTRY_POINT_DECLARATION(vkGetPhysicalDeviceImageFormatProperties);
ENTRY_POINT_DECLARATION(vkGetPhysicalDeviceQueueFamilyProperties);
ENTRY_POINT_DECLARATION(vkGetPhysicalDeviceSurfaceFormats);
ENTRY_POINT_DECLARATION(vkGetPhysicalDevicePresentSupport);
ENTRY_POINT_DECLARATION(vkEnumerateDeviceExtensionProperties);
ENTRY_POINT_DECLARATION(vkEnumerateDeviceLayerProperties);
ENTRY_POINT_DECLARATION(vkCreateDevice);
ENTRY_POINT_DECLARATION(vkDestroyDevice);
ENTRY_POINT_DECLARATION(vkDeviceWaitIdle);
ENTRY_POINT_DECLARATION(vkDeviceSetVSync);
ENTRY_POINT_DECLARATION(vkGetDeviceQueue);
ENTRY_POINT_DECLARATION(vkGetDeviceGraphicsQueue);
ENTRY_POINT_DECLARATION(vkGetDeviceComputeQueue);
ENTRY_POINT_DECLARATION(vkGetDeviceTransferQueue);
ENTRY_POINT_DECLARATION(vkQueueSubmit);
ENTRY_POINT_DECLARATION(vkQueuePresent);
ENTRY_POINT_DECLARATION(vkQueueWaitIdle);
ENTRY_POINT_DECLARATION(vkDestroyFence);
ENTRY_POINT_DECLARATION(vkGetFenceStatus);
ENTRY_POINT_DECLARATION(vkWaitForFences);
ENTRY_POINT_DECLARATION(vkDestroySemaphore);
ENTRY_POINT_DECLARATION(vkCreateEvent);
ENTRY_POINT_DECLARATION(vkDestroyEvent);
ENTRY_POINT_DECLARATION(vkGetEventStatus);
ENTRY_POINT_DECLARATION(vkSetEvent);
ENTRY_POINT_DECLARATION(vkResetEvent);
ENTRY_POINT_DECLARATION(vkCreateQueryPool);
ENTRY_POINT_DECLARATION(vkDestroyQueryPool);
ENTRY_POINT_DECLARATION(vkGetQueryPoolResults);
ENTRY_POINT_DECLARATION(vkCreateShaderModule);
ENTRY_POINT_DECLARATION(vkDestroyShaderModule);
ENTRY_POINT_DECLARATION(vkGetShaderModuleInfoLog);
ENTRY_POINT_DECLARATION(vkCreateGraphicsPipeline);
ENTRY_POINT_DECLARATION(vkCreateComputePipeline);
ENTRY_POINT_DECLARATION(vkDestroyPipeline);
ENTRY_POINT_DECLARATION(vkEnumeratePipelineResources);
ENTRY_POINT_DECLARATION(vkGetPipelineResource);
ENTRY_POINT_DECLARATION(vkCreateVertexInputFormat);
ENTRY_POINT_DECLARATION(vkDestroyVertexInputFormat);
ENTRY_POINT_DECLARATION(vkCreateSampler);
ENTRY_POINT_DECLARATION(vkDestroySampler);
ENTRY_POINT_DECLARATION(vkCreateBuffer);
ENTRY_POINT_DECLARATION(vkDestroyBuffer);
ENTRY_POINT_DECLARATION(vkBufferSubData);
ENTRY_POINT_DECLARATION(vkMapBuffer);
ENTRY_POINT_DECLARATION(vkUnmapBuffer);
ENTRY_POINT_DECLARATION(vkFlushMappedBufferRanges);
ENTRY_POINT_DECLARATION(vkInvalidateMappedBufferRanges);
ENTRY_POINT_DECLARATION(vkCreateBufferView);
ENTRY_POINT_DECLARATION(vkDestroyBufferView);
ENTRY_POINT_DECLARATION(vkCreateImage);
ENTRY_POINT_DECLARATION(vkDestroyImage);
ENTRY_POINT_DECLARATION(vkImageSubData);
ENTRY_POINT_DECLARATION(vkCreateImageView);
ENTRY_POINT_DECLARATION(vkDestroyImageView);
ENTRY_POINT_DECLARATION(vkCreateFramebuffer);
ENTRY_POINT_DECLARATION(vkDestroyFramebuffer);
ENTRY_POINT_DECLARATION(vkAllocateCommandBuffers);
ENTRY_POINT_DECLARATION(vkFreeCommandBuffers);
ENTRY_POINT_DECLARATION(vkBeginCommandBuffer);
ENTRY_POINT_DECLARATION(vkEndCommandBuffer);
ENTRY_POINT_DECLARATION(vkResetCommandBuffer);
ENTRY_POINT_DECLARATION(vkCmdBeginRenderPass);
ENTRY_POINT_DECLARATION(vkCmdNextSubpass);
ENTRY_POINT_DECLARATION(vkCmdEndRenderPass);
ENTRY_POINT_DECLARATION(vkCmdExecuteCommands);
ENTRY_POINT_DECLARATION(vkCmdBindPipeline);
ENTRY_POINT_DECLARATION(vkCmdPushConstants);
ENTRY_POINT_DECLARATION(vkCmdBindBuffer);
ENTRY_POINT_DECLARATION(vkCmdBindBufferView);
ENTRY_POINT_DECLARATION(vkCmdBindImageView);
ENTRY_POINT_DECLARATION(vkCmdBindSampler);
ENTRY_POINT_DECLARATION(vkCmdBindVertexBuffers);
ENTRY_POINT_DECLARATION(vkCmdBindIndexBuffer);
ENTRY_POINT_DECLARATION(vkCmdSetVertexInputFormat);
ENTRY_POINT_DECLARATION(vkCmdSetViewportState);
ENTRY_POINT_DECLARATION(vkCmdSetInputAssemblyState);
ENTRY_POINT_DECLARATION(vkCmdSetRasterizationState);
ENTRY_POINT_DECLARATION(vkCmdSetMultisampleState);
ENTRY_POINT_DECLARATION(vkCmdSetDepthStencilState);
ENTRY_POINT_DECLARATION(vkCmdSetColorBlendState);
ENTRY_POINT_DECLARATION(vkCmdSetViewport);
ENTRY_POINT_DECLARATION(vkCmdSetScissor);
ENTRY_POINT_DECLARATION(vkCmdSetLineWidth);
ENTRY_POINT_DECLARATION(vkCmdSetDepthBias);
ENTRY_POINT_DECLARATION(vkCmdSetBlendConstants);
ENTRY_POINT_DECLARATION(vkCmdSetDepthBounds);
ENTRY_POINT_DECLARATION(vkCmdSetStencilCompareMask);
ENTRY_POINT_DECLARATION(vkCmdSetStencilWriteMask);
ENTRY_POINT_DECLARATION(vkCmdSetStencilReference);
ENTRY_POINT_DECLARATION(vkCmdDraw);
ENTRY_POINT_DECLARATION(vkCmdDrawIndexed);
ENTRY_POINT_DECLARATION(vkCmdDrawIndirect);
ENTRY_POINT_DECLARATION(vkCmdDrawIndexedIndirect);
ENTRY_POINT_DECLARATION(vkCmdDispatch);
ENTRY_POINT_DECLARATION(vkCmdDispatchIndirect);
ENTRY_POINT_DECLARATION(vkCmdCopyBuffer);
ENTRY_POINT_DECLARATION(vkCmdCopyImage);
ENTRY_POINT_DECLARATION(vkCmdBlitImage);
ENTRY_POINT_DECLARATION(vkCmdCopyBufferToImage);
ENTRY_POINT_DECLARATION(vkCmdCopyImageToBuffer);
ENTRY_POINT_DECLARATION(vkCmdUpdateBuffer);
ENTRY_POINT_DECLARATION(vkCmdFillBuffer);
ENTRY_POINT_DECLARATION(vkCmdClearColorImage);
ENTRY_POINT_DECLARATION(vkCmdClearDepthStencilImage);
ENTRY_POINT_DECLARATION(vkCmdClearAttachments);
ENTRY_POINT_DECLARATION(vkCmdResolveImage);
ENTRY_POINT_DECLARATION(vkCmdSetEvent);
ENTRY_POINT_DECLARATION(vkCmdResetEvent);

ENTRY_POINT_DECLARATION(vkCreateDebugReportCallbackEXT);
ENTRY_POINT_DECLARATION(vkDestroyDebugReportCallbackEXT);
ENTRY_POINT_DECLARATION(vkDebugReportMessageEXT);

extern VkResult InitVulkanEZ();

#endif

#endif