#pragma once
#include <vulkan/vulkan.hpp>

#include "Spec.h"

namespace hyper
{
	enum Severity
	{
		Setup = 4, // Enums start at the first value :D idk why I don't just set this to -1
		Verbose = 0,
		Info = 1,
		Warning = 2,
		Error = 3
	};

	class Logger
	{
	public:
		static Logger* logger; // This lets it sit globally without having to get the logger in each scope
		Logger() { logger = this; }

		std::string getCurrentTimestamp() const;

		void SetDebug(Spec spec = {}) { m_Debug = spec.Debug; m_InfoDebug = spec.InfoDebug; }
		bool IsDebug() const { return m_Debug; }
		bool IsInfoDebug() const { return m_InfoDebug; }

		void Log(std::string message, Severity severity = Severity::Setup) const;

		// Probably doesn't need to be /here/
		vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::detail::DispatchLoaderDynamic> MakeDebugMessenger(vk::UniqueInstance& instance,
			vk::detail::DispatchLoaderDynamic& dldi);
	private:
		bool m_Debug = false;
		bool m_InfoDebug = false;
	};
}