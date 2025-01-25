#include "Swapchain.h"

namespace hyper
{
	void Swapchain::CreateSwapchain(uint32_t count, vk::Format format, vk::Extent2D extent, GLFWwindow* window, vk::Device device, uint32_t graphicsIndex, uint32_t presentIndex, vk::SurfaceKHR surface)
	{
		Resized = false;
		ImageCount = count;
		ImageFormat = format;
		Extent = extent;

		int width = 0, height = 0;
		glfwGetFramebufferSize(window, &width, &height);
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents();
		}
		Extent = vk::Extent2D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

		OldSwapchain = std::move(ActualSwapchain);
		Images.clear();
		ImageViews.clear();

		std::vector<uint32_t> familyIndices{ static_cast<uint32_t>(graphicsIndex) };
		if (graphicsIndex != presentIndex)
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