#include "Renderer.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>

#include "Logger.h"
#include "File.h"

namespace hyper
{
	std::optional<std::vector<std::shared_ptr<MeshAsset>>> LoadModel(vk::CommandPool commandPool, vk::Device device, vk::Queue queue, VmaAllocator allocator,
		std::filesystem::path filePath)
	{
		auto gltfFile = fastgltf::GltfDataBuffer::FromPath(filePath);
		if (gltfFile.error() != fastgltf::Error::None) return {};
		constexpr auto gltfOptions = fastgltf::Options::LoadExternalBuffers;
		fastgltf::Parser parser{};
		auto load = parser.loadGltfBinary(gltfFile.get(), filePath.parent_path(), gltfOptions);
		fastgltf::Asset gltf;
		if (load) gltf = std::move(load.get());
		else return {};
		std::vector<std::shared_ptr<MeshAsset>> meshes;

		std::vector<uint32_t> indices;
		std::vector<Vertex> vertices;

		for (fastgltf::Mesh& mesh : gltf.meshes) {
			MeshAsset newmesh;

			newmesh.name = mesh.name;

			// clear the mesh arrays each mesh, we dont want to merge them by error
			indices.clear();
			vertices.clear();

			for (auto&& p : mesh.primitives) {
				GeoSurface newSurface;
				newSurface.startIndex = (uint32_t)indices.size();
				newSurface.count = (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

				size_t initial_vtx = vertices.size();

				// load indexes
				{
					fastgltf::Accessor& indexaccessor = gltf.accessors[p.indicesAccessor.value()];
					indices.reserve(indices.size() + indexaccessor.count);
					fastgltf::iterateAccessor<std::uint32_t>(gltf, indexaccessor,
						[&](std::uint32_t idx) {
							indices.push_back(idx + initial_vtx);
						});
				}

				// load vertex positions
				{
					fastgltf::Accessor& posAccessor = gltf.accessors[p.findAttribute("POSITION")->accessorIndex];
					vertices.resize(vertices.size() + posAccessor.count);

					fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor,
						[&](glm::vec3 v, size_t index) {
							Vertex newvtx;
							newvtx.position = v;
							newvtx.normal = { 1, 0, 0 };
							newvtx.color = glm::vec4{ 1.f };
							newvtx.uv = { 0, 0 };
							vertices[initial_vtx + index] = newvtx;
						});
				}
				// load vertex normals
				auto normals = p.findAttribute("NORMAL");
				if (normals != p.attributes.end()) {

					fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[(*normals).accessorIndex],
						[&](glm::vec3 v, size_t index) {
							vertices[initial_vtx + index].normal = v;
						});
				}

				// load UVs
				auto uv = p.findAttribute("TEXCOORD_0");
				if (uv != p.attributes.end()) {

					fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[(*uv).accessorIndex],
						[&](glm::vec2 v, size_t index) {
							vertices[initial_vtx + index].uv = v;
						});
				}

				// load vertex colors
				auto colors = p.findAttribute("COLOR_0");
				if (colors != p.attributes.end()) {

					fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*colors).accessorIndex],
						[&](glm::vec4 v, size_t index) {
							vertices[initial_vtx + index].color = v;
						});
				}
				newmesh.surfaces.push_back(newSurface);
			}

			// display the vertex normals
			constexpr bool OverrideColors = true;
			if (OverrideColors) {
				for (Vertex& vtx : vertices) {
					vtx.color = glm::vec4(vtx.normal, 1.f);
				}
			}
			newmesh.vertexBuffer = CreateBufferStaged(commandPool, device, queue, allocator, vertices.size() * sizeof(vertices[0]),
				vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress, vertices.data());
			newmesh.indexBuffer = CreateBufferStaged(commandPool, device, queue, allocator, indices.size() * sizeof(indices[0]),
				vk::BufferUsageFlagBits::eIndexBuffer, indices.data());

			meshes.emplace_back(std::make_shared<MeshAsset>(std::move(newmesh)));
		}

		return meshes;
	}

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
		const std::vector<const char*> deviceExtensions = { vk::KHRSwapchainExtensionName, vk::KHRDynamicRenderingExtensionName,
			vk::EXTShaderObjectExtensionName, vk::KHRBufferDeviceAddressExtensionName, vk::EXTDescriptorIndexingExtensionName};
		Logger::logger->Log("Device extensions used: "); for (auto& e : deviceExtensions) Logger::logger->Log(" - " + std::string(e));
		
		vk::PhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.samplerAnisotropy = VK_TRUE;

		vk::PhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures = vk::PhysicalDeviceDescriptorIndexingFeatures();
		vk::PhysicalDeviceBufferDeviceAddressFeatures bufferAddressFeatures = vk::PhysicalDeviceBufferDeviceAddressFeatures(1, {}, {}, &descriptorIndexingFeatures);
		vk::PhysicalDeviceShaderObjectFeaturesEXT shaderObjectFeatures = vk::PhysicalDeviceShaderObjectFeaturesEXT(1, &bufferAddressFeatures);
		vk::PhysicalDeviceDynamicRenderingFeatures dynamicFeatures = vk::PhysicalDeviceDynamicRenderingFeatures(1, &shaderObjectFeatures);
		
		m_Device = m_PhysicalDevice.createDeviceUnique(vk::DeviceCreateInfo(vk::DeviceCreateFlags(), static_cast<uint32_t>(queueCreateInfos.size()),
			queueCreateInfos.data(), 0u, nullptr, static_cast<uint32_t>(deviceExtensions.size()), deviceExtensions.data(), &deviceFeatures, &dynamicFeatures));

		// Queues
		m_DeviceQueue = m_Device->getQueue(static_cast<uint32_t>(graphicsQueueFamilyIndex), 0);
		m_PresentQueue = m_Device->getQueue(static_cast<uint32_t>(presentQueueFamilyIndex), 0);

		// VMA Allocator
		VmaAllocatorCreateInfo allocatorInfo{ VmaAllocatorCreateFlags{} | VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
			m_PhysicalDevice, m_Device.get(), {}, {}, {}, {}, {}, m_Instance.get(), m_Spec.ApiVersion };
		vmaCreateAllocator(&allocatorInfo, &m_Allocator);
				
		// Swapchain
		m_SwapchainImageCount = 2;
		Logger::logger->Log("Using " + std::to_string(m_SwapchainImageCount) + " frames in flight");
		m_SwapchainImageFormat = vk::Format::eB8G8R8A8Unorm;
		m_SwapchainExtent = vk::Extent2D{ m_Spec.Width, m_Spec.Height };
		RecreateSwapchain(); // Also recreates image views, and depth buffer/stencil

		// Descriptor set layout
		vk::DescriptorSetLayoutBinding uboLayoutBinding{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex };
		vk::DescriptorSetLayoutBinding samplerLayoutBinding{ 1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment };
		std::array<vk::DescriptorSetLayoutBinding, 2> bindings{ uboLayoutBinding, samplerLayoutBinding };
		vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{ {}, static_cast<uint32_t>(bindings.size()), bindings.data() };

		m_DescriptorSetLayout = m_Device->createDescriptorSetLayoutUnique(descriptorSetLayoutCreateInfo);

		// Pipeline layout
		vk::PushConstantRange pushConstantRange{ vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstantData) };
		vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{ {}, 1, &m_DescriptorSetLayout.get(), 1, &pushConstantRange };
		m_PipelineLayout = m_Device->createPipelineLayoutUnique(pipelineLayoutCreateInfo);

		// Shaders
		std::vector<char> vertShaderCode = readFile("res/shader/shader.vert.spv");
		std::vector<char> fragShaderCode = readFile("res/shader/shader.frag.spv");
		std::vector<vk::ShaderCreateInfoEXT> shaderInfos{
			{ {}, vk::ShaderStageFlagBits::eVertex, vk::ShaderStageFlagBits::eFragment, vk::ShaderCodeTypeEXT::eSpirv,
			vertShaderCode.size(), vertShaderCode.data(), "main", 1, &m_DescriptorSetLayout.get(), 1, &pushConstantRange },
			{ {}, vk::ShaderStageFlagBits::eFragment, vk::ShaderStageFlagBits{0}, vk::ShaderCodeTypeEXT::eSpirv,
			fragShaderCode.size(), fragShaderCode.data(), "main", 1,&m_DescriptorSetLayout.get(), 1, &pushConstantRange } };
		
		m_Shaders = m_Device->createShadersEXTUnique(shaderInfos, nullptr, m_DLDI).value;

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

		// Texture
		m_TextureImage = CreateImageTexture(m_Allocator, m_CommandPool.get(), m_Device.get(), m_DeviceQueue, "res/texture/texture.jpg",
			vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eSampled);

		// Texture sampler
		vk::SamplerCreateInfo samplerInfo{ {}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear,
			vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, {}, VK_TRUE,
			m_PhysicalDevice.getProperties().limits.maxSamplerAnisotropy, VK_FALSE, vk::CompareOp::eAlways, {}, {}, vk::BorderColor::eIntOpaqueBlack, VK_FALSE };
		m_Sampler = m_Device->createSamplerUnique(samplerInfo);

		// Buffs
		m_VertexBuffer = CreateBufferStaged(m_CommandPool.get(), m_Device.get(), m_DeviceQueue, m_Allocator, sizeof(vertices[0]) * vertices.size(),
			vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer, vertices.data());
		m_IndexBuffer = CreateBufferStaged(m_CommandPool.get(), m_Device.get(), m_DeviceQueue, m_Allocator, sizeof(indices[0]) * indices.size(),
			vk::BufferUsageFlagBits::eIndexBuffer, indices.data());

		vk::DeviceAddress vertexBufferDeviceAddress = m_Device->getBufferAddress({ m_VertexBuffer.Buffer });

		testMeshes = LoadModel(m_CommandPool.get(), m_Device.get(), m_DeviceQueue, m_Allocator, "res/model/basicmesh.glb").value();

		// Uniform Buff
		m_UniformBuffers.resize(m_SwapchainImageCount);
		for (auto& ub : m_UniformBuffers)
			ub = CreateBuffer(m_Allocator, sizeof(UniformBufferObject), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
		
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
			vk::DescriptorBufferInfo bufferInfo{ m_UniformBuffers[i].Buffer, 0, sizeof(UniformBufferObject) };
			vk::DescriptorImageInfo imageInfo{ m_Sampler.get(), m_TextureImage.ImageView.get(), vk::ImageLayout::eShaderReadOnlyOptimal };
			std::vector<vk::WriteDescriptorSet> descriptorWrites{
				vk::WriteDescriptorSet{ m_DescriptorSets[i].get(), 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &bufferInfo },
				vk::WriteDescriptorSet{ m_DescriptorSets[i].get(), 1, 0, 1, vk::DescriptorType::eCombinedImageSampler, &imageInfo }	};

			m_Device->updateDescriptorSets(static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
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

		static_cast<void>(m_Device->waitForFences(1, &m_InFlightFence.get(), VK_TRUE, UINT64_MAX));
		static_cast<void>(m_Device->resetFences(1, &m_InFlightFence.get()));

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
		ubo.view = glm::translate(glm::vec3{ 0,0,-5 });
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj = glm::perspective(glm::radians(70.0f), m_SwapchainExtent.width / (float)m_SwapchainExtent.height, 10000.0f, 0.1f);
		ubo.proj[1][1] *= -1;
		
		void* bufferData;
		vmaMapMemory(m_Allocator, m_UniformBuffers[currentFrame].Allocation, &bufferData);
		memcpy(bufferData, &ubo, sizeof(ubo));
		vmaUnmapMemory(m_Allocator, m_UniformBuffers[currentFrame].Allocation);

		// Submit command buffer
		vk::PipelineStageFlags waitStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		m_DeviceQueue.submit(vk::SubmitInfo{ 1, &m_ImageAvailableSemaphore.get(), &waitStageMask, 1, &m_CommandBuffers[imageIndex.value].get(), 1,
			&m_RenderFinishedSemaphore.get() }, m_InFlightFence.get());

		static_cast<void>(m_PresentQueue.presentKHR({ 1, &m_RenderFinishedSemaphore.get(), 1, &m_Swapchain.get(), &imageIndex.value }));
		currentFrame = (currentFrame + 1) % m_SwapchainImageCount;
	}

	Renderer::~Renderer()	
	{
		m_Device->waitIdle(); // Everything will descope automatically due to unique pointers

		DestroyBuffer(m_Allocator, m_VertexBuffer);
		DestroyBuffer(m_Allocator, m_IndexBuffer);
		for (auto& ub : m_UniformBuffers)
			DestroyBuffer(m_Allocator, ub);

		DestroyImage(m_Allocator, m_TextureImage);
		DestroyImage(m_Allocator, m_DepthImage);

		for (std::shared_ptr<MeshAsset> meshAsset : testMeshes)
		{
			DestroyBuffer(m_Allocator, meshAsset->vertexBuffer);
			DestroyBuffer(m_Allocator, meshAsset->indexBuffer);
		}

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
		m_DepthImage = CreateImage(m_Allocator, m_Device.get(), m_SwapchainExtent.width, m_SwapchainExtent.height, depthFormat, vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eDepthStencilAttachment, VMA_MEMORY_USAGE_GPU_ONLY);
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
		vk::Buffer vertexBuffers[] = { m_VertexBuffer.Buffer };
		vk::DeviceSize offsets[] = { 0 };

		// Background colour
		std::vector<vk::ClearValue> clearValues{ {{ 1.0f, 0.5f, 0.3f, 1.0f }}, {{ 1.0f, 0 }} };

		PushConstantData pushConstants{};
		glm::mat4 view = glm::translate(glm::vec3{ 0,0,-5 });
		glm::mat4 projection = glm::perspective(glm::radians(70.f), (float)width / (float)height, 10000.f, 0.1f);
		projection[1][1] *= -1; // invert the Y direction on projection matrix so that we are more similar to opengl and gltf axis
		pushConstants.worldMatrix = projection * view;
		pushConstants.vertexBuffer = m_Device->getBufferAddress({ testMeshes[2]->vertexBuffer.Buffer });

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
			const vk::RenderingAttachmentInfo depthAttachment{ m_DepthImage.ImageView.get(), vk::ImageLayout::eDepthStencilAttachmentOptimal, {}, {}, {}, // Depth
					vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare, clearValues[1] };

			const vk::RenderingInfo renderingInfo{ {}, vk::Rect2D{ { 0, 0 }, m_SwapchainExtent }, 1, {}, attachments, &depthAttachment };

			// Actual command buffers
			m_CommandBuffers[i]->begin(vk::CommandBufferBeginInfo{});
			m_CommandBuffers[i]->pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput,
				{}, 0, nullptr, 0, nullptr, 1, &topImageMemoryBarrier);

			// Get ready for the motherload of boilerplate from using ShaderEXT's
			std::vector<vk::VertexInputBindingDescription2EXT> bindings{ Vertex::getBindingDescription() };
			m_CommandBuffers[i]->setVertexInputEXT(static_cast<uint32_t>(bindings.size()), bindings.data(),
				static_cast<uint32_t>(Vertex::getAttributeDescriptions().size()), Vertex::getAttributeDescriptions().data(), m_DLDI);
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
			m_CommandBuffers[i]->setColorBlendEnableEXT(0, { 1/*vk::BlendFactor::eOne*/, 0/*vk::BlendFactor::eZero*/, 1/*vk::BlendOp::eAdd*/ }, m_DLDI);
			m_CommandBuffers[i]->setColorBlendEquationEXT(0, { { vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd } }, m_DLDI);
			m_CommandBuffers[i]->setColorWriteMaskEXT(0, vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB
				| vk::ColorComponentFlagBits::eA, m_DLDI);
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
			m_CommandBuffers[i]->bindIndexBuffer(testMeshes[2]->indexBuffer.Buffer, 0, vk::IndexType::eUint32);

			m_CommandBuffers[i]->setViewport(0, 1, &viewport);
			m_CommandBuffers[i]->setScissor(0, 1, &scissor);

			m_CommandBuffers[i]->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_PipelineLayout, 0, 1, &m_DescriptorSets[i].get(), 0, nullptr);
			m_CommandBuffers[i]->pushConstants(*m_PipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstantData), &pushConstants);
			m_CommandBuffers[i]->drawIndexed(testMeshes[2]->surfaces[0].count, 1, testMeshes[2]->surfaces[0].startIndex, 0, 0);

			m_CommandBuffers[i]->endRendering();

			m_CommandBuffers[i]->pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe,
				{}, 0, nullptr, 0, nullptr, 1, &bottomImageMemoryBarrier);
			m_CommandBuffers[i]->end();
		}
	}
}