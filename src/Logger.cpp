#include "Logger.h"

#include <iomanip>
#include <chrono>

namespace hyper
{
	Logger* Logger::logger; // Do not re-declare loggers, it is in big boi space, scary

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
	{
		Severity severity = Severity::Info; // You can't switch statement a bitwise check :(
		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
			severity = Severity::Verbose;
		}
		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
			severity = Severity::Info;
		}
		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
			severity = Severity::Warning;
		}
		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
			severity = Severity::Error;
		}
		Logger::logger->Log("validation layer: " + std::string(pCallbackData->pMessage), severity);
		return VK_FALSE;
	}

	std::string Logger::getCurrentTimestamp() const
	{ // This stuff is re-allocated a lot, I wonder if I should make some static or smth
		auto now = std::chrono::system_clock::now(); // TBH best case scenario the logger shouldn't be called often ;^)
		std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
		struct tm* timeInfo = localtime(&now_time_t);
		auto duration = now.time_since_epoch();
		auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;
		std::ostringstream oss;
		oss << std::put_time(timeInfo, "%Y-%m-%d %H:%M:%S") << "." << std::setfill('0') << std::setw(3) << milliseconds;
		return oss.str();
	}

	void Logger::Log(std::string message, Severity severity) const
	{
		if (m_Debug) // There was a semicolon hidden here, tripped me up for like 10min trying to figure out why my debug statements were running when debug was off
		{
			// Timestamp and severity
			switch (severity)
			{
			case Severity::Setup:
				std::cout << "\033[36;40m[ " << getCurrentTimestamp() << " ] [ SET. ] " << "\033[0m"; // Cyan
				break;
			case Severity::Verbose:
				std::cout << "\033[90;40m[ " << getCurrentTimestamp() << " ] [ VRB. ] " << "\033[0m"; // Blue
				break;
			case Severity::Info:
				if (Logger::logger->IsInfoDebug())
					std::cout << "\033[32;40m[ " << getCurrentTimestamp() << " ] [ INFO ] " << "\033[0m"; // Green
				break;
			case Severity::Warning:
				std::cout << "\033[33;40m[ " << getCurrentTimestamp() << " ] [ WARN ] " << "\033[0m"; // Yellow
				break;
			case Severity::Error:
				std::cerr << "\033[31;40m[ " << getCurrentTimestamp() << " ] [ ERR. ] " << "\033[0m"; // Red
				break;
			}
			// Message
			std::cout << message << std::endl; // Much wow
		}
	}

	vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> Logger::MakeDebugMessenger(vk::UniqueInstance& instance, vk::DispatchLoaderDynamic& dldi)
	{ // Will only run in the renderer with debug on so no need to check for debug here
		return instance->createDebugUtilsMessengerEXTUnique(
			vk::DebugUtilsMessengerCreateInfoEXT{ {}, vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
					vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo, // Info is annoying
				vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
			vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance, debugCallback }, nullptr, dldi);
	}

}