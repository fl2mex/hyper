#include "Application.h"
namespace hyper
{
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

		m_LogicalDevice = CreateLogicalDevice(m_QueueFamilyIndices, m_Surface, m_PhysicalDevice);

		Run();
	}

	Application::~Application()
	{
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

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		GLFWwindow* window = glfwCreateWindow(spec.Width, spec.Height, spec.Name.c_str(), nullptr, nullptr);

		if (!window)
		{
			glfwTerminate();
			log("GLFW: Couldn't Create Window!");
			throw std::runtime_error("GLFW: Couldn't Create Window!");
		}

		return window;
	}

	vk::Instance Application::CreateInstance(const Spec& spec) const
	{
		if (!glfwVulkanSupported())
		{
			log("GLFW: Vulkan not supported!");
			throw std::runtime_error("GLFW: Vulkan not supported!");
		}

		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		uint32_t version = spec.ApiVersion;
		const vk::ApplicationInfo appInfo(spec.Name.c_str(), version, "", version, version);

		log("Vulkan: Version Found: " << VK_API_VERSION_MAJOR(vk::enumerateInstanceVersion()) << "."<< VK_API_VERSION_MINOR(vk::enumerateInstanceVersion())
			<< "." << VK_API_VERSION_PATCH(vk::enumerateInstanceVersion()) << ", Using: " << VK_API_VERSION_MAJOR(version) << "." << VK_API_VERSION_MINOR(version)
			<< "." << VK_API_VERSION_PATCH(version));

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount), layers;

		if (spec.Debug)
		{
			extensions.push_back("VK_EXT_debug_utils"); // Adding validation and debug stuff to each vector to be used in instance
			layers.push_back("VK_LAYER_KHRONOS_validation");
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

	VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)// Not a fan of the C-style func
	{																															// in vulkan-hpp :(
		std::cerr << "Validation Layer: " << pCallbackData->pMessage << std::endl;
		return false;
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
		return surface; // Could use GLFWPP or any other wrapper to probably fix this but nahhhhhhhhhhh
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

		int gpuIndex = 0; // Not good implementation if there are multiple GPUs of different speeds, just picks the last one in the list
		for (int i = 0; i < (int)physicalDevices.size(); i++)
		{
			vk::PhysicalDeviceProperties properties = physicalDevices[i].getProperties();
			if (properties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu)
			{
				gpuIndex = i;
				break;
			}
		}
		for (int i = 0; i < (int)physicalDevices.size(); i++) // Check twice, if a discrete & integrated GPU are found, pick discrete by running loop again
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

	vk::Device Application::CreateLogicalDevice(std::vector<uint32_t> queueFamilyIndices, const vk::SurfaceKHR& surface, const vk::PhysicalDevice& physicalDevice) const
	{
		std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
		float queuePriority = 0.0f;
		for (uint32_t& queueFamilyIndex : queueFamilyIndices)
		{
			queueCreateInfos.push_back(vk::DeviceQueueCreateInfo{ vk::DeviceQueueCreateFlags(),
				static_cast<uint32_t>(queueFamilyIndex), 1, &queuePriority });
		}

		const std::vector<const char*> logicalDeviceExtensions = { "VK_KHR_swapchain" };

		vk::DeviceCreateInfo logicalDeviceInfo(vk::DeviceCreateFlags(), static_cast<uint32_t>(queueCreateInfos.size()), queueCreateInfos.data(),
			0u, nullptr, static_cast<uint32_t>(logicalDeviceExtensions.size()), logicalDeviceExtensions.data());

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

	void Application::Run()
	{
		while (!glfwWindowShouldClose(m_Window)) // Main Loop
		{
			glfwPollEvents();
		}
	}
}