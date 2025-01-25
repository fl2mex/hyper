#pragma once
#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>

namespace hyper
{
	struct Buffer
	{
		vk::Buffer Buffer;
		VmaAllocation Allocation = 0;
		VmaAllocationInfo AllocationInfo = {0};
	};

	Buffer CreateBuffer(VmaAllocator& allocator, vk::DeviceSize size, vk::BufferUsageFlags usage, VmaMemoryUsage memoryUsage);
	Buffer CreateBufferStaged(VmaAllocator& allocator, vk::CommandPool& commandPool, vk::Device& device, vk::Queue& deviceQueue, vk::DeviceSize size,
		vk::BufferUsageFlags usage, const void* data);
	void CopyBuffer(vk::CommandPool& commandPool, vk::Device& device, vk::Queue& deviceQueue, Buffer& src, Buffer& dst, vk::DeviceSize size);
	void DestroyBuffer(VmaAllocator& allocator, Buffer& buffer);
}