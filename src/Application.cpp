#include "Application.h"

namespace hyper
{
	VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)// Not a fan of C-style
	{																															// functions in vulkan-hpp :(
		std::cerr << "Validation Layer: " << pCallbackData->pMessage << std::endl;
		return false;
	}

	void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)	// GLFW-specific key callback function
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) { glfwSetWindowShouldClose(window, GLFW_TRUE); } // Just closing for now :)
	}

	Application::Application(Spec spec)
	{
		m_Spec = spec; // Copy

		m_Window = CreateWindow(m_Spec);
		log("GLFW: Window Created");

		m_Instance = CreateInstance(m_Spec);
		log("Vulkan: Instance Created");
		
		if (m_Spec.Debug)
		{
			m_DLDI = CreateDLDI(m_Instance);
			m_DebugMessenger = CreateDebugMessenger(m_Instance, m_DLDI);
			log("Vulkan: DLDI & Debug Messenger Created");
		}

		m_Surface = CreateSurface(m_Instance, m_Window);
		log("GLFW: Surface Created");

		m_PhysicalDevice = ChoosePhysicalDevice(m_Instance);
		log("Vulkan: Using Physical Device: \n\t" << m_PhysicalDevice.getProperties().deviceName);

		m_QueueFamilyIndices = FindQueueFamilies(m_PhysicalDevice, m_Surface);
		log("Vulkan: Chose Queue Families: \n\t" << "Graphics: " << m_QueueFamilyIndices[0] << "\n\tPresent: " << m_QueueFamilyIndices[1]);

		m_LogicalDevice = CreateLogicalDevice(m_QueueFamilyIndices, m_Surface, m_PhysicalDevice, m_Spec);

		m_GraphicsQueue = GetQueue(m_LogicalDevice, m_QueueFamilyIndices[0]);
		m_PresentQueue = GetQueue(m_LogicalDevice, m_QueueFamilyIndices[1]);
		log("Vulkan: Queues Set");

		m_SwapchainFormat = ChooseSwapchainFormat(m_PhysicalDevice, m_Surface);
		m_SwapchainExtent = ChooseSwapchainExtent(m_PhysicalDevice, m_Surface, m_Spec);
		log("Vulkan: Chose Swapchain Format and Extent");

		m_Swapchain = CreateSwapchain(m_QueueFamilyIndices, m_SwapchainFormat, m_SwapchainExtent, m_PhysicalDevice, m_Surface, m_Spec, m_LogicalDevice);
		log("Vulkan: Swapchain Created");

		m_SwapchainImages = m_LogicalDevice.getSwapchainImagesKHR(m_Swapchain);
		m_SwapchainImageViews = CreateSwapchainImageViews(m_LogicalDevice, m_SwapchainImages, m_SwapchainFormat);
		log("Vulkan: Swapchain Image and Image Views Created");

		Run();
	}

	Application::~Application() // Destroy objects in the opposite order they were created in
	{
		for (const vk::ImageView& imageView : m_SwapchainImageViews) { m_LogicalDevice.destroyImageView(imageView); }

		m_LogicalDevice.destroySwapchainKHR(m_Swapchain);

		m_LogicalDevice.destroy();

		if (m_Spec.Debug) { m_Instance.destroyDebugUtilsMessengerEXT(m_DebugMessenger, nullptr, m_DLDI); }

		m_Instance.destroySurfaceKHR(m_Surface);

		m_Instance.destroy();

		glfwDestroyWindow(m_Window);
		glfwTerminate();
	}

	GLFWwindow* Application::CreateWindow(const Spec& spec) const
	{
		if (!glfwInit())
		{
			log("GLFW: Couldn't Init!");
			throw std::runtime_error("GLFW: Couldn't Init!");
		}

		if (!glfwVulkanSupported())
		{
			log("GLFW: Vulkan not supported!");
			throw std::runtime_error("GLFW: Vulkan not supported!");
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		
		GLFWwindow* window = glfwCreateWindow(spec.Width, spec.Height, spec.Name.c_str(), nullptr, nullptr);
		if (!window)
		{
			glfwTerminate();
			log("GLFW: Couldn't Create Window!");
			throw std::runtime_error("GLFW: Couldn't Create Window!");
		}
		glfwSetKeyCallback(window, KeyCallback);

		return window;
	}

	vk::Instance Application::CreateInstance(const Spec& spec) const
	{
		uint32_t glfwExtensionCount = 0; // C arcane magic
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		uint32_t version = spec.ApiVersion;
		const vk::ApplicationInfo appInfo(spec.Name.c_str(), version, "", version, version);

		log("Vulkan: Version Found: " << VK_API_VERSION_MAJOR(vk::enumerateInstanceVersion()) << "."<< VK_API_VERSION_MINOR(vk::enumerateInstanceVersion())
			<< "." << VK_API_VERSION_PATCH(vk::enumerateInstanceVersion()) << ", Using: " << VK_API_VERSION_MAJOR(version) << "." << VK_API_VERSION_MINOR(version)
			<< "." << VK_API_VERSION_PATCH(version));

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount), layers;

		if (spec.Debug)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); // Adding validation and debug stuff to each vector to be used in instance
			layers.push_back("VK_LAYER_KHRONOS_validation"); // Unfortunately no macro that I can find for layers :(
		}

		std::vector<vk::ExtensionProperties> supportedExtensions = vk::enumerateInstanceExtensionProperties();
		std::vector<vk::LayerProperties> supportedLayers = vk::enumerateInstanceLayerProperties();	
		log("Vulkan: Supported Extensions:");
		for (const vk::ExtensionProperties& supportedExtension : supportedExtensions) { log("\t" << supportedExtension.extensionName); }
		log("Vulkan: Supported Layers:");
		for (const vk::LayerProperties& supportedLayer : supportedLayers) { log("\t" << supportedLayer.layerName); }
		log("Vulkan: Using Extensions:");
		for (const char* extension : extensions) { log("\t" << extension); }
		log("Vulkan: Using Layers:");
		for (const char* layer : layers) { log("\t" << layer); }

		const vk::InstanceCreateInfo instanceInfo(vk::InstanceCreateFlags(), &appInfo,
			static_cast<uint32_t>(layers.size()), layers.data(),
			static_cast<uint32_t>(extensions.size()), extensions.data());

		try
		{
			return vk::createInstance(instanceInfo);
		}
		catch (vk::SystemError err)
		{
			log("Vulkan: Couldn't Create Instance!");
			throw std::runtime_error("Vulkan: Couldn't Create Instance!");
		}
	}

	vk::DispatchLoaderDynamic Application::CreateDLDI(const vk::Instance& instance) const
	{
		try
		{
			return vk::DispatchLoaderDynamic(instance, vkGetInstanceProcAddr);
		}
		catch (vk::SystemError err)
		{
			log("Vulkan: Couldn't Create DLDI!");
			throw std::runtime_error("Vulkan: Couldn't Create DLDI!");
		}
	}
	
	vk::DebugUtilsMessengerEXT Application::CreateDebugMessenger(const vk::Instance& instance, const vk::DispatchLoaderDynamic& dldi) const
	{
		vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerInfo = vk::DebugUtilsMessengerCreateInfoEXT(vk::DebugUtilsMessengerCreateFlagsEXT(),
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError, // eVerbose is annoying, add back if needed
			vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
			DebugCallback, nullptr);

		try
		{
			return instance.createDebugUtilsMessengerEXT(debugUtilsMessengerInfo, nullptr, dldi);
		}
		catch (vk::SystemError err)
		{
			log("Vulkan: Couldn't Create Debug Messenger!");
			throw std::runtime_error("Vulkan: Couldn't Create Debug Messenger!");
		}
	}

	vk::SurfaceKHR Application::CreateSurface(const vk::Instance& instance, GLFWwindow* window) const
	{
		VkSurfaceKHR surface; // Must use C-style convention before casting via return
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
		{
			log("GLFW: Couldn't create surface!");
			throw std::runtime_error("GLFW: Couldn't create surface!");
		}
		return surface; // Could use GLFWPP or any other wrapper to probably fix this
		// But why add a new dependency instead of just dealing with C twice?
	}

	vk::PhysicalDevice Application::ChoosePhysicalDevice(const vk::Instance& instance) const
	{
		std::vector<vk::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();

		if (physicalDevices.size() == 0)
		{
			log("Vulkan: Couldn't find any GPUs with Vulkan support!");
			throw std::runtime_error("Vulkan: Couldn't find any GPUs with Vulkan support!");
		}

		log("Vulkan: Available Physical Devices:");
		for (const vk::PhysicalDevice& physicalDevice : physicalDevices) { log("\t" << physicalDevice.getProperties().deviceName); }

		// Not good implementation if there are multiple GPUs of different speeds, just picks the first one in the list
		// But... If you install your GPUs correctly, the best GPU should be connected to the PCIE slot nearest to the CPU haha
		int gpuIndex = 0;
		for (int i = 0; i < (int)physicalDevices.size(); i++)
		{
			vk::PhysicalDeviceProperties properties = physicalDevices[i].getProperties();
			if (properties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu)
			{
				gpuIndex = i;
				break;
			}
		}
		// Check twice, if a discrete & integrated GPU are found, pick discrete by running loop again
		// Inefficient? Yes. Ingenious? Yes. NVIDIA, hire me immediately.
		for (int i = 0; i < (int)physicalDevices.size(); i++)
		{
			vk::PhysicalDeviceProperties properties = physicalDevices[i].getProperties();
			if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
			{
				gpuIndex = i;
				break;
			}
		}
		return physicalDevices[gpuIndex];
	}

	std::vector<uint32_t> Application::FindQueueFamilies(const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface) const
	{
		std::vector<vk::QueueFamilyProperties> queueFamilies = physicalDevice.getQueueFamilyProperties();
		uint32_t graphicsFamily = -1, presentFamily = -1, queueFamilyIndex = 0;
		for (const vk::QueueFamilyProperties& queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
			{
				graphicsFamily = queueFamilyIndex;
			}

			if (physicalDevice.getSurfaceSupportKHR(queueFamilyIndex, surface) && queueFamilyIndex != graphicsFamily)
			{
				presentFamily = queueFamilyIndex;
			}

			if (graphicsFamily != -1 && presentFamily != -1)
			{
				break;
			}

			queueFamilyIndex++;
		}
		return std::vector<uint32_t>{ graphicsFamily, presentFamily };
	}

	vk::Device Application::CreateLogicalDevice(std::vector<uint32_t> queueFamilyIndices, const vk::SurfaceKHR& surface,
		const vk::PhysicalDevice& physicalDevice, const Spec& spec) const
	{
		std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
		float queuePriority = 0.0f; // Confusing
		for (uint32_t& queueFamilyIndex : queueFamilyIndices)
		{
			queueCreateInfos.push_back(vk::DeviceQueueCreateInfo{ vk::DeviceQueueCreateFlags(),
				static_cast<uint32_t>(queueFamilyIndex), 1, &queuePriority });
		}

		std::vector<const char*> logicalDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME }, logicalDeviceLayers;
		if (spec.Debug) { logicalDeviceLayers.push_back("VK_LAYER_KHRONOS_validation"); } // Added to device for compatability

		log("Vulkan: Using Device Extensions:");
		for (const char* extension : logicalDeviceExtensions) { log("\t" << extension); }
		log("Vulkan: Using Device Layers:");
		for (const char* layer : logicalDeviceLayers) { log("\t" << layer); }

		vk::PhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeatures(true);
		
		vk::DeviceCreateInfo logicalDeviceInfo(vk::DeviceCreateFlags(), static_cast<uint32_t>(queueCreateInfos.size()), queueCreateInfos.data(),
			static_cast<uint32_t>(logicalDeviceLayers.size()), logicalDeviceLayers.data(),
			static_cast<uint32_t>(logicalDeviceExtensions.size()), logicalDeviceExtensions.data(), 0u, &dynamicRenderingFeatures);

		try
		{
			return physicalDevice.createDevice(logicalDeviceInfo);
		}
		catch (vk::SystemError err)
		{
			log("Vulkan: Couldn't Create Logical Device!");
			throw std::runtime_error("Vulkan: Couldn't Create Logical Device!");
		}
	}

	vk::Queue Application::GetQueue(vk::Device logicalDevice, uint32_t queueIndex) const
	{
		try
		{
			return logicalDevice.getQueue(queueIndex, 0); // As simple as that
		}
		catch (vk::SystemError err)
		{
			log("Vulkan: Couldn't Get Queue!");
			throw std::runtime_error("Vulkan: Couldn't Get Queue!");
		}
	}

	vk::Format Application::ChooseSwapchainFormat(const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface) const
	{
		std::vector<vk::SurfaceFormatKHR> formats = physicalDevice.getSurfaceFormatsKHR(surface);
		for (const vk::SurfaceFormatKHR& format : formats)
		{
			if (format.format == vk::Format::eB8G8R8A8Unorm && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) { return format.format; }
		}

		return formats[0].format;
	}

	vk::Extent2D Application::ChooseSwapchainExtent(const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface, const Spec& spec) const
	{
		vk::SurfaceCapabilitiesKHR capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
		if (capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max()) // Yoda was right all along
		{
			return
			{
				std::min(capabilities.maxImageExtent.width, std::max(capabilities.minImageExtent.width, spec.Width)),
				std::min(capabilities.maxImageExtent.height, std::max(capabilities.minImageExtent.height, spec.Height))
			};
		}

		return capabilities.currentExtent;
	}

	vk::SwapchainKHR Application::CreateSwapchain(std::vector<uint32_t> queueFamilyIndices, const vk::Format& format, const vk::Extent2D& extent,
		const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface, const Spec& spec, const vk::Device& logicalDevice) const
	{
		uint32_t imageCount = 2; // Double buffer

		
		vk::SharingMode sharingMode = vk::SharingMode::eExclusive;
		uint32_t familyIndexCount = 0;
		uint32_t* familyIndices{};

		if (queueFamilyIndices[0] != queueFamilyIndices[1])
		{
			sharingMode = vk::SharingMode::eConcurrent;
			familyIndexCount = 2u;
			familyIndices = &queueFamilyIndices[0];
		}					
		
		std::vector<vk::PresentModeKHR> presentModes = physicalDevice.getSurfacePresentModesKHR(surface);
		vk::PresentModeKHR usedPresentMode = vk::PresentModeKHR::eFifo;
		for (const vk::PresentModeKHR& presentMode : presentModes)
		{
			if (presentMode == vk::PresentModeKHR::eMailbox) { usedPresentMode = presentMode; }
		}

		vk::SwapchainCreateInfoKHR swapChainCreateInfo(vk::SwapchainCreateFlagsKHR(), surface, imageCount, format, vk::ColorSpaceKHR::eSrgbNonlinear,
			extent, 1, vk::ImageUsageFlagBits::eColorAttachment, sharingMode, familyIndexCount, familyIndices,
			vk::SurfaceTransformFlagBitsKHR::eIdentity, vk::CompositeAlphaFlagBitsKHR::eOpaque, usedPresentMode, true, vk::SwapchainKHR(nullptr));

		try
		{
			return logicalDevice.createSwapchainKHR(swapChainCreateInfo);
		}
		catch (vk::SystemError err)
		{
			log("Vulkan: Couldn't Create Swapchain!");
			throw std::runtime_error("Vulkan: Couldn't Create Swapchain!");
		}
	}
	
	std::vector<vk::ImageView> Application::CreateSwapchainImageViews(const vk::Device& logicalDevice, std::vector<vk::Image> swapchainImages,
		const vk::Format& format) const
	{
		std::vector<vk::ImageView> imageViews;
		for (const vk::Image& image : swapchainImages)
		{
			vk::ImageViewCreateInfo imageViewCreateInfo(vk::ImageViewCreateFlags(), image,
				vk::ImageViewType::e2D, format,
				vk::ComponentMapping{ vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB,vk::ComponentSwizzle::eA },
				vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });
			imageViews.push_back(logicalDevice.createImageView(imageViewCreateInfo));
		}

		return imageViews;
	}

	void Application::Run()
	{
		while (!glfwWindowShouldClose(m_Window)) // Main Loop
		{
			glfwPollEvents();
		}
	}
}