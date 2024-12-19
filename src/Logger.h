#pragma once
#include "Include.h"

namespace hyper
{
	class Logger
	{
	public:
		static Logger* logger;

		Logger() { logger = this; } // This fixes so many problems

		static Logger* GetLogger() { return (logger) ? logger : new Logger(); };

		void SetDebug(Spec spec) { m_Debug = spec.Debug; }
		bool IsDebug() const { return m_Debug; }

		void Log(std::string message) const;
		// Thank you amengede on github for this bit
		vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> MakeDebugMessenger(vk::UniqueInstance& instance, vk::DispatchLoaderDynamic& dldi);
	
	private:
		bool m_Debug = false;
	};
}