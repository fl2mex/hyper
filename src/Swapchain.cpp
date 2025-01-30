#include "Swapchain.h"

namespace hyper
{
	void Swapchain::CreateSwapchain(uint32_t count, vk::Format format, vk::Extent2D extent, GLFWwindow* window, vk::Device device,
		uint32_t graphicsIndex, uint32_t presentIndex, vk::SurfaceKHR surface)
	{
		Resized = false; // Not a class, so can't do the member initializer list underneath the function definition
		ImageCount = count;
		ImageFormat = format;
		Extent = extent;

		int width = 0, height = 0;
		glfwGetFramebufferSize(window, &width, &height); // Putting this twice makes the window not lag
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents(); // When multithreading, this won't lag as hard, hopefully
		}
		Extent = vk::Extent2D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

		OldSwapchain = std::move(ActualSwapchain); // Should help with smooth resizing, but need to implement multithreading first
		Images.clear();
		ImageViews.clear();

		std::vector<uint32_t> familyIndices{ static_cast<uint32_t>(graphicsIndex) };	// The next three blocks of code is repeated in the renderer class,
		if (graphicsIndex != presentIndex)												// I could maybe pass in the vector?
			familyIndices.push_back(static_cast<uint32_t>(presentIndex));

		vk::SharingMode sharingMode = vk::SharingMode::eExclusive;
		uint32_t familyIndicesCount = 0;
		uint32_t* familyIndicesDataPtr = nullptr;

		if (graphicsIndex != presentIndex)
		{
			sharingMode = vk::SharingMode::eConcurrent;
			familyIndicesCount = 2;
			familyIndicesDataPtr = familyIndices.data();
		}
		ActualSwapchain = device.createSwapchainKHRUnique(vk::SwapchainCreateInfoKHR{ {}, surface, ImageCount, ImageFormat,
			vk::ColorSpaceKHR::eSrgbNonlinear, Extent, 1, vk::ImageUsageFlagBits::eColorAttachment, sharingMode, familyIndicesCount, familyIndicesDataPtr,
			vk::SurfaceTransformFlagBitsKHR::eIdentity, vk::CompositeAlphaFlagBitsKHR::eOpaque, vk::PresentModeKHR::eImmediate, true, OldSwapchain.get() });

		Images = device.getSwapchainImagesKHR(ActualSwapchain.get());
		ImageViews.reserve(Images.size());
		for (vk::Image image : Images)
			ImageViews.push_back(device.createImageViewUnique({ vk::ImageViewCreateFlags(), image, vk::ImageViewType::e2D, ImageFormat,
				vk::ComponentMapping{ vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA },
				vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } }));
	}
}