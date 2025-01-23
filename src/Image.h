#pragma once
#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>

namespace hyper
{
	struct Image
	{
		vk::Image Image;
		vk::UniqueImageView ImageView;
		VmaAllocation Allocation;
		VmaAllocationInfo AllocationInfo;
		vk::Extent2D Extent;
		vk::Format Format;
	};

	Image CreateImage(VmaAllocator allocator, vk::Device device, int width, int height, vk::Format format, vk::ImageTiling tiling,
	vk::ImageUsageFlags usage, VmaMemoryUsage memoryUsage);
	void CopyImage(vk::CommandPool commandPool, vk::Device device, vk::Queue deviceQueue, vk::Buffer buffer, int width, int height, vk::Image dst);
	Image CreateImageStaged(VmaAllocator allocator, vk::CommandPool commandPool, vk::Device device, vk::Queue deviceQueue, int width, int height,
		const void* data, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage);
	Image CreateImageTexture(VmaAllocator allocator, vk::CommandPool commandPool, vk::Device device, vk::Queue deviceQueue, std::string path,
		vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage);
	void DestroyImage(VmaAllocator allocator, Image& image);
}