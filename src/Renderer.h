#pragma once
#define _CRT_SECURE_NO_WARNINGS // Me when MSVC-only problem that only affects vulkan versions
#include <vulkan/vulkan.hpp>	// >1.3.275 and I didn't originally notice because I was ON 1.3.275
#include <GLFW/glfw3.h>			// and only then noticed when I had to reinstall the SDK :)))))
#undef _CRT_SECURE_NO_WARNINGS	// and GLFW probably makes some sort of vulkan call, so it's included here too :))))))))))))
#include <vma/vk_mem_alloc.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp> // This came with the vulkan sdk but if you don't have it, just download it from github
#include <glm/gtc/matrix_transform.hpp>

#include "Include.h"
#include "Logger.h"

namespace hyper
{
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
		
		void CreateBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory);
		void CopyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);
		
		void CreateImage(int width, int height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags imageUsage, vk::MemoryPropertyFlags properties,
			vk::Image& image, vk::DeviceMemory& imageMemory);
		void CopyImage(vk::Buffer srcBuffer, vk::Image dstImage, int width, int height);

		uint32_t currentFrame = 0;
		double previousTime = 0.0;
		uint32_t frameCount = 0;

		Spec m_Spec;
		GLFWwindow* m_Window{};

		vk::UniqueInstance m_Instance{};

		vk::DispatchLoaderDynamic m_DLDI{};
		vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> m_DebugMessenger{};

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
		vk::UniquePipeline m_Pipeline{}; // Count your days, pipeline, VK_EXT_shader_object is replacing jobs like yours

		std::vector<vk::UniqueHandle<vk::ShaderEXT, vk::DispatchLoaderDynamic>>  m_Shaders;

		vk::UniqueCommandPool m_CommandPool{};

		vk::UniqueDescriptorPool m_DescriptorPool{};
		std::vector<vk::UniqueDescriptorSet> m_DescriptorSets{};

		vk::UniqueBuffer m_VertexBuffer;
		vk::UniqueDeviceMemory m_VertexBufferMemory;
		vk::UniqueBuffer m_IndexBuffer;
		vk::UniqueDeviceMemory m_IndexBufferMemory;

		vk::UniqueImage m_TextureImage;
		uint32_t m_TextureWidth, m_TextureHeight;
		vk::UniqueDeviceMemory m_TextureImageMemory;
		vk::UniqueImageView m_TextureImageView;
		vk::UniqueSampler m_Sampler;

		vk::UniqueImage m_DepthImage;
		vk::UniqueDeviceMemory m_DepthImageMemory;
		vk::UniqueImageView m_DepthImageView;

		std::vector<vk::UniqueBuffer> m_UniformBuffers;
		std::vector<vk::UniqueDeviceMemory> m_UniformBuffersMemory;
		std::vector<void*> m_UniformBuffersMapped;

		std::vector<vk::UniqueCommandBuffer> m_CommandBuffers{};

		vk::UniqueFence m_InFlightFence{};
		vk::UniqueSemaphore m_ImageAvailableSemaphore{};
		vk::UniqueSemaphore m_RenderFinishedSemaphore{};
	};
}