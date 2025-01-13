#pragma once
#define _CRT_SECURE_NO_WARNINGS // Me when MSVC-only problem that only affects vulkan versions
#include <vulkan/vulkan.hpp>	// >1.3.275 and I didn't originally notice because I was ON 1.3.275
#undef _CRT_SECURE_NO_WARNINGS	// and only then noticed when I had to reinstall the SDK :)))))

#include "Include.h"

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
		bool IsDebug() { return m_Debug; }
		bool IsInfoDebug() { return m_InfoDebug; }

		void Log(std::string message, Severity severity = Severity::Setup) const;

		// Thank you amengede on github for this bit
		vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> MakeDebugMessenger(vk::UniqueInstance& instance, vk::DispatchLoaderDynamic& dldi);
	
	private:
		bool m_Debug = false;
		bool m_InfoDebug = false;
	};
}