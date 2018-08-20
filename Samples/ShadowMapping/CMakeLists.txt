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

set(COMMON_SOURCES ../Common/AppBase.cpp ../Common/AppBase.h ../Common/Model.cpp ../Common/Model.h)
set(SOURCES main.cpp ShadowMapping.cpp ShadowMapping.h)

source_group("Common" FILES ${COMMON_SOURCES})
source_group("" FILES ${SOURCES})

add_executable(ShadowMapping ${COMMON_SOURCES} ${SOURCES})

target_link_libraries(ShadowMapping
    PRIVATE assimp
    PRIVATE glfw
    PRIVATE Vulkan::Vulkan
    PRIVATE VEZ
)

target_compile_features(ShadowMapping PRIVATE cxx_std_14)

target_compile_definitions(ShadowMapping PUBLIC _CRT_SECURE_NO_WARNINGS STB_IMAGE_IMPLEMENTATION GLM_FORCE_SWIZZLE GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_RADIANS GLFW_INCLUDE_NONE)

set_target_properties(ShadowMapping PROPERTIES FOLDER Samples)

set_target_properties(ShadowMapping PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${OUTPUT_DIRECTORY}")
