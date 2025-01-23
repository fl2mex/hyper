#pragma once
#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>

namespace hyper
{
	struct Buffer
	{
		vk::Buffer Buffer;
		VmaAllocation Allocation;
		VmaAllocationInfo AllocationInfo;
	};

	Buffer CreateBuffer(VmaAllocator allocator, vk::DeviceSize size, vk::BufferUsageFlags usage, VmaMemoryUsage memoryUsage);
	void CopyBuffer(vk::CommandPool commandPool, vk::Device device, vk::Queue deviceQueue, Buffer& src, Buffer& dst, vk::DeviceSize size);
	Buffer CreateBufferStaged(VmaAllocator allocator, vk::CommandPool commandPool, vk::Device device, vk::Queue deviceQueue, vk::DeviceSize size,
		vk::BufferUsageFlags usage, const void* data);
	void DestroyBuffer(VmaAllocator allocator, Buffer& buffer);
}