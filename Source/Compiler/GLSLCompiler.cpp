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
#include <string>
#include <fstream>
#include <glslang/Include/ShHandle.h>
#include <glslang/Include/revision.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/OSDependent/osinclude.h>
#include <SPIRV/GlslangToSpv.h>
#include <SPIRV/GLSL.std.450.h>
#include "ResourceLimits.h"
#include "GLSLCompiler.h"

enum TOptions {
    EOptionNone = 0,
    EOptionIntermediate = (1 << 0),
    EOptionSuppressInfolog = (1 << 1),
    EOptionMemoryLeakMode = (1 << 2),
    EOptionRelaxedErrors = (1 << 3),
    EOptionGiveWarnings = (1 << 4),
    EOptionLinkProgram = (1 << 5),
    EOptionMultiThreaded = (1 << 6),
    EOptionDumpConfig = (1 << 7),
    EOptionDumpReflection = (1 << 8),
    EOptionSuppressWarnings = (1 << 9),
    EOptionDumpVersions = (1 << 10),
    EOptionSpv = (1 << 11),
    EOptionHumanReadableSpv = (1 << 12),
    EOptionVulkanRules = (1 << 13),
    EOptionDefaultDesktop = (1 << 14),
    EOptionOutputPreprocessed = (1 << 15),
    EOptionOutputHexadecimal = (1 << 16),
    EOptionReadHlsl = (1 << 17),
    EOptionCascadingErrors = (1 << 18),
    EOptionAutoMapBindings = (1 << 19),
    EOptionFlattenUniformArrays = (1 << 20),
    EOptionNoStorageFormat = (1 << 21),
    EOptionKeepUncalled = (1 << 22),
    EOptionHlslOffsets = (1 << 23),
    EOptionHlslIoMapping = (1 << 24),
    EOptionAutoMapLocations = (1 << 25),
    EOptionDebug = (1 << 26),
    EOptionStdin = (1 << 27),
    EOptionOptimizeDisable = (1 << 28),
    EOptionOptimizeSize = (1 << 29),
};

struct ShaderCompUnit
{
    EShLanguage stage;
    std::string text;

    // Need to have a special constructors to adjust the fileNameList, since back end needs a list of ptrs
    ShaderCompUnit(EShLanguage istage, const std::string& itext)
    {
        stage = istage;
        text = itext;
    }

    ShaderCompUnit(const ShaderCompUnit &rhs)
    {
        stage = rhs.stage;
        text = rhs.text;
    }
};

void SetMessageOptions(int options, EShMessages& messages)
{
    if (options & EOptionRelaxedErrors)
        messages = (EShMessages)(messages | EShMsgRelaxedErrors);
    if (options & EOptionIntermediate)
        messages = (EShMessages)(messages | EShMsgAST);
    if (options & EOptionSuppressWarnings)
        messages = (EShMessages)(messages | EShMsgSuppressWarnings);
    if (options & EOptionSpv)
        messages = (EShMessages)(messages | EShMsgSpvRules);
    if (options & EOptionVulkanRules)
        messages = (EShMessages)(messages | EShMsgVulkanRules);
    if (options & EOptionOutputPreprocessed)
        messages = (EShMessages)(messages | EShMsgOnlyPreprocessor);
    if (options & EOptionReadHlsl)
        messages = (EShMessages)(messages | EShMsgReadHlsl);
    if (options & EOptionCascadingErrors)
        messages = (EShMessages)(messages | EShMsgCascadingErrors);
    if (options & EOptionKeepUncalled)
        messages = (EShMessages)(messages | EShMsgKeepUncalled);
}

EShLanguage MapShaderStage(VkShaderStageFlagBits stage)
{
    switch (stage)
    {
    case VK_SHADER_STAGE_VERTEX_BIT:
        return EShLangVertex;

    case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
        return EShLangTessControl;

    case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
        return EShLangTessEvaluation;

    case VK_SHADER_STAGE_GEOMETRY_BIT:
        return EShLangGeometry;

    case VK_SHADER_STAGE_FRAGMENT_BIT:
        return EShLangFragment;

    case VK_SHADER_STAGE_COMPUTE_BIT:
        return EShLangCompute;

    default:
        return EShLangVertex;
    }
}

namespace vez
{
    bool CompileGLSL2SPIRV(VkShaderStageFlagBits stage, const std::string& source, const std::string& entryPoint, std::vector<uint32_t>& spirv, std::string& infoLog)
    {
        // Get default built in resource limits.
        auto resourceLimits = glslang::DefaultTBuiltInResource;

        // Initialize glslang library.
        glslang::InitializeProcess();

        // Set message options.
        int options = EOptionSpv | EOptionVulkanRules | EOptionLinkProgram;
        EShMessages messages = EShMsgDefault;
        SetMessageOptions(options, messages);

        // Load GLSL source file into ShaderCompUnit object.
        ShaderCompUnit compUnit(MapShaderStage(stage), source);

        // Create shader from GLSL source.
        const char* fileNameList[1] = { "" };
        const char* shaderText = compUnit.text.c_str();
        glslang::TShader shader(compUnit.stage);
        shader.setStringsWithLengthsAndNames(&shaderText, nullptr, fileNameList, 1);
        shader.setEntryPoint(entryPoint.c_str());
        shader.setSourceEntryPoint(entryPoint.c_str());
        shader.setShiftSamplerBinding(0);
        shader.setShiftTextureBinding(0);
        shader.setShiftImageBinding(0);
        shader.setShiftUboBinding(0);
        shader.setShiftSsboBinding(0);
        shader.setFlattenUniformArrays(false);
        shader.setNoStorageFormat(false);
        if (!shader.parse(&resourceLimits, 100, false, messages))
        {
            infoLog = std::string(shader.getInfoLog()) + "\n" + std::string(shader.getInfoDebugLog());
            return false;
        }

        // Add shader to new program object.
        glslang::TProgram program;
        program.addShader(&shader);

        // Link program.
        if (!program.link(messages))
        {
            infoLog = std::string(program.getInfoLog()) + "\n" + std::string(program.getInfoDebugLog());
            return false;
        }

        // Map IO for SPIRV generation.
        if (!program.mapIO())
        {
            infoLog = std::string(program.getInfoLog()) + "\n" + std::string(program.getInfoDebugLog());
            return false;
        }

        // Save any info log that was generated.
        if (shader.getInfoLog())
            infoLog += std::string(shader.getInfoLog()) + "\n" + std::string(shader.getInfoDebugLog()) + "\n";

        if (program.getInfoLog())
            infoLog += std::string(program.getInfoLog()) + "\n" + std::string(program.getInfoDebugLog());

        // Translate to SPIRV.
        if (program.getIntermediate(compUnit.stage))
        {
            std::string warningsErrors;
            spv::SpvBuildLogger logger;
            glslang::GlslangToSpv(*program.getIntermediate(compUnit.stage), spirv, &logger);
            infoLog += logger.getAllMessages() + "\n";
        }

        // Shutdown glslang library.
        glslang::FinalizeProcess();

        return true;
    }
}
