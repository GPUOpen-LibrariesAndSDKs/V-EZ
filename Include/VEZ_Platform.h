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
#ifndef VEZ_PLATFORM_H_
#define VEZ_PLATFORM_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
    #define VKAPI_ATTR
    #define VKAPI_CALL __stdcall
    #define VKAPI_PTR VKAPI_CALL
#elif
    #define VKAPI_ATTR
    #define VKAPI_CALL
    #define VKAPI_PTR VKAPI_CALL
#include <xcb/xcb.h>
#endif

#include <stdint.h>

#ifdef __cplusplus
}
#endif

#ifdef VK_USE_PLATFORM_WIN32_KHR
#include <Windows.h>
#endif

#ifdef VK_USE_PLATFORM_XLIB_KHR
#include <X11/Xlib.h>
#endif

#ifdef VK_USE_PLATFORM_XCB_KHR
#include <xcb/xcb.h>
#endif

#endif