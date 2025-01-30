#pragma once
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

namespace hyper
{
	struct Swapchain
	{
		vk::UniqueSwapchainKHR OldSwapchain;
		vk::UniqueSwapchainKHR ActualSwapchain;
		std::vector<vk::Image> Images;
		std::vector<vk::UniqueImageView> ImageViews;
		uint32_t ImageCount = 0;
		vk::Format ImageFormat = { vk::Format::eUndefined };
		vk::Extent2D Extent;
		bool Resized = false;

		void CreateSwapchain(uint32_t count, vk::Format format, vk::Extent2D extent, GLFWwindow* window, vk::Device device,
			uint32_t graphicsIndex, uint32_t presentIndex, vk::SurfaceKHR surface);
	};
}