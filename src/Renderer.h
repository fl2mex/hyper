#pragma once
#include <iostream>
#include <filesystem>
#include <vulkan/vulkan.hpp> // Came with sdk
#include <GLFW/glfw3.h> // Downloaded from their website
#include <vma/vk_mem_alloc.h> // Came with sdk, if not, either re-install with this option on or download from respective site
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp> // Came with sdk, if not, either re-install with this option on or download from respective site
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include "Spec.h"
#include "Swapchain.h"
#include "Buffer.h"
#include "Image.h"
#include "Mesh.h"
#include "UserActions.h"
#include "Camera.h"

namespace hyper
{
	struct UniformBufferObject
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};
	struct PushConstantData
	{
		vk::DeviceAddress vertexBuffer;
	};

	class Renderer
	{
	public:
		void SetupRenderer(Spec _spec = {}, GLFWwindow* _window = {});
		void DrawFrame();
		~Renderer();

		void SetFramebufferResized() { m_Swapchain.Resized = true; }

	private:
		Spec m_Spec;

		GLFWwindow* m_Window{};
		vk::UniqueInstance m_Instance;

		vk::detail::DispatchLoaderDynamic m_DLDI;
		vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::detail::DispatchLoaderDynamic> m_DebugMessenger;
		vk::UniqueSurfaceKHR m_Surface;

		vk::PhysicalDevice m_PhysicalDevice;
		vk::UniqueDevice m_Device;
		
		VmaAllocator m_Allocator{};

		uint32_t m_GraphicsIndex = -1, m_PresentIndex = -1;
		vk::Queue m_DeviceQueue, m_PresentQueue;

		Swapchain m_Swapchain;

		vk::UniqueCommandPool m_CommandPool;
		std::vector<vk::UniqueCommandBuffer> m_CommandBuffers;

		vk::UniqueFence m_InFlightFence;
		vk::UniqueSemaphore m_ImageAvailableSemaphore, m_RenderFinishedSemaphore;

		vk::UniqueDescriptorPool m_DescriptorPool;

		Camera m_Camera;

		std::vector<std::shared_ptr<MeshAsset>> testMeshes;

		// Should be handled by the render object soon
		std::vector<vk::UniqueHandle<vk::ShaderEXT, vk::detail::DispatchLoaderDynamic>> m_Shaders;
		vk::UniquePipelineLayout m_PipelineLayout;
		vk::UniqueDescriptorSetLayout m_DescriptorSetLayout;
		std::vector<vk::UniqueDescriptorSet> m_DescriptorSets;
		std::vector<Buffer> m_UniformBuffers;
		
		Image m_DepthImage, m_TextureImage, m_ErrorCheckerboardImage;
		vk::UniqueSampler m_NearestSampler, m_LinearSampler;
	};
}