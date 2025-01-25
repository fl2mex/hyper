#pragma once
#include <iostream>
#include <filesystem>
#include <vulkan/vulkan.hpp>	// >1.3.275 and I didn't originally notice because I was ON 1.3.275
#include <GLFW/glfw3.h>			// and only then noticed when I had to reinstall the SDK :)))))
#include <vma/vk_mem_alloc.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp> // This came with the vulkan sdk but if you don't have it, just download it from github

#include "Spec.h"
#include "Buffer.h"
#include "Image.h"
#include "Mesh.h"

namespace hyper
{
	struct UniformBufferObject
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
		glm::vec4 ambientColor;
		glm::vec4 sunlightDirection;
		glm::vec4 sunlightColor;
	};
	struct PushConstantData
	{
		glm::mat4 worldMatrix;
		vk::DeviceAddress vertexBuffer;
	};

	class Renderer
	{
	public:
		void SetupRenderer(Spec _spec = {}, GLFWwindow* _window = {});
		void DrawFrame();
		~Renderer();

		bool m_FramebufferResized = false;

	private:
		std::vector<std::shared_ptr<MeshAsset>> testMeshes;

		void RecreateSwapchain();
		void RecreateCommandBuffers();
		
		uint32_t currentFrame = 0;
		double previousTime = 0.0;
		uint32_t frameCount = 0;

		Spec m_Spec;
		GLFWwindow* m_Window;

		vk::UniqueInstance m_Instance;

		vk::detail::DispatchLoaderDynamic m_DLDI;
		vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::detail::DispatchLoaderDynamic> m_DebugMessenger;

		vk::UniqueSurfaceKHR m_Surface;

		vk::PhysicalDevice m_PhysicalDevice;
		vk::UniqueDevice m_Device;
		
		VmaAllocator m_Allocator;

		vk::Queue m_DeviceQueue;
		vk::Queue m_PresentQueue;

		vk::Format m_SwapchainImageFormat;
		vk::Extent2D m_SwapchainExtent;
		uint32_t m_SwapchainImageCount;
		vk::UniqueSwapchainKHR m_Swapchain;
		std::vector<vk::Image> m_SwapchainImages; // Can this be turned unique as well?
		std::vector<vk::UniqueImageView> m_SwapchainImageViews;

		vk::UniqueCommandPool m_CommandPool;
		std::vector<vk::UniqueCommandBuffer> m_CommandBuffers;

		vk::UniqueFence m_InFlightFence;
		vk::UniqueSemaphore m_ImageAvailableSemaphore;
		vk::UniqueSemaphore m_RenderFinishedSemaphore;

		std::vector<Buffer> m_UniformBuffers;

		// Should be handled by the render object
		std::vector<vk::UniqueHandle<vk::ShaderEXT, vk::detail::DispatchLoaderDynamic>> m_Shaders;
		vk::UniquePipelineLayout m_PipelineLayout;
		vk::UniqueDescriptorSetLayout m_DescriptorSetLayout;
		vk::UniqueDescriptorPool m_DescriptorPool;
		std::vector<vk::UniqueDescriptorSet> m_DescriptorSets;
		
		Image m_DepthImage;
		Image m_TextureImage;
		Image m_ErrorCheckerboardImage;
		vk::UniqueSampler m_LinearSampler;
		vk::UniqueSampler m_NearestSampler;
	};
}