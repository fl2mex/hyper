#include "Renderer.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

namespace hyper
{
	void Renderer::SetupRenderer(Spec _spec, GLFWwindow* _window)
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
			glfwExtensionsVector.push_back(vk::EXTDebugUtilsExtensionName);
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
		const std::vector<const char*> deviceExtensions = { vk::KHRSwapchainExtensionName, vk::KHRDynamicRenderingExtensionName, vk::EXTShaderObjectExtensionName };
		Logger::logger->Log("Device extensions used: "); for (auto& e : deviceExtensions) Logger::logger->Log(" - " + std::string(e));
		
		vk::PhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.samplerAnisotropy = VK_TRUE;
		vk::PhysicalDeviceShaderObjectFeaturesEXT shaderObjectFeatures = vk::PhysicalDeviceShaderObjectFeaturesEXT(1);
		vk::PhysicalDeviceDynamicRenderingFeatures dynamicFeatures = vk::PhysicalDeviceDynamicRenderingFeatures(1, &shaderObjectFeatures);
		
		m_Device = m_PhysicalDevice.createDeviceUnique(vk::DeviceCreateInfo(vk::DeviceCreateFlags(), static_cast<uint32_t>(queueCreateInfos.size()),
			queueCreateInfos.data(), 0u, nullptr, static_cast<uint32_t>(deviceExtensions.size()), deviceExtensions.data(), &deviceFeatures, &dynamicFeatures));

		// Queues
		m_DeviceQueue = m_Device->getQueue(static_cast<uint32_t>(graphicsQueueFamilyIndex), 0);
		m_PresentQueue = m_Device->getQueue(static_cast<uint32_t>(presentQueueFamilyIndex), 0);

		// VMA Allocator
		VmaAllocatorCreateInfo allocatorInfo{ {}, m_PhysicalDevice, m_Device.get(), {}, {}, {}, {}, {}, m_Instance.get(), m_Spec.ApiVersion };
		vmaCreateAllocator(&allocatorInfo, &m_Allocator);

		// Swapchain
		m_SwapchainImageCount = 2;
		m_SwapchainImageFormat = vk::Format::eB8G8R8A8Unorm;
		m_SwapchainExtent = vk::Extent2D{ m_Spec.Width, m_Spec.Height };
		RecreateSwapchain(); // Also recreates image views, and depth buffer/stencil

		// Descriptor set layout
		vk::DescriptorSetLayoutBinding uboLayoutBinding{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex };
		vk::DescriptorSetLayoutBinding samplerLayoutBinding{ 1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment };
		std::array<vk::DescriptorSetLayoutBinding, 2> bindings{ uboLayoutBinding, samplerLayoutBinding };
		vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{ {}, bindings.size(), bindings.data() };

		m_DescriptorSetLayout = m_Device->createDescriptorSetLayoutUnique(descriptorSetLayoutCreateInfo);

		// Shaders
		std::vector<char> vertShaderCode = readFile("res/shader/shader.vert.spv");
		std::vector<char> fragShaderCode = readFile("res/shader/shader.frag.spv");
		std::vector<vk::ShaderCreateInfoEXT> shaderInfos{
			{ {}, vk::ShaderStageFlagBits::eVertex, vk::ShaderStageFlagBits::eFragment, vk::ShaderCodeTypeEXT::eSpirv,
			vertShaderCode.size(), vertShaderCode.data(), "main", 1, &m_DescriptorSetLayout.get() },
			{ {}, vk::ShaderStageFlagBits::eFragment, vk::ShaderStageFlagBits{0}, vk::ShaderCodeTypeEXT::eSpirv,
			fragShaderCode.size(), fragShaderCode.data(), "main", 1,&m_DescriptorSetLayout.get() } };
		
		m_Shaders = m_Device->createShadersEXTUnique(shaderInfos, nullptr, m_DLDI).value;

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

		// Command pool
		m_CommandPool = m_Device->createCommandPoolUnique({ { vk::CommandPoolCreateFlags() | vk::CommandPoolCreateFlagBits::eResetCommandBuffer },
			static_cast<uint32_t>(graphicsQueueFamilyIndex) });

		// Need to replace this stuff with a more unique handle-friendly way instead of the c-style header
		// Staging buffer
		vk::Buffer stagingBuffer;
		vk::DeviceMemory stagingBufferMemory;

		// Texture
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load("res/texture/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		vk::DeviceSize imageSize = texWidth * texHeight * 4;
		CreateBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
			stagingBuffer, stagingBufferMemory);
		void* imageData;
		imageData = m_Device->mapMemory(stagingBufferMemory, 0, imageSize, {});
		memcpy(imageData, pixels, static_cast<size_t>(imageSize));
		m_Device->unmapMemory(stagingBufferMemory);
		stbi_image_free(pixels);
		CreateImage(texWidth, texHeight, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
			vk::MemoryPropertyFlagBits::eDeviceLocal, m_TextureImage.get(), m_TextureImageMemory.get());
		CopyImage(stagingBuffer, m_TextureImage.get(), texWidth, texHeight);

		m_Device->destroyBuffer(stagingBuffer);
		m_Device->freeMemory(stagingBufferMemory);

		// Texture image view
		vk::ImageViewCreateInfo imageViewCreateInfo{ {}, m_TextureImage.get(), vk::ImageViewType::e2D, vk::Format::eR8G8B8A8Unorm, {},
			{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } };
		m_TextureImageView = m_Device->createImageViewUnique(imageViewCreateInfo);

		// Texture sampler
		vk::SamplerCreateInfo samplerInfo{ {}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear,
			vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, {}, VK_TRUE,
			m_PhysicalDevice.getProperties().limits.maxSamplerAnisotropy, VK_FALSE, vk::CompareOp::eAlways, {}, {}, vk::BorderColor::eIntOpaqueBlack, VK_FALSE };
		m_Sampler = m_Device->createSamplerUnique(samplerInfo);

		// Vertex buffer
		vk::DeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();
		CreateBuffer(vertexBufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
			stagingBuffer, stagingBufferMemory);
		void* vertData;
		vertData = m_Device->mapMemory(stagingBufferMemory, 0, vertexBufferSize, {});
		memcpy(vertData, vertices.data(), (size_t)vertexBufferSize);
		m_Device->unmapMemory(stagingBufferMemory);
		CreateBuffer(vertexBufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal,
			m_VertexBuffer.get(), m_VertexBufferMemory.get());
		CopyBuffer(stagingBuffer, m_VertexBuffer.get(), vertexBufferSize);

		m_Device->destroyBuffer(stagingBuffer);
		m_Device->freeMemory(stagingBufferMemory);

		// Index buffer
		vk::DeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();
		CreateBuffer(indexBufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
			stagingBuffer, stagingBufferMemory);
		void* indexData;
		indexData = m_Device->mapMemory(stagingBufferMemory, 0, indexBufferSize, {});
		memcpy(indexData, indices.data(), (size_t)indexBufferSize);
		m_Device->unmapMemory(stagingBufferMemory);
		CreateBuffer(indexBufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal,
			m_IndexBuffer.get(), m_IndexBufferMemory.get());
		CopyBuffer(stagingBuffer, m_IndexBuffer.get(), indexBufferSize);

		m_Device->destroyBuffer(stagingBuffer);
		m_Device->freeMemory(stagingBufferMemory);

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
		std::vector<vk::DescriptorPoolSize> poolSizes = { { vk::DescriptorType::eUniformBuffer, m_SwapchainImageCount },
			{ vk::DescriptorType::eCombinedImageSampler, m_SwapchainImageCount } };	
		vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo{ { vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet }, m_SwapchainImageCount,
			2, poolSizes.data() };
		m_DescriptorPool = m_Device->createDescriptorPoolUnique(descriptorPoolCreateInfo);

		// Descriptor sets
		std::vector<vk::DescriptorSetLayout> layouts(m_SwapchainImageCount, m_DescriptorSetLayout.get());
		vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo{ m_DescriptorPool.get(), static_cast<uint32_t>(m_SwapchainImageCount), layouts.data() };
		m_DescriptorSets.resize(m_SwapchainImageCount);
		m_DescriptorSets = m_Device->allocateDescriptorSetsUnique(descriptorSetAllocateInfo);

		for (size_t i = 0; i < m_SwapchainImageCount; i++)
		{
			vk::DescriptorBufferInfo bufferInfo{ m_UniformBuffers[i].get(), 0, sizeof(UniformBufferObject) };
			vk::DescriptorImageInfo imageInfo{ m_Sampler.get(), m_TextureImageView.get(), vk::ImageLayout::eShaderReadOnlyOptimal };
			std::vector<vk::WriteDescriptorSet> descriptorWrites{
				vk::WriteDescriptorSet{ m_DescriptorSets[i].get(), 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &bufferInfo },
				vk::WriteDescriptorSet{ m_DescriptorSets[i].get(), 1, 0, 1, vk::DescriptorType::eCombinedImageSampler, &imageInfo }	};

			m_Device->updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		}

		// Command buffers
		m_CommandBuffers = m_Device->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(m_CommandPool.get(), vk::CommandBufferLevel::ePrimary,
			static_cast<uint32_t>(m_SwapchainImageCount)));
		RecreateCommandBuffers();
	}

	void Renderer::DrawFrame()
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

	Renderer::~Renderer()	
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

		m_Device->destroyImage(m_TextureImage.get());
		m_Device->freeMemory(m_TextureImageMemory.get());
		m_TextureImageMemory.get() = VK_NULL_HANDLE;
		m_TextureImage.get() = VK_NULL_HANDLE;

		m_Device->destroyImage(m_DepthImage.get());	
		m_Device->freeMemory(m_DepthImageMemory.get());
		m_DepthImageMemory.get() = VK_NULL_HANDLE;
		m_DepthImage.get() = VK_NULL_HANDLE;

		vmaDestroyAllocator(m_Allocator);

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

		m_Device->destroy(m_DepthImage.get());
		m_Device->free(m_DepthImageMemory.get());
		m_Device->destroy(m_DepthImageView.get());
		m_DepthImage.get() = VK_NULL_HANDLE;
		m_DepthImageMemory.get() = VK_NULL_HANDLE;
		m_DepthImageView.get() = VK_NULL_HANDLE;

		m_Device->destroySwapchainKHR(m_Swapchain.get());
		m_Swapchain.get() = VK_NULL_HANDLE;

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

		// Recreate depth image
		vk::Format depthFormat = vk::Format::eD32Sfloat;
		std::vector<vk::Format> candidates{ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint };
		vk::ImageTiling tiling = vk::ImageTiling::eOptimal;
		vk::FormatFeatureFlags features = vk::FormatFeatureFlagBits::eDepthStencilAttachment;
		for (vk::Format format : candidates) {
			vk::FormatProperties props = m_PhysicalDevice.getFormatProperties(format);
			if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
				depthFormat = format;
				break;
			}
			else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
				depthFormat = format;
				break;
			}
		}

		CreateImage(m_SwapchainExtent.width, m_SwapchainExtent.height, depthFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment,
			vk::MemoryPropertyFlagBits::eDeviceLocal, m_DepthImage.get(), m_DepthImageMemory.get());
		vk::ImageViewCreateInfo depthImageViewCreateInfo{ {}, m_DepthImage.get(), vk::ImageViewType::e2D, depthFormat, {},
			{ vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1 } };
		m_DepthImageView = m_Device->createImageViewUnique(depthImageViewCreateInfo);
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
		std::vector<vk::ClearValue> clearValues{ {{ 1.0f, 0.5f, 0.3f, 1.0f }}, {{ 1.0f, 0 }} };

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
			const std::vector<vk::RenderingAttachmentInfo> attachments{ { m_ImageViews[i].get(), vk::ImageLayout::eAttachmentOptimalKHR, {},{},{}, // Colour
				vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, clearValues[0] } };
			const vk::RenderingAttachmentInfo depthAttachment{ m_DepthImageView.get(), vk::ImageLayout::eDepthStencilAttachmentOptimal, {}, {}, {}, // Depth
					vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare, clearValues[1] };

			const vk::RenderingInfo renderingInfo{ {}, vk::Rect2D{ { 0, 0 }, m_SwapchainExtent }, 1, {}, attachments, &depthAttachment };

			// Actual command buffers
			m_CommandBuffers[i]->begin(vk::CommandBufferBeginInfo{});
			m_CommandBuffers[i]->pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput,
				{}, 0, nullptr, 0, nullptr, 1, &topImageMemoryBarrier);

			// Get ready for the motherload of boilerplate from using ShaderEXT's
			std::vector<vk::VertexInputBindingDescription2EXT> bindings{ Vertex::getBindingDescription() };
			m_CommandBuffers[i]->setVertexInputEXT(bindings.size(), bindings.data(),
				Vertex::getAttributeDescriptions().size(), Vertex::getAttributeDescriptions().data(), m_DLDI);
			m_CommandBuffers[i]->setViewportWithCount(viewport);
			m_CommandBuffers[i]->setScissorWithCount(scissor);
			m_CommandBuffers[i]->setRasterizerDiscardEnable(0);
			m_CommandBuffers[i]->setPolygonModeEXT(vk::PolygonMode::eFill, m_DLDI);
			m_CommandBuffers[i]->setRasterizationSamplesEXT(vk::SampleCountFlagBits::e1, m_DLDI);
			m_CommandBuffers[i]->setCullMode(vk::CullModeFlagBits::eNone);
			m_CommandBuffers[i]->setFrontFace(vk::FrontFace::eCounterClockwise);
			m_CommandBuffers[i]->setDepthTestEnable(1);
			m_CommandBuffers[i]->setDepthWriteEnable(1);
			m_CommandBuffers[i]->setDepthCompareOp(vk::CompareOp::eLess);
			std::vector<vk::Bool32> colorBlends{ 1/*vk::BlendFactor::eOne*/, 0/*vk::BlendFactor::eZero*/, 1/*vk::BlendOp::eAdd*/ };
			m_CommandBuffers[i]->setColorBlendEnableEXT(0, colorBlends.size(), colorBlends.data(), m_DLDI);
			m_CommandBuffers[i]->setColorBlendEquationEXT(0, { { vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd } }, m_DLDI);
			m_CommandBuffers[i]->setColorWriteMaskEXT(0, vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
				| vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA, m_DLDI);
			m_CommandBuffers[i]->setSampleMaskEXT(vk::SampleCountFlagBits::e1, 1, m_DLDI);
			m_CommandBuffers[i]->setAlphaToCoverageEnableEXT(0, m_DLDI);
			m_CommandBuffers[i]->setDepthBiasEnable(0);
			m_CommandBuffers[i]->setStencilTestEnable(0);
			m_CommandBuffers[i]->setPrimitiveTopology(vk::PrimitiveTopology::eTriangleList);
			m_CommandBuffers[i]->setPrimitiveRestartEnable(0);

			m_CommandBuffers[i]->beginRendering(&renderingInfo);
			vk::ShaderStageFlagBits stages[2]{ vk::ShaderStageFlagBits::eVertex, vk::ShaderStageFlagBits::eFragment };
			m_CommandBuffers[i]->bindShadersEXT(stages, { m_Shaders[0].get(), m_Shaders[1].get() }, m_DLDI);

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
			{
				memoryTypeIndex = i;
				break;
			}

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

	void Renderer::CreateImage(int width, int height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags imageUsage, vk::MemoryPropertyFlags properties,
		vk::Image& image, vk::DeviceMemory& imageMemory)
	{
		vk::ImageCreateInfo imageInfo{ {}, vk::ImageType::e2D, format, { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 },
			1, 1, vk::SampleCountFlagBits::e1, tiling, imageUsage, vk::SharingMode::eExclusive };
		image = m_Device->createImage(imageInfo);

		vk::MemoryRequirements memRequirements = m_Device->getImageMemoryRequirements(image);
		vk::PhysicalDeviceMemoryProperties memProperties = m_PhysicalDevice.getMemoryProperties();
		uint32_t memoryTypeIndex = 0;
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
			if ((memRequirements.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				memoryTypeIndex = i;
				break;
			}

		vk::MemoryAllocateInfo memoryAllocateInfo{ memRequirements.size, memoryTypeIndex };
		imageMemory = m_Device->allocateMemory(memoryAllocateInfo);

		m_Device->bindImageMemory(image, imageMemory, 0);
	}

	void Renderer::CopyImage(vk::Buffer srcBuffer, vk::Image dstImage, int width, int height)
	{
		vk::CommandBufferAllocateInfo allocInfo{ m_CommandPool.get(), vk::CommandBufferLevel::ePrimary, 1 };
		std::vector<vk::CommandBuffer> commandBuffer = m_Device->allocateCommandBuffers(allocInfo);
		vk::BufferImageCopy copyRegion{ 0, 0, 0, {vk::ImageAspectFlagBits::eColor, 0, 0, 1 }, { 0, 0, 0 },
			{ static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 } };

		vk::ImageMemoryBarrier topImageMemoryBarrier{ vk::AccessFlagBits::eNone, vk::AccessFlagBits::eTransferWrite,
			vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
			dstImage, vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } };
		vk::ImageMemoryBarrier bottomImageMemoryBarrier{ vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead,
			vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
			dstImage, vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } };

		commandBuffer[0].begin(vk::CommandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

		commandBuffer[0].pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer,
			{}, 0, nullptr, 0, nullptr, 1, &topImageMemoryBarrier);

		commandBuffer[0].copyBufferToImage(srcBuffer, dstImage, vk::ImageLayout::eTransferDstOptimal, 1, &copyRegion);

		commandBuffer[0].pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
			{}, 0, nullptr, 0, nullptr, 1, &bottomImageMemoryBarrier
		);

		commandBuffer[0].end();

		vk::SubmitInfo submitInfo{ 0, nullptr, nullptr, 1, &commandBuffer[0] };

		m_DeviceQueue.submit(submitInfo);
		m_DeviceQueue.waitIdle();

		m_Device->freeCommandBuffers(m_CommandPool.get(), commandBuffer[0]);
	}

}