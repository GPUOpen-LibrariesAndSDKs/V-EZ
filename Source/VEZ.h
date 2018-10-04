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
#ifndef VEZ_H
#define VEZ_H

#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif

VK_DEFINE_NON_DISPATCHABLE_HANDLE(VezSwapchain)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VezPipeline)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VezFramebuffer)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VezVertexInputFormat)

typedef enum VezMemoryFlagsBits
{
    VEZ_MEMORY_GPU_ONLY = 0x00000000,
    VEZ_MEMORY_CPU_ONLY = 0x00000001,
    VEZ_MEMORY_CPU_TO_GPU = 0x00000002,
    VEZ_MEMORY_GPU_TO_CPU = 0x00000004,
    VEZ_MEMORY_DEDICATED_ALLOCATION = 0x00000008,
    VEZ_MEMORY_NO_ALLOCATION = 0x000000010,
} VezMemoryFlagsBits;
typedef VkFlags VezMemoryFlags;

typedef enum VezBaseType
{
    VEZ_BASE_TYPE_BOOL = 0,
    VEZ_BASE_TYPE_CHAR = 1,
    VEZ_BASE_TYPE_INT = 2,
    VEZ_BASE_TYPE_UINT = 3,
    VEZ_BASE_TYPE_UINT64 = 4,
    VEZ_BASE_TYPE_HALF = 5,
    VEZ_BASE_TYPE_FLOAT = 6,
    VEZ_BASE_TYPE_DOUBLE = 7,
    VEZ_BASE_TYPE_STRUCT = 8,
} VezBaseType;

typedef enum VezPipelineResourceType
{
    VEZ_PIPELINE_RESOURCE_TYPE_INPUT = 0,
    VEZ_PIPELINE_RESOURCE_TYPE_OUTPUT = 1,
    VEZ_PIPELINE_RESOURCE_TYPE_SAMPLER = 2,
    VEZ_PIPELINE_RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER = 3,
    VEZ_PIPELINE_RESOURCE_TYPE_SAMPLED_IMAGE = 4,
    VEZ_PIPELINE_RESOURCE_TYPE_STORAGE_IMAGE = 5,
    VEZ_PIPELINE_RESOURCE_TYPE_UNIFORM_TEXEL_BUFFER = 6,
    VEZ_PIPELINE_RESOURCE_TYPE_STORAGE_TEXEL_BUFFER = 7,
    VEZ_PIPELINE_RESOURCE_TYPE_UNIFORM_BUFFER = 8,
    VEZ_PIPELINE_RESOURCE_TYPE_STORAGE_BUFFER = 9,
    VEZ_PIPELINE_RESOURCE_TYPE_INPUT_ATTACHMENT = 10,
    VEZ_PIPELINE_RESOURCE_TYPE_PUSH_CONSTANT_BUFFER = 11,
} VezPipelineResourceType;

typedef struct VezClearAttachment
{
    uint32_t colorAttachment;
    VkClearValue clearValue;
} VezClearAttachment;

typedef struct VezApplicationInfo
{
    const void* pNext;
    const char* pApplicationName;
    uint32_t applicationVersion;
    const char* pEngineName;
    uint32_t engineVersion;
} VezApplicationInfo;

typedef struct VezInstanceCreateInfo
{
    const void* pNext;
    const VezApplicationInfo* pApplicationInfo;
    uint32_t enabledLayerCount;
    const char* const* ppEnabledLayerNames;
    uint32_t enabledExtensionCount;
    const char* const* ppEnabledExtensionNames;
} VezInstanceCreateInfo;

typedef struct VezSwapchainCreateInfo
{
    const void* pNext;
    VkSurfaceKHR surface;
    VkSurfaceFormatKHR format;
    VkBool32 tripleBuffer;
} VezSwapchainCreateInfo;

typedef struct VezDeviceCreateInfo
{
    const void* pNext;
    uint32_t enabledLayerCount;
    const char* const* ppEnabledLayerNames;
    uint32_t enabledExtensionCount;
    const char* const* ppEnabledExtensionNames;
} VezDeviceCreateInfo;

typedef struct VezSubmitInfo
{
    const void* pNext;
    uint32_t waitSemaphoreCount;
    const VkSemaphore* pWaitSemaphores;
    const VkPipelineStageFlags* pWaitDstStageMask;
    uint32_t commandBufferCount;
    const VkCommandBuffer* pCommandBuffers;
    uint32_t signalSemaphoreCount;
    VkSemaphore* pSignalSemaphores;
} VezSubmitInfo;

typedef struct VezPresentInfo
{
    const void* pNext;
    uint32_t waitSemaphoreCount;
    const VkSemaphore* pWaitSemaphores;
    const VkPipelineStageFlags* pWaitDstStageMask;
    uint32_t swapchainCount;
    const VezSwapchain* pSwapchains;
    const VkImage* pImages;
    uint32_t signalSemaphoreCount;
    VkSemaphore* pSignalSemaphores;
    VkResult* pResults;
} VezPresentInfo;

typedef struct VezQueryPoolCreateInfo
{
    const void* pNext;
    VkQueryType queryType;
    uint32_t queryCount;
    VkQueryPipelineStatisticFlags pipelineStatistics;
} VezQueryPoolCreateInfo;

typedef struct VezCommandBufferAllocateInfo
{
    const void* pNext;
    VkQueue queue;
    uint32_t commandBufferCount;
} VezCommandBufferAllocateInfo;

typedef struct VezShaderModuleCreateInfo
{
    const void* pNext;
    VkShaderStageFlagBits stage;
    size_t codeSize;
    const uint32_t* pCode;
    const char* pGLSLSource;
    const char* pEntryPoint;
} VezShaderModuleCreateInfo;

typedef struct VezPipelineShaderStageCreateInfo
{
    const void* pNext;
    VkShaderModule module;
    const char* pEntryPoint;
    const VkSpecializationInfo* pSpecializationInfo;
} VezPipelineShaderStageCreateInfo;

typedef struct VezGraphicsPipelineCreateInfo
{
    const void* pNext;
    uint32_t stageCount;
    const VezPipelineShaderStageCreateInfo* pStages;
} VezGraphicsPipelineCreateInfo;

typedef struct VezComputePipelineCreateInfo
{
    const void* pNext;
    const VezPipelineShaderStageCreateInfo* pStage;
} VezComputePipelineCreateInfo;

typedef struct VezMemberInfo
{
    VezBaseType baseType;
    uint32_t offset;
    uint32_t size;
    uint32_t vecSize;
    uint32_t columns;
    uint32_t arraySize;
    char name[VK_MAX_DESCRIPTION_SIZE];
    const VezMemberInfo* pNext;
    const VezMemberInfo* pMembers;
} VezMemberInfo;

typedef struct VezPipelineResource
{
    VkShaderStageFlags stages;
    VezPipelineResourceType resourceType;
    VezBaseType baseType;
    VkAccessFlags access;
    uint32_t set;
    uint32_t binding;
    uint32_t location;
    uint32_t inputAttachmentIndex;
    uint32_t vecSize;
    uint32_t columns;
    uint32_t arraySize;
    uint32_t offset;
    uint32_t size;
    char name[VK_MAX_DESCRIPTION_SIZE];
    const VezMemberInfo* pMembers;
} VezPipelineResource;

typedef struct VezVertexInputFormatCreateInfo
{
    uint32_t vertexBindingDescriptionCount;
    const VkVertexInputBindingDescription* pVertexBindingDescriptions;
    uint32_t vertexAttributeDescriptionCount;
    const VkVertexInputAttributeDescription* pVertexAttributeDescriptions;
} VezVertexInputFormatCreateInfo;

typedef struct VezSamplerCreateInfo
{
    const void* pNext;
    VkFilter magFilter;
    VkFilter minFilter;
    VkSamplerMipmapMode mipmapMode;
    VkSamplerAddressMode addressModeU;
    VkSamplerAddressMode addressModeV;
    VkSamplerAddressMode addressModeW;
    float mipLodBias;
    VkBool32 anisotropyEnable;
    float maxAnisotropy;
    VkBool32 compareEnable;
    VkCompareOp compareOp;
    float minLod;
    float maxLod;
    VkBorderColor borderColor;
    VkBool32 unnormalizedCoordinates;
} VezSamplerCreateInfo;

typedef struct VezBufferCreateInfo
{
    const void* pNext;
    VkDeviceSize size;
    VkBufferUsageFlags usage;
    uint32_t queueFamilyIndexCount;
    const uint32_t* pQueueFamilyIndices;
} VezBufferCreateInfo;

typedef struct VezMappedBufferRange
{
    VkBuffer buffer;
    VkDeviceSize offset;
    VkDeviceSize size;
} VezMappedBufferRange;

typedef struct VezBufferViewCreateInfo
{
    const void* pNext;
    VkBuffer buffer;
    VkFormat format;
    VkDeviceSize offset;
    VkDeviceSize range;
} VezBufferViewCreateInfo;

typedef struct VezImageCreateInfo
{
    const void* pNext;
    VkImageCreateFlags flags;
    VkImageType imageType;
    VkFormat format;
    VkExtent3D extent;
    uint32_t mipLevels;
    uint32_t arrayLayers;
    VkSampleCountFlagBits samples;
    VkImageTiling tiling;
    VkImageUsageFlags usage;
    uint32_t queueFamilyIndexCount;
    const uint32_t* pQueueFamilyIndices;
} VezImageCreateInfo;

typedef struct VezImageSubresource
{
    uint32_t mipLevel;
    uint32_t arrayLayer;
} VezImageSubresource;

typedef struct VezSubresourceLayout
{
    VkDeviceSize offset;
    VkDeviceSize size;
    VkDeviceSize rowPitch;
    VkDeviceSize arrayPitch;
    VkDeviceSize depthPitch;
} VezSubresourceLayout;

typedef struct VezImageSubresourceRange
{
    uint32_t baseMipLevel;
    uint32_t levelCount;
    uint32_t baseArrayLayer;
    uint32_t layerCount;
} VezImageSubresourceRange;

typedef struct VezImageViewCreateInfo
{
    const void* pNext;
    VkImage image;
    VkImageViewType viewType;
    VkFormat format;
    VkComponentMapping components;
    VezImageSubresourceRange subresourceRange;
} VezImageViewCreateInfo;

typedef struct VezFramebufferCreateInfo
{
    const void* pNext;
    uint32_t attachmentCount;
    const VkImageView* pAttachments;
    uint32_t width;
    uint32_t height;
    uint32_t layers;
} VezFramebufferCreateInfo;

typedef struct VezInputAssemblyState
{
    const void* pNext;
    VkPrimitiveTopology topology;
    VkBool32 primitiveRestartEnable;
} VezInputAssemblyState;

typedef struct VezRasterizationState
{
    const void* pNext;
    VkBool32 depthClampEnable;
    VkBool32 rasterizerDiscardEnable;
    VkPolygonMode polygonMode;
    VkCullModeFlags cullMode;
    VkFrontFace frontFace;
    VkBool32 depthBiasEnable;
} VezRasterizationState;

typedef struct VezMultisampleState
{
    const void* pNext;
    VkSampleCountFlagBits rasterizationSamples;
    VkBool32 sampleShadingEnable;
    float minSampleShading;
    const VkSampleMask* pSampleMask;
    VkBool32 alphaToCoverageEnable;
    VkBool32 alphaToOneEnable;
} VezMultisampleStateCreateInfo;

typedef struct VezStencilOpState
{
    VkStencilOp failOp;
    VkStencilOp passOp;
    VkStencilOp depthFailOp;
    VkCompareOp compareOp;
} VezStencilOpState;

typedef struct VezDepthStencilState
{
    const void* pNext;
    VkBool32 depthTestEnable;
    VkBool32 depthWriteEnable;
    VkCompareOp depthCompareOp;
    VkBool32 depthBoundsTestEnable;
    VkBool32 stencilTestEnable;
    VezStencilOpState front;
    VezStencilOpState back;
} VezPipelineDepthStencilState;

typedef struct VezColorBlendAttachmentState
{
    VkBool32 blendEnable;
    VkBlendFactor srcColorBlendFactor;
    VkBlendFactor dstColorBlendFactor;
    VkBlendOp colorBlendOp;
    VkBlendFactor srcAlphaBlendFactor;
    VkBlendFactor dstAlphaBlendFactor;
    VkBlendOp alphaBlendOp;
    VkColorComponentFlags colorWriteMask;
} VezColorBlendAttachmentState;

typedef struct VezColorBlendState
{
    const void* pNext;
    VkBool32 logicOpEnable;
    VkLogicOp logicOp;
    uint32_t attachmentCount;
    const VezColorBlendAttachmentState* pAttachments;
} VezColorBlendState;

typedef struct VezAttachmentInfo
{
    VkAttachmentLoadOp loadOp;
    VkAttachmentStoreOp storeOp;
    VkClearValue clearValue;
} VezAttachmentReference;

typedef struct VezRenderPassBeginInfo
{
    const void* pNext;
    VezFramebuffer framebuffer;
    uint32_t attachmentCount;
    const VezAttachmentInfo* pAttachments;
} VezRenderPassBeginInfo;

typedef struct VezBufferCopy
{
    VkDeviceSize srcOffset;
    VkDeviceSize dstOffset;
    VkDeviceSize size;
} VezBufferCopy;

typedef struct VezImageSubresourceLayers
{
    uint32_t mipLevel;
    uint32_t baseArrayLayer;
    uint32_t layerCount;
} VezImageSubresourceLayers;

typedef struct VezImageSubDataInfo
{
    uint32_t dataRowLength;
    uint32_t dataImageHeight;
    VezImageSubresourceLayers imageSubresource;
    VkOffset3D imageOffset;
    VkExtent3D imageExtent;
} VezImageSubDataInfo;

typedef struct VezImageResolve
{
    VezImageSubresourceLayers srcSubresource;
    VkOffset3D srcOffset;
    VezImageSubresourceLayers dstSubresource;
    VkOffset3D dstOffset;
    VkExtent3D extent;
} VezImageResolve;

typedef struct VezImageCopy
{
    VezImageSubresourceLayers srcSubresource;
    VkOffset3D srcOffset;
    VezImageSubresourceLayers dstSubresource;
    VkOffset3D dstOffset;
    VkExtent3D extent;
} VezImageCopy;

typedef struct VezImageBlit
{
    VezImageSubresourceLayers srcSubresource;
    VkOffset3D srcOffsets[2];
    VezImageSubresourceLayers dstSubresource;
    VkOffset3D dstOffsets[2];
} VezImageBlit;

typedef struct VezBufferImageCopy
{
    VkDeviceSize bufferOffset;
    uint32_t bufferRowLength;
    uint32_t bufferImageHeight;
    VezImageSubresourceLayers imageSubresource;
    VkOffset3D imageOffset;
    VkExtent3D imageExtent;
} VezBufferImageCopy;

// Instance functions.
VKAPI_ATTR VkResult VKAPI_CALL vezEnumerateInstanceExtensionProperties(const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties);
VKAPI_ATTR VkResult VKAPI_CALL vezEnumerateInstanceLayerProperties(uint32_t* pPropertyCount,VkLayerProperties*pProperties);
VKAPI_ATTR VkResult VKAPI_CALL vezCreateInstance(const VezInstanceCreateInfo* pCreateInfo, VkInstance* pInstance);
VKAPI_ATTR void VKAPI_CALL vezDestroyInstance(VkInstance instance);
VKAPI_ATTR VkResult VKAPI_CALL vezEnumeratePhysicalDevices(VkInstance instance, uint32_t* pPhysicalDeviceCount, VkPhysicalDevice* pPhysicalDevices);

// Physical device functions.
VKAPI_ATTR void VKAPI_CALL vezGetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties* pProperties);
VKAPI_ATTR void VKAPI_CALL vezGetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures* pFeatures);
VKAPI_ATTR void VKAPI_CALL vezGetPhysicalDeviceFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties* pFormatProperties);
VKAPI_ATTR VkResult VKAPI_CALL vezGetPhysicalDeviceImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkImageFormatProperties* pImageFormatProperties);
VKAPI_ATTR void VKAPI_CALL vezGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice, uint32_t* pQueueFamilyPropertyCount, VkQueueFamilyProperties* pQueueFamilyProperties);
VKAPI_ATTR VkResult VKAPI_CALL vezGetPhysicalDeviceSurfaceFormats(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t* pSurfaceFormatCount, VkSurfaceFormatKHR* pSurfaceFormats);
VKAPI_ATTR VkResult VKAPI_CALL vezGetPhysicalDevicePresentSupport(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, VkSurfaceKHR surface, VkBool32* pSupported);
VKAPI_ATTR VkResult VKAPI_CALL vezEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties);
VKAPI_ATTR VkResult VKAPI_CALL vezEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t* pPropertyCount, VkLayerProperties* pProperties);

// Device functions.
VKAPI_ATTR VkResult VKAPI_CALL vezCreateDevice(VkPhysicalDevice physicalDevice, const VezDeviceCreateInfo* pCreateInfo, VkDevice* pDevice);
VKAPI_ATTR void VKAPI_CALL vezDestroyDevice(VkDevice device);
VKAPI_ATTR VkResult VKAPI_CALL vezDeviceWaitIdle(VkDevice device);
VKAPI_ATTR void VKAPI_CALL vezGetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue* pQueue);
VKAPI_ATTR void VKAPI_CALL vezGetDeviceGraphicsQueue(VkDevice device, uint32_t queueIndex, VkQueue* pQueue);
VKAPI_ATTR void VKAPI_CALL vezGetDeviceComputeQueue(VkDevice device, uint32_t queueIndex, VkQueue* pQueue);
VKAPI_ATTR void VKAPI_CALL vezGetDeviceTransferQueue(VkDevice device, uint32_t queueIndex, VkQueue* pQueue);

// Swapchain
VKAPI_ATTR VkResult VKAPI_CALL vezCreateSwapchain(VkDevice device, const VezSwapchainCreateInfo* pCreateInfo, VezSwapchain* pSwapchain);
VKAPI_ATTR void VKAPI_CALL vezDestroySwapchain(VkDevice device, VezSwapchain swapchain);
VKAPI_ATTR void VKAPI_CALL vezGetSwapchainSurfaceFormat(VezSwapchain swapchain, VkSurfaceFormatKHR* pFormat);
VKAPI_ATTR VkResult VKAPI_CALL vezSwapchainSetVSync(VezSwapchain swapchain, VkBool32 enabled);

// Queue functions.
VKAPI_ATTR VkResult VKAPI_CALL vezQueueSubmit(VkQueue queue, uint32_t submitCount, const VezSubmitInfo* pSubmits, VkFence* pFence);
VKAPI_ATTR VkResult VKAPI_CALL vezQueuePresent(VkQueue queue, const VezPresentInfo* pPresentInfo);
VKAPI_ATTR VkResult VKAPI_CALL vezQueueWaitIdle(VkQueue queue);

// Synchronization primitives functions.
VKAPI_ATTR void VKAPI_CALL vezDestroyFence(VkDevice device, VkFence fence);
VKAPI_ATTR VkResult VKAPI_CALL vezGetFenceStatus(VkDevice device, VkFence fence);
VKAPI_ATTR VkResult VKAPI_CALL vezWaitForFences(VkDevice device, uint32_t fenceCount, const VkFence* pFences, VkBool32 waitAll, uint64_t timeout);
VKAPI_ATTR void VKAPI_CALL vezDestroySemaphore(VkDevice device, VkSemaphore semaphore);
VKAPI_ATTR VkResult VKAPI_CALL vezCreateEvent(VkDevice device, VkEvent* pEvent);
VKAPI_ATTR void VKAPI_CALL vezDestroyEvent(VkDevice device, VkEvent event);
VKAPI_ATTR VkResult VKAPI_CALL vezGetEventStatus(VkDevice device, VkEvent event);
VKAPI_ATTR VkResult VKAPI_CALL vezSetEvent(VkDevice device, VkEvent event);
VKAPI_ATTR VkResult VKAPI_CALL vezResetEvent(VkDevice device, VkEvent event);

// Query pool functions.
VKAPI_ATTR VkResult VKAPI_CALL vezCreateQueryPool(VkDevice device, const VezQueryPoolCreateInfo* pCreateInfo, VkQueryPool* pQueryPool);
VKAPI_ATTR void VKAPI_CALL vezDestroyQueryPool(VkDevice device, VkQueryPool queryPool);
VKAPI_ATTR VkResult VKAPI_CALL vezGetQueryPoolResults(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, size_t dataSize, void* pData, VkDeviceSize stride, VkQueryResultFlags flags);

// Shader module and pipeline functions.
VKAPI_ATTR VkResult VKAPI_CALL vezCreateShaderModule(VkDevice device, const VezShaderModuleCreateInfo* pCreateInfo, VkShaderModule* pShaderModule);
VKAPI_ATTR void VKAPI_CALL vezDestroyShaderModule(VkDevice device, VkShaderModule shaderModule);
VKAPI_ATTR void VKAPI_CALL vezGetShaderModuleInfoLog(VkShaderModule shaderModule, uint32_t* pLength, char* pInfoLog);
VKAPI_ATTR VkResult VKAPI_CALL vezGetShaderModuleBinary(VkShaderModule shaderModule, uint32_t* pLength, uint32_t* pBinary);
VKAPI_ATTR VkResult VKAPI_CALL vezCreateGraphicsPipeline(VkDevice device, const VezGraphicsPipelineCreateInfo* pCreateInfo, VezPipeline* pPipeline);
VKAPI_ATTR VkResult VKAPI_CALL vezCreateComputePipeline(VkDevice device, const VezComputePipelineCreateInfo* pCreateInfo, VezPipeline* pPipeline);
VKAPI_ATTR void VKAPI_CALL vezDestroyPipeline(VkDevice device, VezPipeline pipeline);
VKAPI_ATTR VkResult VKAPI_CALL vezEnumeratePipelineResources(VezPipeline pipeline, uint32_t* pResourceCount, VezPipelineResource* ppResources);
VKAPI_ATTR VkResult VKAPI_CALL vezGetPipelineResource(VezPipeline pipeline, const char* name, VezPipelineResource* pResource);

// Vertex input format functions.
VKAPI_ATTR VkResult VKAPI_CALL vezCreateVertexInputFormat(VkDevice device, const VezVertexInputFormatCreateInfo* pCreateInfo, VezVertexInputFormat* pFormat);
VKAPI_ATTR void VKAPI_CALL vezDestroyVertexInputFormat(VkDevice device, VezVertexInputFormat format);

// Sampler functions.
VKAPI_ATTR VkResult VKAPI_CALL vezCreateSampler(VkDevice device, const VezSamplerCreateInfo* pCreateInfo, VkSampler* pSampler);
VKAPI_ATTR void VKAPI_CALL vezDestroySampler(VkDevice device, VkSampler sampler);

// Buffer functions.
VKAPI_ATTR VkResult VKAPI_CALL vezCreateBuffer(VkDevice device, VezMemoryFlags memFlags, const VezBufferCreateInfo* pCreateInfo, VkBuffer* pBuffer);
VKAPI_ATTR void VKAPI_CALL vezDestroyBuffer(VkDevice device, VkBuffer buffer);
VKAPI_ATTR VkResult VKAPI_CALL vezBufferSubData(VkDevice device, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, const void* pData);
VKAPI_ATTR VkResult VKAPI_CALL vezMapBuffer(VkDevice device, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, void** ppData);
VKAPI_ATTR void VKAPI_CALL vezUnmapBuffer(VkDevice device, VkBuffer buffer);
VKAPI_ATTR VkResult VKAPI_CALL vezFlushMappedBufferRanges(VkDevice device, uint32_t bufferRangeCount, const VezMappedBufferRange* pBufferRanges);
VKAPI_ATTR VkResult VKAPI_CALL vezInvalidateMappedBufferRanges(VkDevice device, uint32_t bufferRangeCount, const VezMappedBufferRange* pBufferRanges);
VKAPI_ATTR VkResult VKAPI_CALL vezCreateBufferView(VkDevice device, const VezBufferViewCreateInfo* pCreateInfo, VkBufferView* pView);
VKAPI_ATTR void VKAPI_CALL vezDestroyBufferView(VkDevice device, VkBufferView bufferView);

// Image functions.
VKAPI_ATTR VkResult VKAPI_CALL vezCreateImage(VkDevice device, VezMemoryFlags memFlags, const VezImageCreateInfo* pCreateInfo, VkImage* pImage);
VKAPI_ATTR void VKAPI_CALL vezDestroyImage(VkDevice device, VkImage image);
VKAPI_ATTR VkResult VKAPI_CALL vezImageSubData(VkDevice device, VkImage image, const VezImageSubDataInfo* pSubDataInfo, const void* pData);
VKAPI_ATTR VkResult VKAPI_CALL vezCreateImageView(VkDevice device, const VezImageViewCreateInfo* pCreateInfo, VkImageView* pView);
VKAPI_ATTR void VKAPI_CALL vezDestroyImageView(VkDevice device, VkImageView imageView);

// Framebuffer functions.
VKAPI_ATTR VkResult VKAPI_CALL vezCreateFramebuffer(VkDevice device, const VezFramebufferCreateInfo* pCreateInfo, VezFramebuffer* pFramebuffer);
VKAPI_ATTR void VKAPI_CALL vezDestroyFramebuffer(VkDevice device, VezFramebuffer framebuffer);

// Command buffer functions.
VKAPI_ATTR VkResult VKAPI_CALL vezAllocateCommandBuffers(VkDevice device, const VezCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers);
VKAPI_ATTR void VKAPI_CALL vezFreeCommandBuffers(VkDevice device, uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers);
VKAPI_ATTR VkResult VKAPI_CALL vezBeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags flags);
VKAPI_ATTR VkResult VKAPI_CALL vezEndCommandBuffer();
VKAPI_ATTR VkResult VKAPI_CALL vezResetCommandBuffer(VkCommandBuffer commandBuffer);
VKAPI_ATTR void VKAPI_CALL vezCmdBeginRenderPass(const VezRenderPassBeginInfo* pBeginInfo);
VKAPI_ATTR void VKAPI_CALL vezCmdNextSubpass();
VKAPI_ATTR void VKAPI_CALL vezCmdEndRenderPass();
VKAPI_ATTR void VKAPI_CALL vezCmdBindPipeline(VezPipeline pipeline);
VKAPI_ATTR void VKAPI_CALL vezCmdPushConstants(uint32_t offset, uint32_t size, const void* pValues);
VKAPI_ATTR void VKAPI_CALL vezCmdBindBuffer(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range, uint32_t set, uint32_t binding, uint32_t arrayElement);
VKAPI_ATTR void VKAPI_CALL vezCmdBindBufferView(VkBufferView bufferView, uint32_t set, uint32_t binding, uint32_t arrayElement);
VKAPI_ATTR void VKAPI_CALL vezCmdBindImageView(VkImageView imageView, VkSampler sampler, uint32_t set, uint32_t binding, uint32_t arrayElement);
VKAPI_ATTR void VKAPI_CALL vezCmdBindSampler(VkSampler sampler, uint32_t set, uint32_t binding, uint32_t arrayElement);
VKAPI_ATTR void VKAPI_CALL vezCmdBindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets);
VKAPI_ATTR void VKAPI_CALL vezCmdBindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType);
VKAPI_ATTR void VKAPI_CALL vezCmdSetVertexInputFormat(VezVertexInputFormat format);
VKAPI_ATTR void VKAPI_CALL vezCmdSetViewportState(uint32_t viewportCount);
VKAPI_ATTR void VKAPI_CALL vezCmdSetInputAssemblyState(const VezInputAssemblyState* pStateInfo);
VKAPI_ATTR void VKAPI_CALL vezCmdSetRasterizationState(const VezRasterizationState* pStateInfo);
VKAPI_ATTR void VKAPI_CALL vezCmdSetMultisampleState(const VezMultisampleState* pStateInfo);
VKAPI_ATTR void VKAPI_CALL vezCmdSetDepthStencilState(const VezDepthStencilState* pStateInfo);
VKAPI_ATTR void VKAPI_CALL vezCmdSetColorBlendState(const VezColorBlendState* pStateInfo);
VKAPI_ATTR void VKAPI_CALL vezCmdSetViewport(uint32_t firstViewport, uint32_t viewportCount, const VkViewport* pViewports);
VKAPI_ATTR void VKAPI_CALL vezCmdSetScissor(uint32_t firstScissor, uint32_t scissorCount, const VkRect2D* pScissors);
VKAPI_ATTR void VKAPI_CALL vezCmdSetLineWidth(float lineWidth);
VKAPI_ATTR void VKAPI_CALL vezCmdSetDepthBias(float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor);
VKAPI_ATTR void VKAPI_CALL vezCmdSetBlendConstants(const float blendConstants[4]);
VKAPI_ATTR void VKAPI_CALL vezCmdSetDepthBounds(float minDepthBounds, float maxDepthBounds);
VKAPI_ATTR void VKAPI_CALL vezCmdSetStencilCompareMask(VkStencilFaceFlags faceMask, uint32_t compareMask);
VKAPI_ATTR void VKAPI_CALL vezCmdSetStencilWriteMask(VkStencilFaceFlags faceMask, uint32_t writeMask);
VKAPI_ATTR void VKAPI_CALL vezCmdSetStencilReference(VkStencilFaceFlags faceMask, uint32_t reference);
VKAPI_ATTR void VKAPI_CALL vezCmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
VKAPI_ATTR void VKAPI_CALL vezCmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance);
VKAPI_ATTR void VKAPI_CALL vezCmdDrawIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride);
VKAPI_ATTR void VKAPI_CALL vezCmdDrawIndexedIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride);
VKAPI_ATTR void VKAPI_CALL vezCmdDispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
VKAPI_ATTR void VKAPI_CALL vezCmdDispatchIndirect(VkBuffer buffer, VkDeviceSize offset);
VKAPI_ATTR void VKAPI_CALL vezCmdCopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VezBufferCopy* pRegions);
VKAPI_ATTR void VKAPI_CALL vezCmdCopyImage(VkImage srcImage, VkImage dstImage, uint32_t regionCount, const VezImageCopy* pRegions);
VKAPI_ATTR void VKAPI_CALL vezCmdBlitImage(VkImage srcImage, VkImage dstImage, uint32_t regionCount, const VezImageBlit* pRegions, VkFilter filter);
VKAPI_ATTR void VKAPI_CALL vezCmdCopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, uint32_t regionCount, const VezBufferImageCopy* pRegions);
VKAPI_ATTR void VKAPI_CALL vezCmdCopyImageToBuffer(VkImage srcImage, VkBuffer dstBuffer, uint32_t regionCount, const VezBufferImageCopy* pRegions);
VKAPI_ATTR void VKAPI_CALL vezCmdUpdateBuffer(VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void* pData);
VKAPI_ATTR void VKAPI_CALL vezCmdFillBuffer(VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data);
VKAPI_ATTR void VKAPI_CALL vezCmdClearColorImage(VkImage image,  const VkClearColorValue* pColor, uint32_t rangeCount, const VezImageSubresourceRange* pRanges);
VKAPI_ATTR void VKAPI_CALL vezCmdClearDepthStencilImage(VkImage image, const VkClearDepthStencilValue* pDepthStencil, uint32_t rangeCount, const VezImageSubresourceRange* pRanges);
VKAPI_ATTR void VKAPI_CALL vezCmdClearAttachments(uint32_t attachmentCount, const VezClearAttachment* pAttachments, uint32_t rectCount, const VkClearRect* pRects);
VKAPI_ATTR void VKAPI_CALL vezCmdResolveImage(VkImage srcImage, VkImage dstImage, uint32_t regionCount, const VezImageResolve* pRegions);
VKAPI_ATTR void VKAPI_CALL vezCmdSetEvent(VkEvent event, VkPipelineStageFlags stageMask);
VKAPI_ATTR void VKAPI_CALL vezCmdResetEvent(VkEvent event, VkPipelineStageFlags stageMask);

#ifdef __cplusplus
}
#endif

#endif