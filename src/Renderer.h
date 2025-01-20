#pragma once
#include <iostream>
#include <vulkan/vulkan.hpp>	// >1.3.275 and I didn't originally notice because I was ON 1.3.275
#include <GLFW/glfw3.h>			// and only then noticed when I had to reinstall the SDK :)))))
#include <stb_image.h>
#include <vma/vk_mem_alloc.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp> // This came with the vulkan sdk but if you don't have it, just download it from github
#include <glm/gtc/matrix_transform.hpp>

#include "Spec.h"
#include "Logger.h"

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

	struct Buffer
	{
		vk::Buffer Buffer;
		VmaAllocation Allocation;
		VmaAllocationInfo AllocationInfo;
	};

	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 colour;
		glm::vec2 texCoord;

		static vk::VertexInputBindingDescription2EXT getBindingDescription()
		{
			return vk::VertexInputBindingDescription2EXT{ 0, sizeof(Vertex), vk::VertexInputRate::eVertex, 1 };
		}
		static std::array<vk::VertexInputAttributeDescription2EXT, 3> getAttributeDescriptions()
		{
			std::array<vk::VertexInputAttributeDescription2EXT, 3> attributeDescriptions;
			attributeDescriptions[0] = vk::VertexInputAttributeDescription2EXT{ 0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position) };
			attributeDescriptions[1] = vk::VertexInputAttributeDescription2EXT{ 1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, colour) };
			attributeDescriptions[2] = vk::VertexInputAttributeDescription2EXT{ 2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord) };
			return attributeDescriptions;
		}
	};

	struct UniformBufferObject
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};

	class Renderer
	{
	public:
		void SetupRenderer(Spec _spec = {}, GLFWwindow* _window = {});
		void DrawFrame();
		~Renderer();

		bool m_FramebufferResized = false;

	private:
		std::vector<Vertex> vertices
		{
			{{-0.5f, -0.5f,  0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
			{{ 0.5f, -0.5f,  0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
			{{ 0.5f,  0.5f,  0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
			{{-0.5f,  0.5f,  0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

			{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
			{{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
			{{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
			{{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
		};

		std::vector<uint16_t> indices
		{
			0, 1, 2, 2, 3, 0,
			4, 5, 6, 6, 7, 4
		};

		void RecreateSwapchain();
		void RecreateCommandBuffers();
		
		// Same with image, texture, mesh
		void CreateImage(int width, int height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags imageUsage, vk::MemoryPropertyFlags properties,
			vk::Image& image, vk::DeviceMemory& imageMemory);
		void CopyImage(vk::Buffer srcBuffer, vk::Image dstImage, int width, int height);

		uint32_t currentFrame = 0;
		double previousTime = 0.0;
		uint32_t frameCount = 0;

		Spec m_Spec;
		GLFWwindow* m_Window{};

		vk::UniqueInstance m_Instance{};
		
		vk::detail::DispatchLoaderDynamic m_DLDI;
		vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::detail::DispatchLoaderDynamic> m_DebugMessenger{};

		vk::UniqueSurfaceKHR m_Surface{};

		vk::PhysicalDevice m_PhysicalDevice{};
		vk::UniqueDevice m_Device{};
		
		VmaAllocator m_Allocator;

		vk::Queue m_DeviceQueue{};
		vk::Queue m_PresentQueue{};

		vk::Format m_SwapchainImageFormat{};
		vk::Extent2D m_SwapchainExtent{};
		uint32_t m_SwapchainImageCount{};
		
		vk::UniqueSwapchainKHR m_Swapchain{};

		std::vector<vk::Image> m_SwapchainImages{}; // Can this be turned unique as well?
		std::vector<vk::UniqueImageView> m_ImageViews{};

		vk::UniqueDescriptorSetLayout m_DescriptorSetLayout{};
		vk::UniquePipelineLayout m_PipelineLayout{};

		std::vector<vk::UniqueHandle<vk::ShaderEXT, vk::detail::DispatchLoaderDynamic>> m_Shaders;

		vk::UniqueCommandPool m_CommandPool{};

		vk::UniqueDescriptorPool m_DescriptorPool{};
		std::vector<vk::UniqueDescriptorSet> m_DescriptorSets{};

		Buffer m_VertexB;
		Buffer m_IndexB;
		vk::DeviceAddress m_VertexA;

		std::vector<Buffer> m_UniformBuffs;
		
		Image m_TextureImg;

		vk::UniqueSampler m_Sampler;

		Image m_DepthImg;

		std::vector<vk::UniqueCommandBuffer> m_CommandBuffers{};

		vk::UniqueFence m_InFlightFence{};
		vk::UniqueSemaphore m_ImageAvailableSemaphore{};
		vk::UniqueSemaphore m_RenderFinishedSemaphore{};

		Buffer CreateBuff(vk::DeviceSize size, vk::BufferUsageFlags usage, VmaMemoryUsage memoryUsage) const
		{
			vk::BufferCreateInfo bufferInfo{ {}, size, usage, vk::SharingMode::eExclusive };
			VmaAllocationCreateInfo allocCreateInfo{ VMA_ALLOCATION_CREATE_MAPPED_BIT, memoryUsage };
			Buffer buffer;
			VkResult e = vmaCreateBuffer(m_Allocator, reinterpret_cast<VkBufferCreateInfo*>(&bufferInfo), &allocCreateInfo,
				reinterpret_cast<VkBuffer*>(&buffer.Buffer), &buffer.Allocation, &buffer.AllocationInfo);
			if (e != 0) Logger::logger->Log("Buffer creation error code: " + std::to_string(e));
			return buffer;
		}

		void CopyBuff(Buffer& src, Buffer& dst, vk::DeviceSize size)
		{
			vk::BufferCopy copyRegion{ 0, 0, size };
			vk::CommandBufferAllocateInfo allocInfo{ m_CommandPool.get(), vk::CommandBufferLevel::ePrimary, 1 };
			std::vector<vk::UniqueCommandBuffer> commandBuffer = m_Device->allocateCommandBuffersUnique(allocInfo);
			commandBuffer[0]->begin(vk::CommandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
			commandBuffer[0]->copyBuffer(src.Buffer, dst.Buffer, 1, &copyRegion);
			commandBuffer[0]->end();
			vk::SubmitInfo submitInfo{ 0, nullptr, nullptr, 1, &commandBuffer[0].get() };
			m_DeviceQueue.submit(submitInfo, nullptr);
			m_DeviceQueue.waitIdle();
		}

		Buffer UltimateCreateBuff(vk::DeviceSize size, vk::BufferUsageFlags usage, const void* data)
		{
			Buffer buffer = CreateBuff(size, usage | vk::BufferUsageFlagBits::eTransferDst, VMA_MEMORY_USAGE_GPU_ONLY);
			Buffer stagingBuffer = CreateBuff(size, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY);
			void* bufferData;
			vmaMapMemory(m_Allocator, stagingBuffer.Allocation, &bufferData);
			memcpy(bufferData, data, size);
			vmaUnmapMemory(m_Allocator, stagingBuffer.Allocation);
			CopyBuff(stagingBuffer, buffer, size);
			DestroyBuff(stagingBuffer);
			return buffer;
		}

		void DestroyBuff(Buffer& buffer) const
		{
			vmaDestroyBuffer(m_Allocator, buffer.Buffer, buffer.Allocation);
		}

		Image CreateImg(int width, int height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, VmaMemoryUsage memoryUsage)
		{
			Image image;
			image.Format = format;
			image.Extent = vk::Extent2D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
			vk::ImageCreateInfo imageInfo{ {}, vk::ImageType::e2D, format, { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 },
			1, 1, vk::SampleCountFlagBits::e1, tiling, usage, vk::SharingMode::eExclusive };
			VmaAllocationCreateInfo allocCreateInfo{ VMA_ALLOCATION_CREATE_MAPPED_BIT, memoryUsage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
			VkResult e = vmaCreateImage(m_Allocator, reinterpret_cast<VkImageCreateInfo*>(&imageInfo), &allocCreateInfo, reinterpret_cast<VkImage*>(&image.Image),
				&image.Allocation, &image.AllocationInfo);
			if (e != 0) Logger::logger->Log("Image creation error code: " + std::to_string(e));

			vk::ImageAspectFlags aspectFlag = vk::ImageAspectFlagBits::eColor;
			if (format == vk::Format::eD32Sfloat)
				aspectFlag = vk::ImageAspectFlagBits::eDepth;

			vk::ImageViewCreateInfo imageViewCreateInfo{ {}, image.Image, vk::ImageViewType::e2D, format, {},
			{ aspectFlag, 0, 1, 0, 1 } };
			image.ImageView = m_Device->createImageViewUnique(imageViewCreateInfo);
			return image;
		}
				
		void CopyImg(vk::Buffer buffer, int width, int height, vk::Image dst)
		{
			vk::CommandBufferAllocateInfo allocInfo{ m_CommandPool.get(), vk::CommandBufferLevel::ePrimary, 1 };
			std::vector<vk::UniqueCommandBuffer> commandBuffer = m_Device->allocateCommandBuffersUnique(allocInfo);
			vk::BufferImageCopy copyRegion{ 0, 0, 0, {vk::ImageAspectFlagBits::eColor, 0, 0, 1 }, { 0, 0, 0 },
				{ static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 } };

			vk::ImageMemoryBarrier topImageMemoryBarrier{ vk::AccessFlagBits::eNone, vk::AccessFlagBits::eTransferWrite,
				vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
				dst, vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } };
			vk::ImageMemoryBarrier bottomImageMemoryBarrier{ vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead,
				vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
				dst, vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } };

			commandBuffer[0]->begin(vk::CommandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
			commandBuffer[0]->pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer,
				{}, 0, nullptr, 0, nullptr, 1, &topImageMemoryBarrier);
			commandBuffer[0]->copyBufferToImage(buffer, dst, vk::ImageLayout::eTransferDstOptimal, 1, &copyRegion);
			commandBuffer[0]->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
				{}, 0, nullptr, 0, nullptr, 1, &bottomImageMemoryBarrier);
			commandBuffer[0]->end();
			vk::SubmitInfo submitInfo{ 0, nullptr, nullptr, 1, &commandBuffer[0].get() };
			m_DeviceQueue.submit(submitInfo, nullptr);
			m_DeviceQueue.waitIdle();
		}
		
		Image UltimateCreateImg(std::string path, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage)
		{
			int texWidth, texHeight, texChannels;
			stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
			vk::DeviceSize size = static_cast<vk::DeviceSize>(texWidth * texHeight * 4);
			Buffer buffer = CreateBuff(size, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
			void* imageData;
			vmaMapMemory(m_Allocator, buffer.Allocation, &imageData);
			memcpy(imageData, pixels, size);
			vmaUnmapMemory(m_Allocator, buffer.Allocation);
			stbi_image_free(pixels);
			Image image = CreateImg(texWidth, texHeight, format, tiling, usage | vk::ImageUsageFlagBits::eTransferDst, VMA_MEMORY_USAGE_GPU_ONLY);
			CopyImg(buffer.Buffer, texWidth, texHeight, image.Image);
			return image;
		}

		void DestroyImg(Image& image) const
		{
			vmaDestroyImage(m_Allocator, image.Image, image.Allocation);
		}
	};
}