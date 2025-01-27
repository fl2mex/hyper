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
		UserActions m_UserActions;

	private:
		std::vector<std::shared_ptr<MeshAsset>> testMeshes;
		
		Spec m_Spec;
		GLFWwindow* m_Window;
		Camera m_Camera;

		vk::UniqueInstance m_Instance;

		vk::detail::DispatchLoaderDynamic m_DLDI;
		vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::detail::DispatchLoaderDynamic> m_DebugMessenger;

		vk::UniqueSurfaceKHR m_Surface;

		vk::PhysicalDevice m_PhysicalDevice;
		vk::UniqueDevice m_Device;
		
		VmaAllocator m_Allocator;

		uint32_t m_GraphicsIndex, m_PresentIndex;
		vk::Queue m_DeviceQueue, m_PresentQueue;

		Swapchain m_Swapchain;

		vk::UniqueCommandPool m_CommandPool;
		std::vector<vk::UniqueCommandBuffer> m_CommandBuffers;

		vk::UniqueFence m_InFlightFence;
		vk::UniqueSemaphore m_ImageAvailableSemaphore, m_RenderFinishedSemaphore;

		vk::UniqueDescriptorPool m_DescriptorPool;

		// Should be handled by the render object
		std::vector<vk::UniqueHandle<vk::ShaderEXT, vk::detail::DispatchLoaderDynamic>> m_Shaders;
		vk::UniquePipelineLayout m_PipelineLayout;
		vk::UniqueDescriptorSetLayout m_DescriptorSetLayout;
		std::vector<vk::UniqueDescriptorSet> m_DescriptorSets;
		std::vector<Buffer> m_UniformBuffers;
		
		Image m_DepthImage, m_TextureImage, m_ErrorCheckerboardImage;
		vk::UniqueSampler m_LinearSampler, m_NearestSampler;
	};
}