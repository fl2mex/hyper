#include "Renderer.h"

namespace hyper
{
	void hyper::Renderer::SetupRenderer(Spec _spec, GLFWwindow* _window)
	{
		m_Spec = _spec;
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

		// DLDI and debug messenger
		if (m_Spec.Debug)
		{
			m_DLDI = vk::DispatchLoaderDynamic(*m_Instance, vkGetInstanceProcAddr);
			m_DebugMessenger = Logger::logger->MakeDebugMessenger(m_Instance, m_DLDI);
		}

		// Surface
		VkSurfaceKHR surfaceTmp;
		glfwCreateWindowSurface(*m_Instance, m_Window, nullptr, &surfaceTmp);
		m_Surface = vk::UniqueSurfaceKHR(surfaceTmp, *m_Instance);

		// Physical device
		std::vector<vk::PhysicalDevice> physicalDevices = m_Instance->enumeratePhysicalDevices();
		Logger::logger->Log("Devices available: "); for (auto& d : physicalDevices) Logger::logger->Log(" - " + std::string(d.getProperties().deviceName.data()));
		
		m_PhysicalDevice = physicalDevices[0]; // Set physical device to the first one, then check for a discrete gpu and set it to that
		for (auto& d : physicalDevices)
			if (d.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
				m_PhysicalDevice = d;
		Logger::logger->Log("Chose device: " + std::string(m_PhysicalDevice.getProperties().deviceName.data()));

		// Queue families
		std::vector<vk::QueueFamilyProperties> queueFamilyProperties = m_PhysicalDevice.getQueueFamilyProperties();
		size_t graphicsQueueFamilyIndex = std::distance(queueFamilyProperties.begin(), std::find_if(queueFamilyProperties.begin(), queueFamilyProperties.end(),
			[](vk::QueueFamilyProperties const& qfp) { return qfp.queueFlags & vk::QueueFlagBits::eGraphics; }));
		size_t presentQueueFamilyIndex = 0u;
		for (uint32_t i = 0; i < queueFamilyProperties.size(); i++)
			if (m_PhysicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), m_Surface.get()))
				presentQueueFamilyIndex = i;

		std::vector<uint32_t> FamilyIndices{ static_cast<uint32_t>(graphicsQueueFamilyIndex) };
		if (graphicsQueueFamilyIndex != presentQueueFamilyIndex)
			FamilyIndices.push_back(static_cast<uint32_t>(presentQueueFamilyIndex));

		std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
		float queuePriority = 0.0f;
		for (auto& queueFamilyIndex : FamilyIndices)
			queueCreateInfos.push_back(vk::DeviceQueueCreateInfo{ vk::DeviceQueueCreateFlags(), static_cast<uint32_t>(queueFamilyIndex), 1, &queuePriority });

		// Logical device
		const std::vector<const char*> deviceExtensions = { vk::KHRSwapchainExtensionName, vk::KHRDynamicRenderingExtensionName };
		Logger::logger->Log("Device extensions used: "); for (auto& e : deviceExtensions) Logger::logger->Log(" - " + std::string(e));

		vk::PhysicalDeviceDynamicRenderingFeatures dynamicFeatures = vk::PhysicalDeviceDynamicRenderingFeatures(1);
		m_Device = m_PhysicalDevice.createDeviceUnique(vk::DeviceCreateInfo(vk::DeviceCreateFlags(), static_cast<uint32_t>(queueCreateInfos.size()),
			queueCreateInfos.data(), 0u, nullptr, static_cast<uint32_t>(deviceExtensions.size()), deviceExtensions.data(), {}, &dynamicFeatures));

		// Queues
		m_DeviceQueue = m_Device->getQueue(static_cast<uint32_t>(graphicsQueueFamilyIndex), 0);
		m_PresentQueue = m_Device->getQueue(static_cast<uint32_t>(presentQueueFamilyIndex), 0);

		// Swapchain
		m_SwapchainImageCount = 2;
		m_SwapchainImageFormat = vk::Format::eB8G8R8A8Unorm;
		m_SwapchainExtent = vk::Extent2D{ m_Spec.Width, m_Spec.Height };
		RecreateSwapchain();

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
	
		// Vertex input
		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{ {}, 1, &bindingDescription,
			static_cast<uint32_t>(attributeDescriptions.size()), attributeDescriptions.data() };

		// Descriptor set layout
		vk::DescriptorSetLayoutBinding uboLayoutBinding{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex };
		vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{ {}, 1, &uboLayoutBinding };
		m_DescriptorSetLayout = m_Device->createDescriptorSetLayoutUnique(descriptorSetLayoutCreateInfo);

		// Pipeline creation
		vk::PipelineInputAssemblyStateCreateInfo inputAssembly{ {}, vk::PrimitiveTopology::eTriangleList, false };

		vk::PipelineViewportStateCreateInfo viewportState{ {}, 1, nullptr, 1, nullptr };

		vk::PipelineRasterizationStateCreateInfo rasterizer{ {}, /*depthClamp*/ false, /*rasterizeDiscard*/ false, vk::PolygonMode::eFill,
			/*cullMode*/ vk::CullModeFlagBits::eBack , /*frontFace*/ vk::FrontFace::eCounterClockwise, {}, {}, {}, {}, 1.0f };
		vk::PipelineMultisampleStateCreateInfo multisampling{ {}, vk::SampleCountFlagBits::e1, false, 1.0 };
		vk::PipelineColorBlendAttachmentState colorBlendAttachment{ {}, /*srcCol*/ vk::BlendFactor::eOne, /*dstCol*/ vk::BlendFactor::eZero,
			/*colBlend*/ vk::BlendOp::eAdd, /*srcAlpha*/ vk::BlendFactor::eOne, /*dstAlpha*/ vk::BlendFactor::eZero, /*alphaBlend*/ vk::BlendOp::eAdd,
			vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA };
		vk::PipelineColorBlendStateCreateInfo colorBlending{ {}, /*logicOpEnable=*/false,
			vk::LogicOp::eCopy, /*attachmentCount=*/1, /*colourAttachments=*/&colorBlendAttachment };

		// Pipeline layout
		vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{ {}, 1, &m_DescriptorSetLayout.get() };
		m_PipelineLayout = m_Device->createPipelineLayoutUnique(pipelineLayoutCreateInfo);

		vk::AttachmentDescription colorAttachment{ {}, m_SwapchainImageFormat, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eStore, {}, {}, {}, vk::ImageLayout::ePresentSrcKHR };
		vk::AttachmentReference colourAttachmentRef{ 0, vk::ImageLayout::eColorAttachmentOptimal };
		vk::SubpassDescription subpass{ {}, vk::PipelineBindPoint::eGraphics, /*inAttachmentCount*/ 0, nullptr, 1, &colourAttachmentRef };

		// Fence and semaphores
		m_InFlightFence = m_Device->createFenceUnique({ vk::FenceCreateFlagBits::eSignaled });
		m_ImageAvailableSemaphore = m_Device->createSemaphoreUnique({});
		m_RenderFinishedSemaphore = m_Device->createSemaphoreUnique({});

		// Graphics pipeline creation
		vk::DynamicState dynamicStates[2]{ vk::DynamicState::eViewport, vk::DynamicState::eScissor };
		vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo{ {}, 2, dynamicStates };
		vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{ 0, 1, &m_SwapchainImageFormat };

		vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo{ {}, 2, pipelineShaderStages.data(), &vertexInputInfo, &inputAssembly, nullptr, &viewportState,
			&rasterizer, &multisampling, nullptr, &colorBlending, &dynamicStateCreateInfo, *m_PipelineLayout, nullptr, 0, nullptr, 0, &pipelineRenderingCreateInfo };
		m_Pipeline = m_Device->createGraphicsPipelineUnique({}, graphicsPipelineCreateInfo).value; // Why is this the only one that requires .value?

		// Command pool
		m_CommandPool = m_Device->createCommandPoolUnique({ { vk::CommandPoolCreateFlags() | vk::CommandPoolCreateFlagBits::eResetCommandBuffer },
			static_cast<uint32_t>(graphicsQueueFamilyIndex) });

		// Vertex buffer
		vk::DeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();
		vk::Buffer vertexStagingBuffer;
		vk::DeviceMemory vertexStagingBufferMemory;
		CreateBuffer(vertexBufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
			vertexStagingBuffer, vertexStagingBufferMemory);

		void* vertData;
		vertData = m_Device->mapMemory(vertexStagingBufferMemory, 0, vertexBufferSize, {});
		memcpy(vertData, vertices.data(), (size_t)vertexBufferSize);
		m_Device->unmapMemory(vertexStagingBufferMemory);

		CreateBuffer(vertexBufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal,
			m_VertexBuffer.get(), m_VertexBufferMemory.get());

		CopyBuffer(vertexStagingBuffer, m_VertexBuffer.get(), vertexBufferSize);

		m_Device->destroyBuffer(vertexStagingBuffer);
		m_Device->freeMemory(vertexStagingBufferMemory);

		// Index buffer
		vk::DeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();
		vk::Buffer indexStagingBuffer;
		vk::DeviceMemory indexStagingBufferMemory;
		CreateBuffer(indexBufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
			indexStagingBuffer, indexStagingBufferMemory);

		void* indexData;
		indexData = m_Device->mapMemory(indexStagingBufferMemory, 0, indexBufferSize, {});
		memcpy(indexData, indices.data(), (size_t)indexBufferSize);
		m_Device->unmapMemory(indexStagingBufferMemory);

		CreateBuffer(indexBufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal,
			m_IndexBuffer.get(), m_IndexBufferMemory.get());

		CopyBuffer(indexStagingBuffer, m_IndexBuffer.get(), indexBufferSize);

		m_Device->destroyBuffer(indexStagingBuffer);
		m_Device->freeMemory(indexStagingBufferMemory);

		// Uniform buffer
		vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
		m_UniformBuffers.resize(m_SwapchainImageCount);
		m_UniformBuffersMemory.resize(m_SwapchainImageCount);
		m_UniformBuffersMapped.resize(m_SwapchainImageCount);
		for (size_t i = 0; i < m_SwapchainImageCount; i++)
		{
			CreateBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
				m_UniformBuffers[i].get(), m_UniformBuffersMemory[i].get());
			m_UniformBuffersMapped[i] = m_Device->mapMemory(m_UniformBuffersMemory[i].get(), 0, bufferSize, {});
		}

		// Descriptor pool
		vk::DescriptorPoolSize poolSize{ vk::DescriptorType::eUniformBuffer, m_SwapchainImageCount };
		vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo{ { vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet }, m_SwapchainImageCount, 1, &poolSize };
		m_DescriptorPool = m_Device->createDescriptorPoolUnique(descriptorPoolCreateInfo);

		// Descriptor sets
		std::vector<vk::DescriptorSetLayout> layouts(m_SwapchainImageCount, m_DescriptorSetLayout.get());
		vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo{ m_DescriptorPool.get(), static_cast<uint32_t>(m_SwapchainImageCount), layouts.data() };
		m_DescriptorSets.resize(m_SwapchainImageCount);
		m_DescriptorSets = m_Device->allocateDescriptorSetsUnique(descriptorSetAllocateInfo);

		for (size_t i = 0; i < m_SwapchainImageCount; i++) {
			vk::DescriptorBufferInfo bufferInfo{ m_UniformBuffers[i].get(), 0, sizeof(UniformBufferObject) };
			vk::WriteDescriptorSet descriptorWrite{ m_DescriptorSets[i].get(), 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &bufferInfo };
			m_Device->updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
		}

		// Command buffers
		m_CommandBuffers = m_Device->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(m_CommandPool.get(), vk::CommandBufferLevel::ePrimary,
			static_cast<uint32_t>(m_SwapchainImageCount)));
		RecreateCommandBuffers();
	}

	void hyper::Renderer::DrawFrame()
	{
		// FPS counter
		double currentTime = glfwGetTime();
		frameCount++;
		if (currentTime - previousTime >= 1.0)
		{
			glfwSetWindowTitle(m_Window, (m_Spec.Title + " | FPS: " + std::to_string(frameCount)).c_str());
			frameCount = 0;
			previousTime = currentTime;
		}

		if (m_FramebufferResized)
		{
			m_Device->waitIdle();
			RecreateSwapchain();
			RecreateCommandBuffers();
		}

		m_Device->waitForFences(1, &m_InFlightFence.get(), VK_TRUE, UINT64_MAX);
		m_Device->resetFences(1, &m_InFlightFence.get());

		// Get next image
		vk::ResultValue<uint32_t> imageIndex = m_Device->acquireNextImageKHR(m_Swapchain.get(), std::numeric_limits<uint64_t>::max(),
			m_ImageAvailableSemaphore.get(), {});

		if (imageIndex.result == vk::Result::eErrorOutOfDateKHR || imageIndex.result == vk::Result::eSuboptimalKHR)
			m_FramebufferResized = true;

		// Update UBO
		static double startTime = glfwGetTime();
		double uboTime = glfwGetTime();
		UniformBufferObject ubo{};
		ubo.model = glm::rotate(glm::mat4(1.0f), static_cast<float>(uboTime - startTime) * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), m_SwapchainExtent.width / (float)m_SwapchainExtent.height, 0.1f, 10.0f);
		ubo.proj[1][1] *= -1;
		memcpy(m_UniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));

		// Submit command buffer
		vk::PipelineStageFlags waitStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		m_DeviceQueue.submit(vk::SubmitInfo{ 1, &m_ImageAvailableSemaphore.get(), &waitStageMask, 1, &m_CommandBuffers[imageIndex.value].get(), 1,
			&m_RenderFinishedSemaphore.get() }, m_InFlightFence.get());

		m_PresentQueue.presentKHR({ 1, &m_RenderFinishedSemaphore.get(), 1, &m_Swapchain.get(), &imageIndex.value });
		currentFrame = (currentFrame + 1) % m_SwapchainImageCount;
	}

	hyper::Renderer::~Renderer()
	{
		m_Device->waitIdle(); // Everything will descope automatically due to unique pointers
		
		m_Device->destroyBuffer(m_VertexBuffer.get()); // Why do I need to explicitly do this? They're unique handles
		m_Device->freeMemory(m_VertexBufferMemory.get());
		m_VertexBufferMemory.get() = VK_NULL_HANDLE;
		m_VertexBuffer.get() = VK_NULL_HANDLE;

		m_Device->destroyBuffer(m_IndexBuffer.get());
		m_Device->freeMemory(m_IndexBufferMemory.get());
		m_IndexBufferMemory.get() = VK_NULL_HANDLE;
		m_IndexBuffer.get() = VK_NULL_HANDLE;

		for (size_t i = 0; i < m_SwapchainImageCount; i++)
		{
			m_Device->destroyBuffer(m_UniformBuffers[i].get());
			m_Device->freeMemory(m_UniformBuffersMemory[i].get());
			m_UniformBuffersMemory[i].get() = VK_NULL_HANDLE;
			m_UniformBuffers[i].get() = VK_NULL_HANDLE;
		}

		Logger::logger->Log("Goodbye!");
	}

	void Renderer::RecreateSwapchain()
	{
		m_FramebufferResized = false;
		int width = 0, height = 0;
		glfwGetFramebufferSize(m_Window, &width, &height);
		while (width == 0 || height == 0)
		{
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
		

		std::vector<uint32_t> FamilyIndices{ static_cast<uint32_t>(graphicsQueueFamilyIndex) };
		if (graphicsQueueFamilyIndex != presentQueueFamilyIndex)
			FamilyIndices.push_back(static_cast<uint32_t>(presentQueueFamilyIndex));

		// Recreate swapchain
		vk::SharingMode sharingMode = vk::SharingMode::eExclusive;
		uint32_t familyIndicesCount = 0;
		uint32_t* familyIndicesDataPtr = nullptr;
		if (graphicsQueueFamilyIndex != presentQueueFamilyIndex)
		{
			sharingMode = vk::SharingMode::eConcurrent;
			familyIndicesCount = 2;
			familyIndicesDataPtr = FamilyIndices.data();
		}
		m_Swapchain = m_Device->createSwapchainKHRUnique(vk::SwapchainCreateInfoKHR{ {}, m_Surface.get(), m_SwapchainImageCount, m_SwapchainImageFormat,
			vk::ColorSpaceKHR::eSrgbNonlinear, m_SwapchainExtent, 1, vk::ImageUsageFlagBits::eColorAttachment, sharingMode, familyIndicesCount, familyIndicesDataPtr,
			vk::SurfaceTransformFlagBitsKHR::eIdentity, vk::CompositeAlphaFlagBitsKHR::eOpaque, vk::PresentModeKHR::eImmediate, true, nullptr });

		Logger::logger->Log("Swapchain recreated");

		// Recreate images and image views
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
		// Dynamic state
		int width = 0, height = 0;
		glfwGetFramebufferSize(m_Window, &width, &height);
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(m_Window, &width, &height);
			glfwWaitEvents();
		}
		vk::Viewport viewport{ 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f };
		vk::Rect2D scissor{ { 0, 0 }, { (uint32_t)width, (uint32_t)height } };

		// Vertex buffers
		vk::Buffer vertexBuffers[] = { m_VertexBuffer.get() };
		vk::DeviceSize offsets[] = { 0 };

		// Background colour
		vk::ClearValue clearValues{ { 1.0f, 0.5f, 0.3f, 1.0f } };

		for (size_t i = 0; i < m_CommandBuffers.size(); i++)
		{
			// Memory barriers for synchronisation
			vk::ImageMemoryBarrier topImageMemoryBarrier{ vk::AccessFlagBits::eMemoryRead, vk::AccessFlagBits::eColorAttachmentWrite,
				vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
				m_SwapchainImages[i], vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } };
			vk::ImageMemoryBarrier bottomImageMemoryBarrier{ vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eMemoryRead,
				vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
				m_SwapchainImages[i], vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } };
			
			// Rendering info
			const vk::RenderingAttachmentInfo colorAttachmentInfo{ m_ImageViews[i].get(), vk::ImageLayout::eAttachmentOptimalKHR,
				{}, {},{}, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, clearValues };
			const vk::RenderingInfo renderingInfo{ {}, vk::Rect2D{ { 0, 0 }, m_SwapchainExtent }, 1, {}, 1, &colorAttachmentInfo };

			// Actual command buffers
			m_CommandBuffers[i]->begin(vk::CommandBufferBeginInfo{});
			m_CommandBuffers[i]->pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput,
				{}, 0, nullptr, 0, nullptr, 1, &topImageMemoryBarrier);

			m_CommandBuffers[i]->beginRendering(&renderingInfo);
			m_CommandBuffers[i]->bindPipeline(vk::PipelineBindPoint::eGraphics, *m_Pipeline);
			 
			m_CommandBuffers[i]->bindVertexBuffers(0, 1, vertexBuffers, offsets);
			m_CommandBuffers[i]->bindIndexBuffer(m_IndexBuffer.get(), 0, vk::IndexType::eUint16);

			m_CommandBuffers[i]->setViewport(0, 1, &viewport);
			m_CommandBuffers[i]->setScissor(0, 1, &scissor);

			m_CommandBuffers[i]->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_PipelineLayout, 0, 1, &m_DescriptorSets[i].get(), 0, nullptr);
			m_CommandBuffers[i]->drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

			m_CommandBuffers[i]->endRendering();

			m_CommandBuffers[i]->pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe,
				{}, 0, nullptr, 0, nullptr, 1, &bottomImageMemoryBarrier);
			m_CommandBuffers[i]->end();
		}
	}

	void Renderer::CreateBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer,
		vk::DeviceMemory& bufferMemory)
	{
		vk::BufferCreateInfo bufferCreateInfo{ {}, size, usage, vk::SharingMode::eExclusive };
		buffer = m_Device->createBuffer(bufferCreateInfo);

		vk::MemoryRequirements memRequirements = m_Device->getBufferMemoryRequirements(buffer);
		vk::PhysicalDeviceMemoryProperties memProperties = m_PhysicalDevice.getMemoryProperties();
		uint32_t memoryTypeIndex = 0;
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
			if ((memRequirements.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
				memoryTypeIndex = i;

		vk::MemoryAllocateInfo memoryAllocateInfo{ memRequirements.size, memoryTypeIndex };
		bufferMemory = m_Device->allocateMemory(memoryAllocateInfo);
			
		m_Device->bindBufferMemory(buffer, bufferMemory, 0);
	}

	void Renderer::CopyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size)
	{
		vk::CommandBufferAllocateInfo allocInfo{ m_CommandPool.get(), vk::CommandBufferLevel::ePrimary, 1 };
		std::vector<vk::CommandBuffer> commandBuffer = m_Device->allocateCommandBuffers(allocInfo);
		vk::BufferCopy copyRegion{ 0, 0, size };

		commandBuffer[0].begin(vk::CommandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
		
		commandBuffer[0].copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);
		
		commandBuffer[0].end();

		vk::SubmitInfo submitInfo{ 0, nullptr, nullptr, 1, &commandBuffer[0] };

		m_DeviceQueue.submit(submitInfo);
		m_DeviceQueue.waitIdle();

		m_Device->freeCommandBuffers(m_CommandPool.get(), commandBuffer[0]);
	}
}