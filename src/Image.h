#pragma once
#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>

namespace hyper
{
	struct Image
	{
		vk::Image Image;
		vk::ImageView ImageView;
		VmaAllocation Allocation;
		VmaAllocationInfo AllocationInfo;
		vk::Extent2D Extent;
		vk::Format Format;
	};

	Image CreateImage(VmaAllocator allocator, vk::Device device, vk::Extent2D extent, vk::Format format, vk::ImageTiling tiling,
	vk::ImageUsageFlags usage, VmaMemoryUsage memoryUsage);
	Image CreateImageStaged(VmaAllocator allocator, vk::CommandPool commandPool, vk::Device device, vk::Queue deviceQueue, vk::Extent2D extent,
		const void* data, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage);
	Image CreateImageTexture(VmaAllocator allocator, vk::CommandPool commandPool, vk::Device device, vk::Queue deviceQueue, std::string path,
		vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage);
	void CopyImage(vk::CommandPool commandPool, vk::Device device, vk::Queue deviceQueue, vk::Buffer buffer, vk::Extent2D extent, vk::Image dst);
	void DestroyImage(VmaAllocator allocator, vk::Device device, Image& image);
}