#pragma once
#include <vulkan/vulkan.hpp>

#include "Spec.h"

namespace hyper
{
	enum Severity
	{
		Setup = 4,
		Verbose = 0,
		Info = 1,
		Warning = 2,
		Error = 3
	};

	class Logger
	{
	public:
		static Logger* logger;
		Logger() { logger = this; }

		std::string getCurrentTimestamp() const;

		void SetDebug(Spec spec = {}) { m_Debug = spec.Debug; m_InfoDebug = spec.InfoDebug; }
		bool IsDebug() const { return m_Debug; }
		bool IsInfoDebug() const { return m_InfoDebug; }

		void Log(std::string message, Severity severity = Severity::Setup) const;

		vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::detail::DispatchLoaderDynamic> MakeDebugMessenger(vk::UniqueInstance& instance,
			vk::detail::DispatchLoaderDynamic& dldi);
	private:
		bool m_Debug = false;
		bool m_InfoDebug = false;
	};
}