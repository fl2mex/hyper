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
			glfwExtensionsVector.push_back("VK_EXT_debug_utils");
			layers.push_back("VK_LAYER_KHRONOS_validation");
		}

		Logger::logger->Log("Extensions used: "); for (int i = 0; i < glfwExtensionCount; i++) Logger::logger->Log(" - " + std::string(glfwExtensions[i]));
		Logger::logger->Log("Validation layers used: "); for (auto& l : layers) Logger::logger->Log(" - " + std::string(l));

		m_Instance = vk::createInstanceUnique(vk::InstanceCreateInfo{ vk::InstanceCreateFlags(), &appInfo, static_cast<uint32_t>(layers.size()), layers.data(),
			static_cast<uint32_t>(glfwExtensionsVector.size()), glfwExtensionsVector.data() });

		if (m_Spec.Debug)
		{
			m_DLDI = vk::DispatchLoaderDynamic(*m_Instance, vkGetInstanceProcAddr);
			m_DebugMessenger = Logger::logger->MakeDebugMessenger(m_Instance, m_DLDI);
		}

		VkSurfaceKHR surfaceTmp;
		glfwCreateWindowSurface(*m_Instance, m_Window, nullptr, &surfaceTmp);
		m_Surface = vk::UniqueSurfaceKHR(surfaceTmp, *m_Instance);

		std::vector<vk::PhysicalDevice> physicalDevices = m_Instance->enumeratePhysicalDevices();
		Logger::logger->Log("Devices available: "); for (auto& d : physicalDevices) Logger::logger->Log(" - " + std::string(d.getProperties().deviceName.data()));
		m_PhysicalDevice = physicalDevices[0]; // Need to fix later, maybe a function to choose the best device via benchmarking?
		Logger::logger->Log("Chose device: " + std::string(m_PhysicalDevice.getProperties().deviceName.data()));

		std::vector<vk::QueueFamilyProperties> queueFamilyProperties = m_PhysicalDevice.getQueueFamilyProperties();

		size_t graphicsQueueFamilyIndex = std::distance(queueFamilyProperties.begin(), std::find_if(queueFamilyProperties.begin(), queueFamilyProperties.end(),
			[](vk::QueueFamilyProperties const& qfp) { return qfp.queueFlags & vk::QueueFlagBits::eGraphics; }));

		size_t presentQueueFamilyIndex = 0u;
		for (uint32_t i = 0; i < queueFamilyProperties.size(); i++)
			if (m_PhysicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), m_Surface.get()))
				presentQueueFamilyIndex = i;
		Logger::logger->Log("Graphics queue family index: " + std::to_string(graphicsQueueFamilyIndex) +
			"\nPresent queue family index: " + std::to_string(presentQueueFamilyIndex));

		std::set<uint32_t> uniqueQueueFamilyIndices = { static_cast<uint32_t>(graphicsQueueFamilyIndex), static_cast<uint32_t>(presentQueueFamilyIndex) };
		std::vector<uint32_t> FamilyIndices = { uniqueQueueFamilyIndices.begin(), uniqueQueueFamilyIndices.end() };

		std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
		float queuePriority = 0.0f;
		for (auto& queueFamilyIndex : uniqueQueueFamilyIndices)
			queueCreateInfos.push_back(vk::DeviceQueueCreateInfo{ vk::DeviceQueueCreateFlags(), static_cast<uint32_t>(queueFamilyIndex), 1, &queuePriority });

		const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

		Logger::logger->Log("Device extensions used: "); for (auto& e : deviceExtensions) Logger::logger->Log(" - " + std::string(e));

		m_Device = m_PhysicalDevice.createDeviceUnique(vk::DeviceCreateInfo(vk::DeviceCreateFlags(), static_cast<uint32_t>(queueCreateInfos.size()),
			queueCreateInfos.data(), 0u, nullptr, static_cast<uint32_t>(deviceExtensions.size()), deviceExtensions.data()));

		uint32_t imageCount = 2;
		struct SM { // Black magic, from dokipen3d on github
			vk::SharingMode sharingMode;
			uint32_t familyIndicesCount;
			uint32_t* familyIndicesDataPtr;
		} sharingModeUtil{ (graphicsQueueFamilyIndex != presentQueueFamilyIndex) ?
							   SM{ vk::SharingMode::eConcurrent, 2u, FamilyIndices.data() } :
							   SM{ vk::SharingMode::eExclusive, 0u, static_cast<uint32_t*>(nullptr) } };

		vk::Format format = vk::Format::eB8G8R8A8Unorm;
		vk::Extent2D extent{ m_Spec.Width, m_Spec.Height };

		m_Swapchain = m_Device->createSwapchainKHRUnique(vk::SwapchainCreateInfoKHR{ {}, m_Surface.get(), imageCount, format,vk::ColorSpaceKHR::eSrgbNonlinear,
			extent, 1, vk::ImageUsageFlagBits::eColorAttachment, sharingModeUtil.sharingMode, sharingModeUtil.familyIndicesCount, sharingModeUtil.familyIndicesDataPtr,
			vk::SurfaceTransformFlagBitsKHR::eIdentity, vk::CompositeAlphaFlagBitsKHR::eOpaque, vk::PresentModeKHR::eFifo, true, nullptr });
		m_SwapChainImages = m_Device->getSwapchainImagesKHR(m_Swapchain.get());

		m_ImageViews.reserve(m_SwapChainImages.size());
		for (vk::Image image : m_SwapChainImages)
		{
			vk::ImageViewCreateInfo imageViewCreateInfo(vk::ImageViewCreateFlags(), image, vk::ImageViewType::e2D, format,
				vk::ComponentMapping{ vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA },
				vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });
			m_ImageViews.push_back(m_Device->createImageViewUnique(imageViewCreateInfo));
		}

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
		vk::Rect2D scissor{ { 0, 0 }, extent };
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

		vk::AttachmentDescription colorAttachment{ {}, format, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
			{}, {}, {}, vk::ImageLayout::ePresentSrcKHR };
		vk::AttachmentReference colourAttachmentRef{ 0, vk::ImageLayout::eColorAttachmentOptimal };
		vk::SubpassDescription subpass{ {}, vk::PipelineBindPoint::eGraphics, /*inAttachmentCount*/ 0, nullptr, 1, &colourAttachmentRef };

		m_InFlightFence = m_Device->createFenceUnique({ vk::FenceCreateFlagBits::eSignaled });

		m_ImageAvailableSemaphore = m_Device->createSemaphoreUnique({});
		m_RenderFinishedSemaphore = m_Device->createSemaphoreUnique({});

		vk::SubpassDependency subpassDependency{ VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::PipelineStageFlagBits::eColorAttachmentOutput, {}, vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite };

		m_RenderPass = m_Device->createRenderPassUnique(vk::RenderPassCreateInfo{ {}, 1, &colorAttachment, 1, &subpass, 1, &subpassDependency });

		m_Pipeline = m_Device->createGraphicsPipelineUnique({}, { {}, 2, pipelineShaderStages.data(),&vertexInputInfo, &inputAssembly, nullptr, &viewportState,
			&rasterizer, &multisampling, nullptr, &colorBlending, nullptr, *m_PipelineLayout, *m_RenderPass, 0 }).value;

		m_Framebuffers = std::vector<vk::UniqueFramebuffer>(imageCount);
		for (size_t i = 0; i < m_ImageViews.size(); i++)
			m_Framebuffers[i] = m_Device->createFramebufferUnique(vk::FramebufferCreateInfo{ {}, *m_RenderPass, 1, &(*m_ImageViews[i]), extent.width, extent.height, 1 });

		m_CommandPoolUnique = m_Device->createCommandPoolUnique({ {}, static_cast<uint32_t>(graphicsQueueFamilyIndex) });

		m_CommandBuffers = m_Device->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(m_CommandPoolUnique.get(), vk::CommandBufferLevel::ePrimary,
			static_cast<uint32_t>(m_Framebuffers.size())));

		m_DeviceQueue = m_Device->getQueue(static_cast<uint32_t>(graphicsQueueFamilyIndex), 0);
		m_PresentQueue = m_Device->getQueue(static_cast<uint32_t>(presentQueueFamilyIndex), 0);

		for (size_t i = 0; i < m_CommandBuffers.size(); i++)
		{
			m_CommandBuffers[i]->begin(vk::CommandBufferBeginInfo{});
			vk::ClearValue clearValues{};
			vk::RenderPassBeginInfo renderPassBeginInfo{ m_RenderPass.get(), m_Framebuffers[i].get(), vk::Rect2D{ { 0, 0 }, extent }, 1, &clearValues };

			m_CommandBuffers[i]->beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
			m_CommandBuffers[i]->bindPipeline(vk::PipelineBindPoint::eGraphics, *m_Pipeline);
			m_CommandBuffers[i]->draw(3, 1, 0, 0);
			m_CommandBuffers[i]->endRenderPass();
			m_CommandBuffers[i]->end();
		}
	}

	void hyper::Renderer::DrawFrame()
	{
		m_Device->waitForFences(1, &m_InFlightFence.get(), VK_TRUE, UINT64_MAX);
		m_Device->resetFences(1, &m_InFlightFence.get());

		vk::ResultValue<uint32_t> imageIndex = m_Device->acquireNextImageKHR(m_Swapchain.get(), std::numeric_limits<uint64_t>::max(),
			m_ImageAvailableSemaphore.get(), {});

		vk::PipelineStageFlags waitStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		m_DeviceQueue.submit(vk::SubmitInfo{ 1, &m_ImageAvailableSemaphore.get(), &waitStageMask, 1, &m_CommandBuffers[imageIndex.value].get(), 1,
			&m_RenderFinishedSemaphore.get() }, m_InFlightFence.get());

		m_PresentQueue.presentKHR({ 1, &m_RenderFinishedSemaphore.get(), 1, &m_Swapchain.get(), &imageIndex.value });
	}

	hyper::Renderer::~Renderer()
	{
		m_Device->waitIdle();
	}
}