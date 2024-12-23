#pragma once
#include "Include.h"
#include "Logger.h"

namespace hyper
{
	struct SM
	{ // Black magic, from dokipen3d on github
		vk::SharingMode sharingMode;
		uint32_t familyIndicesCount;
		uint32_t* familyIndicesDataPtr;
	};

	class Renderer
	{
	public:
		void SetupRenderer(Spec _spec = {}, GLFWwindow* _window = {});
		void DrawFrame();
		~Renderer();

		bool m_FramebufferResized = false;
		int bruhtesting = 0;
	private:
		void RecreateSwapchain();
		void RecreateCommandBuffers();

		Spec m_Spec;
		GLFWwindow* m_Window;

		vk::UniqueInstance m_Instance{};

		vk::DispatchLoaderDynamic m_DLDI{};
		vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> m_DebugMessenger{};

		vk::UniqueSurfaceKHR m_Surface{};

		vk::PhysicalDevice m_PhysicalDevice{};
		vk::UniqueDevice m_Device{};
		vk::Queue m_DeviceQueue{};
		vk::Queue m_PresentQueue{};
		
		vk::Format m_SwapchainImageFormat{};
		vk::Extent2D m_SwapchainExtent{};
		uint32_t m_SwapchainImageCount{};
		SM m_SharingModeUtil;
		
		vk::UniqueSwapchainKHR m_Swapchain{};

		std::vector<vk::Image> m_SwapchainImages{}; // Can this be turned unique as well?
		std::vector<vk::UniqueImageView> m_ImageViews{};
		std::vector<vk::UniqueFramebuffer> m_Framebuffers{};

		vk::UniquePipelineLayout m_PipelineLayout{}; // Can this go out of scope?
		vk::UniqueRenderPass m_RenderPass{};
		vk::UniquePipeline m_Pipeline{};

		vk::UniqueCommandPool m_CommandPoolUnique{};
		std::vector<vk::UniqueCommandBuffer> m_CommandBuffers{};

		vk::UniqueFence m_InFlightFence{};
		vk::UniqueSemaphore m_ImageAvailableSemaphore{};
		vk::UniqueSemaphore m_RenderFinishedSemaphore{};
	};
}