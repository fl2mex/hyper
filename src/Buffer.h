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

	Buffer CreateBuff(VmaAllocator allocator, vk::DeviceSize size, vk::BufferUsageFlags usage, VmaMemoryUsage memoryUsage);
	void CopyBuff(vk::CommandPool commandPool, vk::Device device, vk::Queue deviceQueue, Buffer& src, Buffer& dst, vk::DeviceSize size);
	Buffer UltimateCreateBuff(vk::CommandPool commandPool, vk::Device device, vk::Queue deviceQueue, VmaAllocator allocator, vk::DeviceSize size,
		vk::BufferUsageFlags usage, const void* data);
	void DestroyBuff(VmaAllocator allocator, Buffer& buffer);
}