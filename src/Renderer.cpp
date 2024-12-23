#include "Renderer.h"

namespace hyper
{
	void hyper::Renderer::SetupRenderer(Spec _spec, GLFWwindow* _window)
	{
		m_Spec = _spec; // No constructor :(
		m_Window = _window;

		vk::ApplicationInfo appInfo(m_Spec.Title.c_str(), m_Spec.ApiVersion, "hyper", m_Spec.ApiVersion, m_Spec.ApiVersion);

		uint32_t glfwExtensionCount = 0u;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		std::vector<const char*> glfwExtensionsVector(glfwExtensions, glfwExtensions + glfwExtensionCount), layers;

		if (m_Spec.Debug)
		{
			Logger::logger->Log("Debug Enabled");
			glfwExtensionsVector.push_back("VK_EXT_debug_utils");
			layers.push_back("VK_LAYER_KHRONOS_validation");
		}
		Logger::logger->Log("Extensions used: "); for (uint32_t i = 0; i < glfwExtensionCount; i++) Logger::logger->Log(" - " + std::string(glfwExtensions[i]));
		Logger::logger->Log("Validation layers used: "); for (auto& l : layers) Logger::logger->Log(" - " + std::string(l));

		// Instance
		m_Instance = vk::createInstanceUnique(vk::InstanceCreateInfo{ vk::InstanceCreateFlags(), &appInfo, static_cast<uint32_t>(layers.size()), layers.data(),
			static_cast<uint32_t>(glfwExtensionsVector.size()), glfwExtensionsVector.data() });

		if (m_Spec.Debug)
		{
			m_DLDI = vk::DispatchLoaderDynamic(*m_Instance, vkGetInstanceProcAddr);
			m_DebugMessenger = Logger::logger->MakeDebugMessenger(m_Instance, m_DLDI);
		}

		// Surface
		VkSurfaceKHR surfaceTmp;
		glfwCreateWindowSurface(*m_Instance, m_Window, nullptr, &surfaceTmp);
		m_Surface = vk::UniqueSurfaceKHR(surfaceTmp, *m_Instance);

		// Physical Device
		
		std::vector<vk::PhysicalDevice> physicalDevices = m_Instance->enumeratePhysicalDevices();
		Logger::logger->Log("Devices available: "); for (auto& d : physicalDevices) Logger::logger->Log(" - " + std::string(d.getProperties().deviceName.data()));
		
		m_PhysicalDevice = physicalDevices[0]; // Set physical device to the first one, then check for a discrete gpu and set it to that
		for (auto& d : physicalDevices)
		{
			if (d.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
				m_PhysicalDevice = d;
		}
		Logger::logger->Log("Chose device: " + std::string(m_PhysicalDevice.getProperties().deviceName.data()));

		std::vector<vk::QueueFamilyProperties> queueFamilyProperties = m_PhysicalDevice.getQueueFamilyProperties();

		// Queue Families
		size_t graphicsQueueFamilyIndex = std::distance(queueFamilyProperties.begin(), std::find_if(queueFamilyProperties.begin(), queueFamilyProperties.end(),
			[](vk::QueueFamilyProperties const& qfp) { return qfp.queueFlags & vk::QueueFlagBits::eGraphics; }));
		size_t presentQueueFamilyIndex = 0u;
		for (uint32_t i = 0; i < queueFamilyProperties.size(); i++)
			if (m_PhysicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), m_Surface.get()))
				presentQueueFamilyIndex = i;

		std::set<uint32_t> uniqueQueueFamilyIndices = { static_cast<uint32_t>(graphicsQueueFamilyIndex), static_cast<uint32_t>(presentQueueFamilyIndex) };
		std::vector<uint32_t> FamilyIndices = { uniqueQueueFamilyIndices.begin(), uniqueQueueFamilyIndices.end() };

		std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
		float queuePriority = 0.0f;
		for (auto& queueFamilyIndex : uniqueQueueFamilyIndices)
			queueCreateInfos.push_back(vk::DeviceQueueCreateInfo{ vk::DeviceQueueCreateFlags(), static_cast<uint32_t>(queueFamilyIndex), 1, &queuePriority });

		const std::vector<const char*> deviceExtensions = { vk::KHRSwapchainExtensionName, vk::KHRDynamicRenderingExtensionName };
		Logger::logger->Log("Device extensions used: "); for (auto& e : deviceExtensions) Logger::logger->Log(" - " + std::string(e));

		vk::PhysicalDeviceFeatures deviceFeatures = vk::PhysicalDeviceFeatures();
		vk::PhysicalDeviceDynamicRenderingFeaturesKHR dynamicFeatures = vk::PhysicalDeviceDynamicRenderingFeaturesKHR(1);
		
		// Logical Device
		m_Device = m_PhysicalDevice.createDeviceUnique(vk::DeviceCreateInfo(vk::DeviceCreateFlags(), static_cast<uint32_t>(queueCreateInfos.size()),
			queueCreateInfos.data(), 0u, nullptr, static_cast<uint32_t>(deviceExtensions.size()), deviceExtensions.data(), &deviceFeatures, &dynamicFeatures));

		m_SwapchainImageCount = 2;
		
		m_SharingModeUtil = SM{ (graphicsQueueFamilyIndex != presentQueueFamilyIndex) ?
							   SM{ vk::SharingMode::eConcurrent, 2u, FamilyIndices.data() } :
							   SM{ vk::SharingMode::eExclusive, 0u, static_cast<uint32_t*>(nullptr) } };

		m_SwapchainImageFormat = vk::Format::eB8G8R8A8Unorm;
		m_SwapchainExtent = vk::Extent2D{ m_Spec.Width, m_Spec.Height };

		// Swapchain
		m_Swapchain = m_Device->createSwapchainKHRUnique(vk::SwapchainCreateInfoKHR{ {}, m_Surface.get(), m_SwapchainImageCount, m_SwapchainImageFormat,
			vk::ColorSpaceKHR::eSrgbNonlinear, m_SwapchainExtent, 1, vk::ImageUsageFlagBits::eColorAttachment, m_SharingModeUtil.sharingMode,
			m_SharingModeUtil.familyIndicesCount, m_SharingModeUtil.familyIndicesDataPtr, vk::SurfaceTransformFlagBitsKHR::eIdentity,
			vk::CompositeAlphaFlagBitsKHR::eOpaque, vk::PresentModeKHR::eFifo, true, nullptr });
		m_SwapchainImages = m_Device->getSwapchainImagesKHR(m_Swapchain.get());
		Logger::logger->Log("Swapchain created.    Graphics queue family index: " + std::to_string(graphicsQueueFamilyIndex) +
			"    Present queue family index: " + std::to_string(presentQueueFamilyIndex));
		// Image Views
		m_ImageViews.reserve(m_SwapchainImages.size());
		for (vk::Image image : m_SwapchainImages)
		{
			vk::ImageViewCreateInfo imageViewCreateInfo(vk::ImageViewCreateFlags(), image, vk::ImageViewType::e2D, m_SwapchainImageFormat,
				vk::ComponentMapping{ vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA },
				vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });
			m_ImageViews.push_back(m_Device->createImageViewUnique(imageViewCreateInfo));
		}

		// Shaders
		std::vector<char> vertShaderCode = readFile(m_Spec.VertexShader);
		vk::ShaderModuleCreateInfo vertShaderCreateInfo = vk::ShaderModuleCreateInfo{ {}, vertShaderCode.size(),
			reinterpret_cast<const uint32_t*>(vertShaderCode.data()) };
		vk::UniqueShaderModule vertexShaderModule = m_Device->createShaderModuleUnique(vertShaderCreateInfo);

		std::vector<char> fragShaderCode = readFile(m_Spec.FragmentShader);
		vk::ShaderModuleCreateInfo fragShaderCreateInfo = vk::ShaderModuleCreateInfo{ {}, fragShaderCode.size(),
			reinterpret_cast<const uint32_t*>(fragShaderCode.data()) };
		vk::UniqueShaderModule fragmentShaderModule = m_Device->createShaderModuleUnique(fragShaderCreateInfo);

		std::vector<vk::PipelineShaderStageCreateInfo> pipelineShaderStages{ { {}, vk::ShaderStageFlagBits::eVertex, *vertexShaderModule, "main" },
			{ {}, vk::ShaderStageFlagBits::eFragment, *fragmentShaderModule, "main" } };

		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{ {}, 0u, nullptr, 0u, nullptr };
		vk::PipelineInputAssemblyStateCreateInfo inputAssembly{ {}, vk::PrimitiveTopology::eTriangleList, false };

		vk::Viewport viewport{ 0.0f, 0.0f, static_cast<float>(m_Spec.Width), static_cast<float>(m_Spec.Height), 0.0f, 1.0f };
		vk::Rect2D scissor{ { 0, 0 }, m_SwapchainExtent };
		vk::PipelineViewportStateCreateInfo viewportState{ {}, 1, &viewport, 1, &scissor };

		// Could turn this into another type of spec file, like a render spec or something
		// I wish it was the same with extensions, but you need to chain them together via create infos, hope i'm wrong though
		vk::PipelineRasterizationStateCreateInfo rasterizer{ {}, /*depthClamp*/ false, /*rasterizeDiscard*/ false, vk::PolygonMode::eFill, {},
			/*frontFace*/ vk::FrontFace::eCounterClockwise, {}, {}, {}, {}, 1.0f };
		vk::PipelineMultisampleStateCreateInfo multisampling{ {}, vk::SampleCountFlagBits::e1, false, 1.0 };
		vk::PipelineColorBlendAttachmentState colorBlendAttachment{ {}, /*srcCol*/ vk::BlendFactor::eOne, /*dstCol*/ vk::BlendFactor::eZero,
			/*colBlend*/ vk::BlendOp::eAdd, /*srcAlpha*/ vk::BlendFactor::eOne, /*dstAlpha*/ vk::BlendFactor::eZero, /*alphaBlend*/ vk::BlendOp::eAdd,
			vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA };
		vk::PipelineColorBlendStateCreateInfo colorBlending{ {}, /*logicOpEnable=*/false,
			vk::LogicOp::eCopy, /*attachmentCount=*/1, /*colourAttachments=*/&colorBlendAttachment };

		m_PipelineLayout = m_Device->createPipelineLayoutUnique({}, nullptr);

		vk::AttachmentDescription colorAttachment{ {}, m_SwapchainImageFormat, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eStore, {}, {}, {}, vk::ImageLayout::ePresentSrcKHR };
		vk::AttachmentReference colourAttachmentRef{ 0, vk::ImageLayout::eColorAttachmentOptimal };
		vk::SubpassDescription subpass{ {}, vk::PipelineBindPoint::eGraphics, /*inAttachmentCount*/ 0, nullptr, 1, &colourAttachmentRef };

		m_InFlightFence = m_Device->createFenceUnique({ vk::FenceCreateFlagBits::eSignaled });

		m_ImageAvailableSemaphore = m_Device->createSemaphoreUnique({});
		m_RenderFinishedSemaphore = m_Device->createSemaphoreUnique({});

		vk::SubpassDependency subpassDependency{ VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::PipelineStageFlagBits::eColorAttachmentOutput, {}, vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite };

		// Pipeline
		vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{ 0, 1, &m_SwapchainImageFormat };

		vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo{ {}, 2, pipelineShaderStages.data(),&vertexInputInfo, &inputAssembly, nullptr, &viewportState,
			&rasterizer, &multisampling, nullptr, &colorBlending, nullptr, *m_PipelineLayout, nullptr, 0, nullptr, 0, &pipelineRenderingCreateInfo };

		m_Pipeline = m_Device->createGraphicsPipelineUnique({}, graphicsPipelineCreateInfo).value;

		// Command Pool
		m_CommandPoolUnique = m_Device->createCommandPoolUnique({ { vk::CommandPoolCreateFlags() | vk::CommandPoolCreateFlagBits::eResetCommandBuffer },
			static_cast<uint32_t>(graphicsQueueFamilyIndex) });

		m_CommandBuffers = m_Device->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(m_CommandPoolUnique.get(), vk::CommandBufferLevel::ePrimary,
			static_cast<uint32_t>(m_SwapchainImageCount)));

		m_DeviceQueue = m_Device->getQueue(static_cast<uint32_t>(graphicsQueueFamilyIndex), 0);
		m_PresentQueue = m_Device->getQueue(static_cast<uint32_t>(presentQueueFamilyIndex), 0);

		RecreateCommandBuffers();
	}

	void hyper::Renderer::DrawFrame()
	{
		m_Device->waitForFences(1, &m_InFlightFence.get(), VK_TRUE, UINT64_MAX);
		m_Device->resetFences(1, &m_InFlightFence.get());

		if (m_FramebufferResized)
		{
			m_Device->waitIdle();
			RecreateSwapchain();
			RecreateCommandBuffers();
		}

		vk::ResultValue<uint32_t> imageIndex = m_Device->acquireNextImageKHR(m_Swapchain.get(), std::numeric_limits<uint64_t>::max(),
			m_ImageAvailableSemaphore.get(), {});

		if (imageIndex.result == vk::Result::eErrorOutOfDateKHR || imageIndex.result == vk::Result::eSuboptimalKHR)
		{
			m_FramebufferResized = true;
			return;
		}

		vk::PipelineStageFlags waitStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		m_DeviceQueue.submit(vk::SubmitInfo{ 1, &m_ImageAvailableSemaphore.get(), &waitStageMask, 1, &m_CommandBuffers[imageIndex.value].get(), 1,
			&m_RenderFinishedSemaphore.get() }, m_InFlightFence.get());

		m_PresentQueue.presentKHR({ 1, &m_RenderFinishedSemaphore.get(), 1, &m_Swapchain.get(), &imageIndex.value });
	}

	hyper::Renderer::~Renderer()
	{
		m_Device->waitIdle();
	}

	void Renderer::RecreateSwapchain()
	{
		m_FramebufferResized = false;
		int width = 0, height = 0;
		glfwGetFramebufferSize(m_Window, &width, &height);
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(m_Window, &width, &height);
			glfwWaitEvents();
		}

		m_SwapchainExtent = vk::Extent2D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

		m_SwapchainImages.clear();
		m_ImageViews.clear();

		m_Device->destroySwapchainKHR(m_Swapchain.get());
		m_Swapchain.get() = VK_NULL_HANDLE;

		// Reset SM
		std::vector<vk::QueueFamilyProperties> queueFamilyProperties = m_PhysicalDevice.getQueueFamilyProperties();

		size_t graphicsQueueFamilyIndex = std::distance(queueFamilyProperties.begin(), std::find_if(queueFamilyProperties.begin(), queueFamilyProperties.end(),
			[](vk::QueueFamilyProperties const& qfp) { return qfp.queueFlags & vk::QueueFlagBits::eGraphics; }));

		size_t presentQueueFamilyIndex = 0u;
		for (uint32_t i = 0; i < queueFamilyProperties.size(); i++)
			if (m_PhysicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), m_Surface.get()))
				presentQueueFamilyIndex = i;
		
		std::set<uint32_t> uniqueQueueFamilyIndices = { static_cast<uint32_t>(graphicsQueueFamilyIndex), static_cast<uint32_t>(presentQueueFamilyIndex) };
		std::vector<uint32_t> FamilyIndices = { uniqueQueueFamilyIndices.begin(), uniqueQueueFamilyIndices.end() };

		m_SharingModeUtil = SM{ (graphicsQueueFamilyIndex != presentQueueFamilyIndex) ?
						   SM{ vk::SharingMode::eConcurrent, 2u, FamilyIndices.data() } :
						   SM{ vk::SharingMode::eExclusive, 0u, static_cast<uint32_t*>(nullptr) } };

		m_Swapchain = m_Device->createSwapchainKHRUnique(vk::SwapchainCreateInfoKHR{ {}, m_Surface.get(), m_SwapchainImageCount, m_SwapchainImageFormat,
			vk::ColorSpaceKHR::eSrgbNonlinear, m_SwapchainExtent, 1, vk::ImageUsageFlagBits::eColorAttachment, m_SharingModeUtil.sharingMode,
			m_SharingModeUtil.familyIndicesCount, m_SharingModeUtil.familyIndicesDataPtr, vk::SurfaceTransformFlagBitsKHR::eIdentity,
			vk::CompositeAlphaFlagBitsKHR::eOpaque, vk::PresentModeKHR::eFifo, true, nullptr });

		Logger::logger->Log("Swapchain recreated.  Graphics queue family index: " + std::to_string(graphicsQueueFamilyIndex) +
			"    Present queue family index: " + std::to_string(presentQueueFamilyIndex));

		m_SwapchainImages = m_Device->getSwapchainImagesKHR(m_Swapchain.get());

		m_ImageViews.reserve(m_SwapchainImages.size());
		for (vk::Image image : m_SwapchainImages)
		{
			vk::ImageViewCreateInfo imageViewCreateInfo(vk::ImageViewCreateFlags(), image, vk::ImageViewType::e2D, m_SwapchainImageFormat,
				vk::ComponentMapping{ vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA },
				vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });
			m_ImageViews.push_back(m_Device->createImageViewUnique(imageViewCreateInfo));
		}
	}

	void Renderer::RecreateCommandBuffers()
	{
		for (size_t i = 0; i < m_CommandBuffers.size(); i++)
		{
			m_CommandBuffers[i]->begin(vk::CommandBufferBeginInfo{});

			vk::ImageMemoryBarrier imageMemoryBarrier{ vk::AccessFlagBits::eMemoryRead, vk::AccessFlagBits::eColorAttachmentWrite,
				vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
				m_SwapchainImages[i], vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } };

			m_CommandBuffers[i]->pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput,
				{}, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

			vk::ClearValue clearValues{};
			const vk::RenderingAttachmentInfo colorAttachmentInfo{ m_ImageViews[i].get(), vk::ImageLayout::eAttachmentOptimalKHR,
				{}, {},{}, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, clearValues };
			const vk::RenderingInfo renderingInfo{ {}, vk::Rect2D{ { 0, 0 }, m_SwapchainExtent }, 1, {}, 1, &colorAttachmentInfo };

			m_CommandBuffers[i]->beginRendering(&renderingInfo);
			m_CommandBuffers[i]->bindPipeline(vk::PipelineBindPoint::eGraphics, *m_Pipeline);
			m_CommandBuffers[i]->draw(3, 1, 0, 0);
			m_CommandBuffers[i]->endRendering();

			imageMemoryBarrier = vk::ImageMemoryBarrier{ vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eMemoryRead,
				vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
				m_SwapchainImages[i], vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } };

			m_CommandBuffers[i]->pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe,
				{}, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

			m_CommandBuffers[i]->end();
		}
	}
}