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
#include <stdint.h>
#include <unordered_map>
#include "Utility/SpinLock.h"
#include "ObjectLookup.h"

namespace vez
{
    template <class Key, class Type>
    class ObjectLookupImpl
    {
    public:
        Type Get(Key key)
        {
            Type result = nullptr;
            m_spinLock.Lock();
            auto itr = m_objects.find(key);
            if (itr != m_objects.end())
                result = itr->second;
            m_spinLock.Unlock();
            return result;
        }

        void Add(Key key, Type object)
        {
            m_spinLock.Lock();
            m_objects.emplace(key, object);
            m_spinLock.Unlock();
        }

        void Remove(Key key)
        {
            m_spinLock.Lock();
            m_objects.erase(key);
            m_spinLock.Unlock();
        }

    private:
        std::unordered_map<Key, Type> m_objects;
        SpinLock m_spinLock;
    };

    #define OBJECT_LOOKUP_DEFINITION(Handle, Impl) \
    static ObjectLookupImpl<Handle, vez::Impl*> s_##Impl##Lookup;\
    \
    vez::Impl* GetImpl##Impl(Handle handle)\
    {\
        return s_##Impl##Lookup.Get(handle);\
    }\
    \
    void AddObjectImpl(Handle handle, vez::Impl* object)\
    {\
        s_##Impl##Lookup.Add(handle, object);\
    }\
    \
    void RemoveImpl##Impl(Handle handle)\
    {\
        s_##Impl##Lookup.Remove(handle);\
    }

    namespace ObjectLookup
    {
        OBJECT_LOOKUP_DEFINITION(VkInstance, Instance);
        OBJECT_LOOKUP_DEFINITION(VkPhysicalDevice, PhysicalDevice);
        OBJECT_LOOKUP_DEFINITION(VkDevice, Device);
        OBJECT_LOOKUP_DEFINITION(VkQueue, Queue);
        OBJECT_LOOKUP_DEFINITION(VkSwapchainKHR, Swapchain);
        OBJECT_LOOKUP_DEFINITION(VkCommandBuffer, CommandBuffer);
        OBJECT_LOOKUP_DEFINITION(VkShaderModule, ShaderModule);
        OBJECT_LOOKUP_DEFINITION(VkBuffer, Buffer);
        OBJECT_LOOKUP_DEFINITION(VkBufferView, BufferView);
        OBJECT_LOOKUP_DEFINITION(VkImage, Image);
        OBJECT_LOOKUP_DEFINITION(VkImageView, ImageView);
        OBJECT_LOOKUP_DEFINITION(VkFence, Fence);
    }    
}