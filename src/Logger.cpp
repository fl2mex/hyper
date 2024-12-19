#include "Logger.h"

namespace hyper
{
	Logger* Logger::logger; // Do not re-declare loggers, it is in big boi space, scary

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
	{
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
		return VK_FALSE;
	}

	void Logger::Log(std::string message) const
	{
		if (m_Debug) // There was a semicolon hidden here, tripped me up for like 10min trying to figure out why my debug statements were running when debug was off
			std::cout << message << std::endl; // Much wow
	}

	vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> Logger::MakeDebugMessenger(vk::UniqueInstance& instance, vk::DispatchLoaderDynamic& dldi)
	{ // Will only run in the renderer with debug on so no need to check for debug here
		return instance->createDebugUtilsMessengerEXTUnique(
			vk::DebugUtilsMessengerCreateInfoEXT{ {}, vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
					vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose /* | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo */, // Info is annoying
				vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
			vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance, debugCallback }, nullptr, dldi);
	}

}