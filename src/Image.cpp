#include "Image.h"

#include <stb_image.h>

#include "Buffer.h"

namespace hyper
{
	Image CreateImage(VmaAllocator& allocator, vk::Device& device, vk::Extent2D extent, vk::Format format, vk::ImageTiling tiling,
		vk::ImageUsageFlags usage, VmaMemoryUsage memoryUsage)
	{
		Image image;
		image.Format = format;
		image.Extent = extent;
		vk::ImageCreateInfo imageInfo{ {}, vk::ImageType::e2D, format, { extent.width, extent.height, 1 },
		1, 1, vk::SampleCountFlagBits::e1, tiling, usage, vk::SharingMode::eExclusive };
		VmaAllocationCreateInfo allocCreateInfo{ VMA_ALLOCATION_CREATE_MAPPED_BIT, memoryUsage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
		vmaCreateImage(allocator, reinterpret_cast<VkImageCreateInfo*>(&imageInfo), &allocCreateInfo, reinterpret_cast<VkImage*>(&image.Image),
			&image.Allocation, &image.AllocationInfo);

		vk::ImageAspectFlags aspectFlag = vk::ImageAspectFlagBits::eColor;
		if (format == vk::Format::eD32Sfloat)
			aspectFlag = vk::ImageAspectFlagBits::eDepth;

		vk::ImageViewUsageCreateInfo imageViewUsageCreateInfo{ usage };
		vk::ImageViewCreateInfo imageViewCreateInfo{ {}, image.Image, vk::ImageViewType::e2D, format, {},
		{ aspectFlag, 0, 1, 0, 1 }, &imageViewUsageCreateInfo };

		image.ImageView = device.createImageView(imageViewCreateInfo);
		return image;
	}

	Image CreateImageStaged(VmaAllocator& allocator, vk::CommandPool& commandPool, vk::Device& device, vk::Queue& deviceQueue, vk::Extent2D extent,
		const void* data, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage)
	{
		Buffer buffer = CreateBuffer(allocator, extent.width * extent.height * 4, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
		memcpy(buffer.AllocationInfo.pMappedData, data, extent.width * extent.height * 4);
		Image image = CreateImage(allocator, device, extent, format, tiling, usage | vk::ImageUsageFlagBits::eTransferDst,
			VMA_MEMORY_USAGE_GPU_ONLY);
		CopyImage(commandPool, device, deviceQueue, buffer.Buffer, extent, image.Image);
		DestroyBuffer(allocator, buffer);
		return image;
	}

	Image CreateImageTexture(VmaAllocator& allocator, vk::CommandPool& commandPool, vk::Device& device, vk::Queue& deviceQueue, std::string path,
		vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage)
	{
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		vk::DeviceSize size = static_cast<vk::DeviceSize>(texWidth * texHeight * 4);
		Buffer buffer = CreateBuffer(allocator, size, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
		memcpy(buffer.AllocationInfo.pMappedData, pixels, size);
		stbi_image_free(pixels);
		Image image = CreateImage(allocator, device, { (uint32_t)texWidth, (uint32_t)texHeight }, format, tiling, usage | vk::ImageUsageFlagBits::eTransferDst,
			VMA_MEMORY_USAGE_GPU_ONLY);
		CopyImage(commandPool, device, deviceQueue, buffer.Buffer, { (uint32_t)texWidth, (uint32_t)texHeight }, image.Image);
		DestroyBuffer(allocator, buffer);
		return image;
	}

	void CopyImage(vk::CommandPool& commandPool, vk::Device& device, vk::Queue& deviceQueue, vk::Buffer& buffer, vk::Extent2D extent, vk::Image& dst)
	{
		vk::UniqueFence fence = device.createFenceUnique({ vk::FenceCreateFlagBits::eSignaled });

		vk::CommandBufferAllocateInfo allocInfo{ commandPool, vk::CommandBufferLevel::ePrimary, 1 };
		std::vector<vk::UniqueCommandBuffer> commandBuffer = device.allocateCommandBuffersUnique(allocInfo);
		vk::BufferImageCopy copyRegion{ 0, 0, 0, {vk::ImageAspectFlagBits::eColor, 0, 0, 1 }, { 0, 0, 0 },
			{ extent.width, extent.height, 1 } };

		vk::ImageMemoryBarrier2 topImageMemoryBarrier2{ vk::PipelineStageFlagBits2::eTopOfPipe, vk::AccessFlagBits2::eNone,
			vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eUndefined,
			vk::ImageLayout::eTransferDstOptimal, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, dst,
			vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } };
		vk::ImageMemoryBarrier2 bottomImageMemoryBarrier2{ vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite,
			vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderRead, vk::ImageLayout::eTransferDstOptimal,
			vk::ImageLayout::eShaderReadOnlyOptimal, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, dst,
			vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } };

		static_cast<void>(device.resetFences(1, &fence.get()));

		commandBuffer[0]->begin(vk::CommandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
		commandBuffer[0]->pipelineBarrier2({ vk::DependencyFlagBits::eByRegion, 0, nullptr, 0, nullptr, 1, &topImageMemoryBarrier2 });
		commandBuffer[0]->copyBufferToImage(buffer, dst, vk::ImageLayout::eTransferDstOptimal, 1, &copyRegion);
		commandBuffer[0]->pipelineBarrier2({ vk::DependencyFlagBits::eByRegion, 0, nullptr, 0, nullptr, 1, &bottomImageMemoryBarrier2 });
		commandBuffer[0]->end();

		vk::CommandBufferSubmitInfo commandBufferSubmitInfo{ commandBuffer[0].get() };
		deviceQueue.submit2(vk::SubmitInfo2{ {}, 0, nullptr, 1, &commandBufferSubmitInfo, 0, nullptr }, fence.get());
		static_cast<void>(device.waitForFences(1, &fence.get(), VK_TRUE, UINT64_MAX));
	}

	void DestroyImage(VmaAllocator& allocator, vk::Device& device, Image& image)
	{
		device.destroyImageView(image.ImageView);
		vmaDestroyImage(allocator, image.Image, image.Allocation);
	}
}