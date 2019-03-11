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
#include <algorithm>
#include <unordered_map>
#include <spirv_glsl.hpp>
#include "SPIRVReflection.h"

namespace vez
{
    static std::unordered_map<spirv_cross::SPIRType::BaseType, VezBaseType> spirvTypeToVezBaseType = {
        { spirv_cross::SPIRType::Boolean, VEZ_BASE_TYPE_BOOL },
        { spirv_cross::SPIRType::Char, VEZ_BASE_TYPE_CHAR },
        { spirv_cross::SPIRType::Int, VEZ_BASE_TYPE_INT },
        { spirv_cross::SPIRType::UInt, VEZ_BASE_TYPE_UINT },
        { spirv_cross::SPIRType::Half, VEZ_BASE_TYPE_HALF },
        { spirv_cross::SPIRType::Float, VEZ_BASE_TYPE_FLOAT },
        { spirv_cross::SPIRType::Double, VEZ_BASE_TYPE_DOUBLE },
        { spirv_cross::SPIRType::Struct, VEZ_BASE_TYPE_STRUCT },
    };

    static VezMemberInfo* ParseMembers(spirv_cross::CompilerGLSL& compiler, const spirv_cross::SPIRType& spirType)
    {
        // Iterate over member hierarchy.
        VezMemberInfo* pFirstMemberInfo = nullptr;
        VezMemberInfo* pPrevMemberInfo = nullptr;
        for (auto i = 0U; i < spirType.member_types.size(); ++i)
        {
            // Validate member is of a supported type.
            const auto& memberType = compiler.get_type(spirType.member_types[i]);
            if (spirvTypeToVezBaseType.find(memberType.basetype) == spirvTypeToVezBaseType.end())
                continue;

            // Create a new VezMemberInfo entry.
            auto mi = new VezMemberInfo;
            mi->baseType = spirvTypeToVezBaseType.at(memberType.basetype);
            mi->offset = compiler.type_struct_member_offset(spirType, i);
            mi->size = compiler.get_declared_struct_member_size(spirType, i);
            mi->vecSize = memberType.vecsize;
            mi->columns = memberType.columns;
            mi->arraySize = (memberType.array.size() == 0) ? 1 : memberType.array[0];
            mi->pNext = nullptr;
            mi->pMembers = nullptr;
            memset(mi->name, 0, VK_MAX_DESCRIPTION_SIZE);

            const auto& name = compiler.get_member_name(spirType.self, i);
            memcpy(mi->name, name.c_str(), name.size());

            // Link current and last member infos.
            if (pPrevMemberInfo == nullptr)
                pPrevMemberInfo = mi;
            else
                pPrevMemberInfo->pNext = mi;

            // Keep pointer to first member info.
            if (pFirstMemberInfo == nullptr)
                pFirstMemberInfo = mi;

            // Update previous member.
            pPrevMemberInfo = mi;

            // Recursively process members that are structs.
            if (memberType.basetype == spirv_cross::SPIRType::Struct)
                mi->pMembers = ParseMembers(compiler, memberType);
        }

        // Return the first member info created.
        return pFirstMemberInfo;
    }

    class CustomCompiler : public spirv_cross::CompilerGLSL
    {
    public:
        CustomCompiler(const std::vector<uint32_t>& spirv)
            : spirv_cross::CompilerGLSL(spirv)
        {

        }

        VkAccessFlags GetAccessFlags(const spirv_cross::SPIRType& type)
        {
            // SPIRV-Cross hack to get the correct readonly and writeonly attributes on ssbos.
            // This isn't working correctly via Compiler::get_decoration(id, spv::DecorationNonReadable) for example.
            // So the code below is extracted from private methods within spirv_cross.cpp.
            // The spirv_cross executable correctly outputs the attributes when converting spirv back to GLSL,
            // but it's own reflection code does not :-(
            auto all_members_flag_mask = spirv_cross::Bitset(~0ULL);
            for (auto i = 0U; i < type.member_types.size(); ++i)
                all_members_flag_mask.merge_and(get_member_decoration_bitset(type.self, i));
            
            auto base_flags = ir.meta[type.self].decoration.decoration_flags;
            base_flags.merge_or(spirv_cross::Bitset(all_members_flag_mask));

            VkAccessFlags access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            if (base_flags.get(spv::DecorationNonReadable))
                access = VK_ACCESS_SHADER_WRITE_BIT;
            else if (base_flags.get(spv::DecorationNonWritable))
                access = VK_ACCESS_SHADER_READ_BIT;

            return access;
        }
    };

    bool SPIRVReflectResources(const std::vector<uint32_t>& spirv, VkShaderStageFlagBits stage, std::vector<VezPipelineResource>& shaderResources)
    {
        // Parse SPIRV binary.
        CustomCompiler compiler(spirv);
        spirv_cross::CompilerGLSL::Options opts = compiler.get_common_options();
        opts.enable_420pack_extension = true;
        compiler.set_common_options(opts);

        // Reflect on all resource bindings.
        auto resources = compiler.get_shader_resources();

        // Extract per stage inputs.
        for (auto& resource : resources.stage_inputs)
        {
            const auto& spirType = compiler.get_type_from_variable(resource.id);

            VezPipelineResource pipelineResource = {};
            pipelineResource.stages = stage;
            pipelineResource.resourceType = VEZ_PIPELINE_RESOURCE_TYPE_INPUT;
            pipelineResource.access = VK_ACCESS_SHADER_READ_BIT;
            pipelineResource.location = compiler.get_decoration(resource.id, spv::DecorationLocation);
            pipelineResource.vecSize = spirType.vecsize;
            pipelineResource.columns = spirType.columns;
            pipelineResource.arraySize = (spirType.array.size() == 0) ? 1 : spirType.array[0];

            auto it = spirvTypeToVezBaseType.find(spirType.basetype);
            if (it == spirvTypeToVezBaseType.end())
                continue;

            pipelineResource.baseType = it->second;
            memcpy(pipelineResource.name, resource.name.c_str(), std::min(sizeof(pipelineResource.name), resource.name.length()));
            shaderResources.push_back(pipelineResource);
        }

        // Extract per stage outputs.
        for (auto& resource : resources.stage_outputs)
        {
            const auto& spirType = compiler.get_type_from_variable(resource.id);

            VezPipelineResource pipelineResource = {};
            pipelineResource.stages = stage;
            pipelineResource.resourceType = VEZ_PIPELINE_RESOURCE_TYPE_OUTPUT;
            pipelineResource.access = VK_ACCESS_SHADER_WRITE_BIT;
            pipelineResource.location = compiler.get_decoration(resource.id, spv::DecorationLocation);
            pipelineResource.vecSize = spirType.vecsize;
            pipelineResource.columns = spirType.columns;
            pipelineResource.arraySize = (spirType.array.size() == 0) ? 1 : spirType.array[0];

            auto it = spirvTypeToVezBaseType.find(spirType.basetype);
            if (it == spirvTypeToVezBaseType.end())
                continue;

            pipelineResource.baseType = it->second;
            memcpy(pipelineResource.name, resource.name.c_str(), std::min(sizeof(pipelineResource.name), resource.name.length()));
            shaderResources.push_back(pipelineResource);
        }

        // Extract uniform buffers.
        for (auto& resource : resources.uniform_buffers)
        {
            const auto& spirType = compiler.get_type_from_variable(resource.id);

            VezPipelineResource pipelineResource = {};
            pipelineResource.stages = stage;
            pipelineResource.resourceType = VEZ_PIPELINE_RESOURCE_TYPE_UNIFORM_BUFFER;
            pipelineResource.access = VK_ACCESS_UNIFORM_READ_BIT;
            pipelineResource.set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            pipelineResource.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
            pipelineResource.arraySize = (spirType.array.size() == 0) ? 1 : spirType.array[0];
            pipelineResource.size = compiler.get_declared_struct_size(spirType);
            memcpy(pipelineResource.name, resource.name.c_str(), std::min(sizeof(pipelineResource.name), resource.name.length()));
            pipelineResource.pMembers = ParseMembers(compiler, spirType);
            shaderResources.push_back(pipelineResource);
        }

        // Extract storage buffers.
        for (auto& resource : resources.storage_buffers)
        {
            const auto& spirType = compiler.get_type_from_variable(resource.id);

            VezPipelineResource pipelineResource = {};
            pipelineResource.stages = stage;
            pipelineResource.resourceType = VEZ_PIPELINE_RESOURCE_TYPE_STORAGE_BUFFER;
            pipelineResource.access = compiler.GetAccessFlags(spirType);
            pipelineResource.set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            pipelineResource.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
            pipelineResource.arraySize = (spirType.array.size() == 0) ? 1 : spirType.array[0];
            pipelineResource.size = compiler.get_declared_struct_size(spirType);
            memcpy(pipelineResource.name, resource.name.c_str(), std::min(sizeof(pipelineResource.name), resource.name.length()));
            pipelineResource.pMembers = ParseMembers(compiler, spirType);
            shaderResources.push_back(pipelineResource);
        }

        // Extract separate samplers.
        for (auto& resource : resources.separate_samplers)
        {
            const auto& spirType = compiler.get_type_from_variable(resource.id);

            VezPipelineResource pipelineResource = {};
            pipelineResource.stages = stage;
            pipelineResource.resourceType = VEZ_PIPELINE_RESOURCE_TYPE_SAMPLER;
            pipelineResource.access = VK_ACCESS_SHADER_READ_BIT;
            pipelineResource.set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            pipelineResource.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
            pipelineResource.arraySize = (spirType.array.size() == 0) ? 1 : spirType.array[0];
            memcpy(pipelineResource.name, resource.name.c_str(), std::min(sizeof(pipelineResource.name), resource.name.length()));
            shaderResources.push_back(pipelineResource);
        }

        // Extract sampled images (combined sampler + image or texture buffers).
        for (auto& resource : resources.sampled_images)
        {
            const auto& spirType = compiler.get_type_from_variable(resource.id);

            VezPipelineResource pipelineResource = {};
            pipelineResource.stages = stage;
            pipelineResource.resourceType = (spirType.image.dim == spv::Dim::DimBuffer) ? VEZ_PIPELINE_RESOURCE_TYPE_UNIFORM_TEXEL_BUFFER : VEZ_PIPELINE_RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER;
            pipelineResource.access = VK_ACCESS_SHADER_READ_BIT;
            pipelineResource.set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            pipelineResource.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
            pipelineResource.arraySize = (spirType.array.size() == 0) ? 1 : spirType.array[0];
            memcpy(pipelineResource.name, resource.name.c_str(), std::min(sizeof(pipelineResource.name), resource.name.length()));
            shaderResources.push_back(pipelineResource);
        }

        // Extract seperate images ('sampled' in vulkan terminology or no sampler attached).
        for (auto& resource : resources.separate_images)
        {
            const auto& spirType = compiler.get_type_from_variable(resource.id);

            VezPipelineResource pipelineResource = {};
            pipelineResource.stages = stage;
            pipelineResource.resourceType = VEZ_PIPELINE_RESOURCE_TYPE_SAMPLED_IMAGE;
            pipelineResource.access = VK_ACCESS_SHADER_READ_BIT;
            pipelineResource.set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            pipelineResource.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
            pipelineResource.arraySize = (spirType.array.size() == 0) ? 1 : spirType.array[0];
            memcpy(pipelineResource.name, resource.name.c_str(), std::min(sizeof(pipelineResource.name), resource.name.length()));
            shaderResources.push_back(pipelineResource);
        }

        // Extract storage images.
        for (auto& resource : resources.storage_images)
        {
            auto nonReadable = compiler.get_decoration(resource.id, spv::DecorationNonReadable);
            auto nonWriteable = compiler.get_decoration(resource.id, spv::DecorationNonWritable);
            VkAccessFlags access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            if (nonReadable) access = VK_ACCESS_SHADER_WRITE_BIT;
            else if (nonWriteable) access = VK_ACCESS_SHADER_READ_BIT;

            const auto& spirType = compiler.get_type_from_variable(resource.id);

            VezPipelineResource pipelineResource = {};
            pipelineResource.stages = stage;
            pipelineResource.resourceType = (spirType.image.dim == spv::Dim::DimBuffer) ? VEZ_PIPELINE_RESOURCE_TYPE_STORAGE_TEXEL_BUFFER : VEZ_PIPELINE_RESOURCE_TYPE_STORAGE_IMAGE;
            pipelineResource.access = access;
            pipelineResource.set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            pipelineResource.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
            pipelineResource.arraySize = (spirType.array.size() == 0) ? 1 : spirType.array[0];
            memcpy(pipelineResource.name, resource.name.c_str(), std::min(sizeof(pipelineResource.name), resource.name.length()));
            shaderResources.push_back(pipelineResource);
        }

        // Extract subpass inputs.
        for (auto& resource : resources.subpass_inputs)
        {
            VezPipelineResource pipelineResource = {};
            pipelineResource.resourceType = VEZ_PIPELINE_RESOURCE_TYPE_INPUT_ATTACHMENT;
            pipelineResource.stages = VK_SHADER_STAGE_FRAGMENT_BIT;
            pipelineResource.access = VK_ACCESS_SHADER_READ_BIT;
            pipelineResource.inputAttachmentIndex = compiler.get_decoration(resource.id, spv::DecorationInputAttachmentIndex);
            pipelineResource.set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            pipelineResource.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
            pipelineResource.arraySize = 1;
            memcpy(pipelineResource.name, resource.name.c_str(), std::min(sizeof(pipelineResource.name), resource.name.length()));
            shaderResources.push_back(pipelineResource);
        }

        // Extract push constants.
        for (auto& resource : resources.push_constant_buffers)
        {
            const auto& spirType = compiler.get_type_from_variable(resource.id);

            // Get the start offset of the push constant buffer since this will differ between shader stages.
            uint32_t offset = ~0;
            for (auto i = 0U; i < spirType.member_types.size(); ++i)
            {
                auto memberType = compiler.get_type(spirType.member_types[i]);
                offset = std::min(offset, compiler.get_member_decoration(spirType.self, i, spv::DecorationOffset));
            }

            VezPipelineResource pipelineResource = {};
            pipelineResource.stages = stage;
            pipelineResource.resourceType = VEZ_PIPELINE_RESOURCE_TYPE_PUSH_CONSTANT_BUFFER;
            pipelineResource.access = VK_ACCESS_SHADER_READ_BIT;
            pipelineResource.offset = offset;
            pipelineResource.size = compiler.get_declared_struct_size(spirType);
            memcpy(pipelineResource.name, resource.name.c_str(), std::min(sizeof(pipelineResource.name), resource.name.length()));
            pipelineResource.pMembers = ParseMembers(compiler, spirType);
            shaderResources.push_back(pipelineResource);
        }

        return true;
    }   

    void DestroyMemberInfos(VezMemberInfo* pMemberInfo)
    {
        if (!pMemberInfo)
            return;

        if (pMemberInfo->pMembers)
            DestroyMemberInfos(const_cast<VezMemberInfo*>(pMemberInfo->pMembers));

        if (pMemberInfo->pNext)
            DestroyMemberInfos(const_cast<VezMemberInfo*>(pMemberInfo->pNext));

        delete pMemberInfo;
    }
}