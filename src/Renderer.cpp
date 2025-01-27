#include "Renderer.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "imgui_impl_glfw.h"

#include "Logger.h"
#include "File.h"

namespace hyper
{
	void Renderer::SetupRenderer(Spec _spec, GLFWwindow* _window)
	{
#pragma region StuffThatDoesntReallyNeedToBeTouchedOrSeenAfterBeingSetup
		m_Spec = _spec;
		m_Window = _window;

		vk::ApplicationInfo appInfo{ m_Spec.Title.c_str(), m_Spec.ApiVersion, "hyper", m_Spec.ApiVersion, m_Spec.ApiVersion };

		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		std::vector<const char*> glfwExtensionsVector(glfwExtensions, glfwExtensions + glfwExtensionCount), layers;

		Logger::logger->Log("Debug Enabled");
		if (m_Spec.Debug)
		{
			glfwExtensionsVector.push_back(vk::EXTDebugUtilsExtensionName);
			layers.push_back("VK_LAYER_KHRONOS_validation");
		}

		Logger::logger->Log("Extensions used: "); for (uint32_t i = 0; i < glfwExtensionCount; i++) Logger::logger->Log(" - " + std::string(glfwExtensions[i]));
		Logger::logger->Log("Layers used: "); for (auto& l : layers) Logger::logger->Log(" - " + std::string(l));

		// Instance
		m_Instance = vk::createInstanceUnique(vk::InstanceCreateInfo{ vk::InstanceCreateFlags(), &appInfo, static_cast<uint32_t>(layers.size()), layers.data(),
			static_cast<uint32_t>(glfwExtensionsVector.size()), glfwExtensionsVector.data() });

		// DLDI and debug messenger
		m_DLDI = vk::detail::DispatchLoaderDynamic(*m_Instance, vkGetInstanceProcAddr);
		if (m_Spec.Debug)
			m_DebugMessenger = Logger::logger->MakeDebugMessenger(m_Instance, m_DLDI);

		// Surface
		VkSurfaceKHR surfaceTmp;
		glfwCreateWindowSurface(*m_Instance, m_Window, nullptr, &surfaceTmp);
		m_Surface = vk::UniqueSurfaceKHR(surfaceTmp, *m_Instance);

		// Physical device
		std::vector<vk::PhysicalDevice> physicalDevices = m_Instance->enumeratePhysicalDevices();
		Logger::logger->Log("Devices available: ");
		for (auto& d : physicalDevices)
			Logger::logger->Log(" - " + std::string(d.getProperties().deviceName.data()));
		m_PhysicalDevice = physicalDevices[0]; // Set physical device to the first one, then check for a discrete gpu and set it to that
		for (auto& d : physicalDevices)
			if (d.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
				m_PhysicalDevice = d;
		Logger::logger->Log("Chose device: " + std::string(m_PhysicalDevice.getProperties().deviceName.data()));

		// Queue families
		std::vector<vk::QueueFamilyProperties> queueFamilyProperties = m_PhysicalDevice.getQueueFamilyProperties();
		for (uint32_t i = 0; i < queueFamilyProperties.size(); i++)
			if (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eCompute)
			{
				m_GraphicsIndex = i;
				break;
			}
		for (uint32_t i = 0; i < queueFamilyProperties.size(); i++)
			if (m_PhysicalDevice.getSurfaceSupportKHR(i, m_Surface.get()))
				m_PresentIndex = i;
		std::vector<uint32_t> familyIndices{ m_GraphicsIndex };
		if (m_GraphicsIndex != m_PresentIndex)
			familyIndices.push_back(m_PresentIndex);

		std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
		float queuePriority = 0.0f;
		for (auto& queueFamilyIndex : familyIndices)
			queueCreateInfos.push_back(vk::DeviceQueueCreateInfo{ vk::DeviceQueueCreateFlags(), static_cast<uint32_t>(queueFamilyIndex), 1, &queuePriority });

		// Logical device
		const std::vector<const char*> deviceExtensions = { vk::KHRSwapchainExtensionName, vk::KHRDynamicRenderingExtensionName,
			vk::EXTShaderObjectExtensionName, vk::KHRBufferDeviceAddressExtensionName };//, vk::EXTDescriptorIndexingExtensionName }; // For later
		Logger::logger->Log("Device extensions used: "); for (auto& e : deviceExtensions) Logger::logger->Log(" - " + std::string(e));
		
		vk::PhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.samplerAnisotropy = VK_TRUE;
		//vk::PhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures = vk::PhysicalDeviceDescriptorIndexingFeatures(); // For later
		vk::PhysicalDeviceBufferDeviceAddressFeatures bufferAddressFeatures = vk::PhysicalDeviceBufferDeviceAddressFeatures(1, {}, {});// , & descriptorIndexingFeatures);  // For later
		vk::PhysicalDeviceShaderObjectFeaturesEXT shaderObjectFeatures = vk::PhysicalDeviceShaderObjectFeaturesEXT(1, &bufferAddressFeatures);
		vk::PhysicalDeviceDynamicRenderingFeatures dynamicFeatures = vk::PhysicalDeviceDynamicRenderingFeatures(1, &shaderObjectFeatures);
		m_Device = m_PhysicalDevice.createDeviceUnique(vk::DeviceCreateInfo(vk::DeviceCreateFlags(), static_cast<uint32_t>(queueCreateInfos.size()),
			queueCreateInfos.data(), 0, nullptr, static_cast<uint32_t>(deviceExtensions.size()), deviceExtensions.data(), &deviceFeatures, &dynamicFeatures));

		// Queues
		m_DeviceQueue = m_Device->getQueue(m_GraphicsIndex, 0);
		m_PresentQueue = m_Device->getQueue(m_PresentIndex, 0);

		// VMA Allocator
		VmaAllocatorCreateInfo allocatorInfo{ VmaAllocatorCreateFlags{} | VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
			m_PhysicalDevice, m_Device.get(), {}, {}, {}, {}, {}, m_Instance.get(), m_Spec.ApiVersion };
		vmaCreateAllocator(&allocatorInfo, &m_Allocator);
				
		// Swapchain
		m_Swapchain.CreateSwapchain(2, vk::Format::eB8G8R8A8Unorm, { m_Spec.Width, m_Spec.Height }, m_Window, m_Device.get(),
			m_GraphicsIndex, m_PresentIndex, m_Surface.get());
		Logger::logger->Log("Swapchain created: Using " + std::to_string(m_Swapchain.ImageCount) + " frames in flight");

		m_DepthImage = CreateImage(m_Allocator, m_Device.get(), m_Swapchain.Extent, vk::Format::eD32Sfloat, vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eDepthStencilAttachment, VMA_MEMORY_USAGE_GPU_ONLY);

		// Command pool
		m_CommandPool = m_Device->createCommandPoolUnique({ { vk::CommandPoolCreateFlags() | vk::CommandPoolCreateFlagBits::eResetCommandBuffer },
			static_cast<uint32_t>(m_GraphicsIndex) });

		// Fence and semaphores
		m_InFlightFence = m_Device->createFenceUnique({ vk::FenceCreateFlagBits::eSignaled });
		m_ImageAvailableSemaphore = m_Device->createSemaphoreUnique({});
		m_RenderFinishedSemaphore = m_Device->createSemaphoreUnique({});
#pragma endregion

		// Descriptor set layout
		vk::DescriptorSetLayoutBinding uboLayoutBinding{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex };
		vk::DescriptorSetLayoutBinding samplerLayoutBinding{ 1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment };
		std::array<vk::DescriptorSetLayoutBinding, 2> bindings{ uboLayoutBinding, samplerLayoutBinding };
		m_DescriptorSetLayout = m_Device->createDescriptorSetLayoutUnique({ {}, static_cast<uint32_t>(bindings.size()), bindings.data() });

		// Pipeline layout
		vk::PushConstantRange pushConstantRange{ vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstantData) };
		m_PipelineLayout = m_Device->createPipelineLayoutUnique({ {}, 1, &m_DescriptorSetLayout.get(), 1, &pushConstantRange });

		// Shaders
		std::vector<char> vertShaderCode = readFile("res/shader/shader.vert.spv");
		std::vector<char> fragShaderCode = readFile("res/shader/shader.frag.spv");
		std::vector<vk::ShaderCreateInfoEXT> shaderInfos{
			{ {}, vk::ShaderStageFlagBits::eVertex, vk::ShaderStageFlagBits::eFragment, vk::ShaderCodeTypeEXT::eSpirv,
			vertShaderCode.size(), vertShaderCode.data(), "main", 1, &m_DescriptorSetLayout.get(), 1, &pushConstantRange },
			{ {}, vk::ShaderStageFlagBits::eFragment, vk::ShaderStageFlagBits{0}, vk::ShaderCodeTypeEXT::eSpirv,
			fragShaderCode.size(), fragShaderCode.data(), "main", 1,&m_DescriptorSetLayout.get(), 1, &pushConstantRange } };
		m_Shaders = m_Device->createShadersEXTUnique(shaderInfos, nullptr, m_DLDI).value;

		vk::AttachmentDescription colorAttachment{ {}, m_Swapchain.ImageFormat, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eStore, {}, {}, {}, vk::ImageLayout::ePresentSrcKHR };
		vk::AttachmentReference colourAttachmentRef{ 0, vk::ImageLayout::eColorAttachmentOptimal };
		vk::SubpassDescription subpass{ {}, vk::PipelineBindPoint::eGraphics, /*inAttachmentCount*/ 0, nullptr, 1, &colourAttachmentRef };

		// Images
		m_TextureImage = CreateImageTexture(m_Allocator, m_CommandPool.get(), m_Device.get(), m_DeviceQueue, "res/texture/texture.jpg",
			vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eSampled);
		std::array<uint32_t, 16 * 16 > pixels = { 0 };
		for (int x = 0; x < 16; x++) 
			for (int y = 0; y < 16; y++) 
				pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? glm::packUnorm4x8(glm::vec4(1, 0, 1, 1)) : glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
		m_ErrorCheckerboardImage = CreateImageStaged(m_Allocator, m_CommandPool.get(), m_Device.get(), m_DeviceQueue, { 16, 16 }, pixels.data(),
			vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eSampled);

		// Texture samplers
		vk::SamplerCreateInfo samplerInfo{ {}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eNearest,
			vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, {}, VK_TRUE,
			m_PhysicalDevice.getProperties().limits.maxSamplerAnisotropy, VK_FALSE, vk::CompareOp::eAlways, {}, {}, vk::BorderColor::eIntOpaqueBlack, VK_FALSE };
		m_LinearSampler = m_Device->createSamplerUnique(samplerInfo);
		samplerInfo.magFilter = vk::Filter::eNearest;
		samplerInfo.minFilter = vk::Filter::eNearest;
		m_NearestSampler = m_Device->createSamplerUnique(samplerInfo);

		// Meshes
		testMeshes = LoadModel(m_CommandPool.get(), m_Device.get(), m_DeviceQueue, m_Allocator, "res/model/basicmesh.glb");

		// Uniform Buffer
		m_UniformBuffers.resize(m_Swapchain.ImageCount);
		for (auto& ub : m_UniformBuffers)
			ub = CreateBuffer(m_Allocator, sizeof(UniformBufferObject), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
		
		// Descriptor pool
		std::vector<vk::DescriptorPoolSize> poolSizes = { { vk::DescriptorType::eUniformBuffer, m_Swapchain.ImageCount },
			{ vk::DescriptorType::eCombinedImageSampler, m_Swapchain.ImageCount } };
		m_DescriptorPool = m_Device->createDescriptorPoolUnique({ { vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet }, m_Swapchain.ImageCount,
			2, poolSizes.data() });

		// Descriptor sets
		std::vector<vk::DescriptorSetLayout> layouts(m_Swapchain.ImageCount, m_DescriptorSetLayout.get());
		vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo{ m_DescriptorPool.get(), static_cast<uint32_t>(m_Swapchain.ImageCount), layouts.data() };
		m_DescriptorSets.resize(m_Swapchain.ImageCount);
		m_DescriptorSets = m_Device->allocateDescriptorSetsUnique(descriptorSetAllocateInfo);
		for (size_t i = 0; i < m_Swapchain.ImageCount; i++)
		{
			vk::DescriptorBufferInfo bufferInfo{ m_UniformBuffers[i].Buffer, 0, sizeof(UniformBufferObject) };
			vk::DescriptorImageInfo imageInfo{ m_NearestSampler.get(), m_ErrorCheckerboardImage.ImageView, vk::ImageLayout::eShaderReadOnlyOptimal };
			std::vector<vk::WriteDescriptorSet> descriptorWrites{
				vk::WriteDescriptorSet{ m_DescriptorSets[i].get(), 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &bufferInfo },
				vk::WriteDescriptorSet{ m_DescriptorSets[i].get(), 1, 0, 1, vk::DescriptorType::eCombinedImageSampler, &imageInfo }	};
			m_Device->updateDescriptorSets(static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}

		// ImGui
		ImGui::CreateContext();
		ImGui_ImplGlfw_InitForVulkan(m_Window, true);
		ImGui_ImplVulkan_InitInfo imGuiInfo{ m_Instance.get(), m_PhysicalDevice, m_Device.get(), static_cast<uint32_t>(m_GraphicsIndex), m_DeviceQueue,
			nullptr, nullptr, m_Swapchain.ImageCount, m_Swapchain.ImageCount, VK_SAMPLE_COUNT_1_BIT, nullptr, 0, 2, true,
			vk::PipelineRenderingCreateInfoKHR{ 0, 1, &m_Swapchain.ImageFormat, vk::Format::eD32Sfloat } };
		ImGui_ImplVulkan_Init(&imGuiInfo);

		// Command buffers
		m_CommandBuffers = m_Device->allocateCommandBuffersUnique({ m_CommandPool.get(), vk::CommandBufferLevel::ePrimary, m_Swapchain.ImageCount });
	}

	void Renderer::DrawFrame()
	{
		static_cast<void>(m_Device->waitForFences(1, &m_InFlightFence.get(), VK_TRUE, UINT64_MAX));
		
		if (m_Swapchain.Resized)
		{
			m_Swapchain.CreateSwapchain(2, vk::Format::eB8G8R8A8Unorm, { m_Spec.Width, m_Spec.Height }, m_Window, m_Device.get(),
				m_GraphicsIndex, m_PresentIndex, m_Surface.get());
			DestroyImage(m_Allocator, m_Device.get(), m_DepthImage);
			m_DepthImage = CreateImage(m_Allocator, m_Device.get(), m_Swapchain.Extent, vk::Format::eD32Sfloat, vk::ImageTiling::eOptimal,
				vk::ImageUsageFlagBits::eDepthStencilAttachment, VMA_MEMORY_USAGE_GPU_ONLY);
			Logger::logger->Log("Swapchain rereated");
		}

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		{ // Demo window
			ImGui::ShowDemoWindow();
		}
		static std::vector<vk::ClearValue> clearValues{ vk::ClearColorValue{ 1.0f, 0.5f, 0.3f, 1.0f }, vk::ClearColorValue{ 1.0f, 0.0f, 0.0f, 0.0f } };

		{ // Custom window, change clear value
			ImGui::Begin("Clear Colour");
			ImGui::ColorEdit4("Clear Colour", clearValues[0].color.float32.data()); // wtf is this???
			ImGui::End();
		}

		ImGui::Render();

		//vk::Buffer vertexBuffers[] = { m_VertexBuffer.Buffer };
		//vk::DeviceSize offsets[] = { 0 };
		PushConstantData pushConstants{};
		pushConstants.vertexBuffer = m_Device->getBufferAddress({ testMeshes[2]->vertexBuffer.Buffer });
		for (size_t i = 0; i < m_CommandBuffers.size(); i++)
		{
			// Memory barriers for synchronisation
			vk::ImageMemoryBarrier topImageMemoryBarrier{ vk::AccessFlagBits::eMemoryRead, vk::AccessFlagBits::eColorAttachmentWrite,
				vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
				m_Swapchain.Images[i], vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } };
			vk::ImageMemoryBarrier bottomImageMemoryBarrier{ vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eMemoryRead,
				vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
				m_Swapchain.Images[i], vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } };

			const std::vector<vk::RenderingAttachmentInfo> attachments{ { m_Swapchain.ImageViews[i].get(), vk::ImageLayout::eAttachmentOptimalKHR, {},{},{}, // Colour
				vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, clearValues[0] } };
			const vk::RenderingAttachmentInfo depthAttachment{ m_DepthImage.ImageView, vk::ImageLayout::eDepthStencilAttachmentOptimal, {}, {}, {}, // Depth
					vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare, clearValues[1] };

			const vk::RenderingInfo renderingInfo{ {}, vk::Rect2D{ { 0, 0 }, m_Swapchain.Extent }, 1, {}, attachments, &depthAttachment };

			// Actual command buffers
			m_CommandBuffers[i]->begin(vk::CommandBufferBeginInfo{}); // vk::CommandBufferUsageFlagBits::eOneTimeSubmit // How to fix?
			m_CommandBuffers[i]->pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput,
				{}, 0, nullptr, 0, nullptr, 1, &topImageMemoryBarrier);

			// Get ready for the motherload of boilerplate from using ShaderEXT's
			std::vector<vk::VertexInputBindingDescription2EXT> bindings{ Vertex::getBindingDescription() };
			m_CommandBuffers[i]->setVertexInputEXT(static_cast<uint32_t>(bindings.size()), bindings.data(),
				static_cast<uint32_t>(Vertex::getAttributeDescriptions().size()), Vertex::getAttributeDescriptions().data(), m_DLDI);
			m_CommandBuffers[i]->setViewportWithCount(vk::Viewport{ 0.0f, 0.0f, static_cast<float>(m_Swapchain.Extent.width), static_cast<float>(m_Swapchain.Extent.height), 0.0f, 1.0f });
			m_CommandBuffers[i]->setScissorWithCount(vk::Rect2D{ { 0, 0 }, { m_Swapchain.Extent.width, m_Swapchain.Extent.height } });
			m_CommandBuffers[i]->setRasterizerDiscardEnable(0);
			m_CommandBuffers[i]->setPolygonModeEXT(vk::PolygonMode::eFill, m_DLDI);
			m_CommandBuffers[i]->setRasterizationSamplesEXT(vk::SampleCountFlagBits::e1, m_DLDI);
			m_CommandBuffers[i]->setCullMode(vk::CullModeFlagBits::eNone);
			m_CommandBuffers[i]->setFrontFace(vk::FrontFace::eCounterClockwise);

			m_CommandBuffers[i]->setDepthTestEnable(1);
			m_CommandBuffers[i]->setDepthWriteEnable(1);
			m_CommandBuffers[i]->setDepthCompareOp(vk::CompareOp::eLess);
			m_CommandBuffers[i]->setDepthBiasEnable(0);

			m_CommandBuffers[i]->setColorBlendEnableEXT(0, { 1/*vk::BlendFactor::eOne*/, 0/*vk::BlendFactor::eZero*/, 1/*vk::BlendOp::eAdd*/ }, m_DLDI);
			m_CommandBuffers[i]->setColorBlendEquationEXT(0, vk::ColorBlendEquationEXT{ vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd }, m_DLDI);
			m_CommandBuffers[i]->setColorWriteMaskEXT(0, vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB
				| vk::ColorComponentFlagBits::eA, m_DLDI);

			m_CommandBuffers[i]->setSampleMaskEXT(vk::SampleCountFlagBits::e1, 1, m_DLDI);
			m_CommandBuffers[i]->setAlphaToCoverageEnableEXT(0, m_DLDI);
			m_CommandBuffers[i]->setStencilTestEnable(0);
			m_CommandBuffers[i]->setPrimitiveTopology(vk::PrimitiveTopology::eTriangleList);
			m_CommandBuffers[i]->setPrimitiveRestartEnable(0);

			m_CommandBuffers[i]->beginRendering(&renderingInfo);
			vk::ShaderStageFlagBits stages[2];
			m_CommandBuffers[i]->bindShadersEXT({ vk::ShaderStageFlagBits::eVertex, vk::ShaderStageFlagBits::eFragment }, { m_Shaders[0].get(), m_Shaders[1].get() }, m_DLDI);

			//m_CommandBuffers[i]->bindVertexBuffers(0, 1, vertexBuffers, offsets); // Using push constants atm, probably not for long
			m_CommandBuffers[i]->bindIndexBuffer(testMeshes[2]->indexBuffer.Buffer, 0, vk::IndexType::eUint32);

			m_CommandBuffers[i]->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_PipelineLayout, 0, 1, &m_DescriptorSets[i].get(), 0, nullptr);
			m_CommandBuffers[i]->pushConstants(*m_PipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstantData), &pushConstants);

			m_CommandBuffers[i]->drawIndexed(testMeshes[2]->surfaces[0].count, 1, testMeshes[2]->surfaces[0].startIndex, 0, 0);

			
			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_CommandBuffers[i].get());

			m_CommandBuffers[i]->endRendering();

			m_CommandBuffers[i]->pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe,
				{}, 0, nullptr, 0, nullptr, 1, &bottomImageMemoryBarrier);
			m_CommandBuffers[i]->end();
		}

		static_cast<void>(m_Device->resetFences(1, &m_InFlightFence.get()));
	
		// Get next image
		vk::ResultValue<uint32_t> imageIndex = m_Device->acquireNextImageKHR(m_Swapchain.ActualSwapchain.get(), std::numeric_limits<uint64_t>::max(),
			m_ImageAvailableSemaphore.get(), {});

		if (imageIndex.result == vk::Result::eErrorOutOfDateKHR || imageIndex.result == vk::Result::eSuboptimalKHR)
			m_Swapchain.Resized = true;

		// Update UBO
		static double startTime = glfwGetTime();
		double uboTime = glfwGetTime();
		UniformBufferObject ubo{};
		ubo.model = glm::rotate(glm::mat4(1.0f), static_cast<float>(uboTime - startTime) * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		ubo.view = glm::lookAt(glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), m_Swapchain.Extent.width / (float)m_Swapchain.Extent.height, 0.1f, 10.0f);
		ubo.proj[1][1] *= -1;
		memcpy(m_UniformBuffers[currentFrame].AllocationInfo.pMappedData, &ubo, sizeof(ubo));
		
		// Submit command buffer
		vk::PipelineStageFlags waitStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		m_DeviceQueue.submit(vk::SubmitInfo{ 1, &m_ImageAvailableSemaphore.get(), &waitStageMask, 1, &m_CommandBuffers[imageIndex.value].get(),
			1, &m_RenderFinishedSemaphore.get() }, m_InFlightFence.get());

		static_cast<void>(m_PresentQueue.presentKHR({ 1, &m_RenderFinishedSemaphore.get(), 1, &m_Swapchain.ActualSwapchain.get(), &imageIndex.value }));
		currentFrame = (currentFrame + 1) % m_Swapchain.ImageCount;
	}

	Renderer::~Renderer()	
	{
		m_Device->waitIdle(); // Everything will descope automatically due to unique pointers

		for (auto& ub : m_UniformBuffers)
			DestroyBuffer(m_Allocator, ub);

		DestroyImage(m_Allocator, m_Device.get(), m_DepthImage);
		DestroyImage(m_Allocator, m_Device.get(), m_TextureImage);
		DestroyImage(m_Allocator, m_Device.get(), m_ErrorCheckerboardImage);

		for (std::shared_ptr<MeshAsset> meshAsset : testMeshes)
		{
			DestroyBuffer(m_Allocator, meshAsset->vertexBuffer);
			DestroyBuffer(m_Allocator, meshAsset->indexBuffer);
		}

		vmaDestroyAllocator(m_Allocator);

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		Logger::logger->Log("Goodbye!");
	}
}