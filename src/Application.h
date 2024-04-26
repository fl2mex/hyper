#pragma once
#include "Include.h"
#include "Spec.h"

namespace hyper
{
	class Application
	{
	public:
		Application(const Spec& spec);
		~Application();

	private:
		GLFWwindow* CreateWindow(const Spec& spec) const;
		
		vk::UniqueInstance CreateInstance(const Spec& spec) const;

		vk::DispatchLoaderDynamic CreateDLDI(vk::UniqueInstance& instance) const;
		vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> CreateDebugMessenger(vk::UniqueInstance& instance, const vk::DispatchLoaderDynamic& dldi) const;
		vk::UniqueSurfaceKHR CreateSurface(vk::UniqueInstance& instance, GLFWwindow* window) const;

		vk::PhysicalDevice ChoosePhysicalDevice(vk::UniqueInstance& instance) const;
		std::vector<uint32_t> FindQueueFamilies(vk::PhysicalDevice physicalDevice, vk::UniqueSurfaceKHR& surface) const;
		vk::UniqueDevice CreateLogicalDevice(std::vector<uint32_t> queueFamilyIndices, vk::UniqueSurfaceKHR& surface, vk::PhysicalDevice, const Spec& spec) const;

		vk::Queue GetQueue(vk::UniqueDevice& logicalDevice, uint32_t queueIndex) const;

		vk::Format ChooseSwapchainFormat(vk::PhysicalDevice physicalDevice, vk::UniqueSurfaceKHR& surface) const;
		vk::Extent2D ChooseSwapchainExtent(vk::PhysicalDevice physicalDevice, vk::UniqueSurfaceKHR& surface, const Spec& spec) const;
		vk::UniqueSwapchainKHR CreateSwapchain(std::vector<uint32_t> queueFamilyIndices, vk::Format format, vk::Extent2D extent, vk::PhysicalDevice physicalDevice,
			vk::UniqueSurfaceKHR& surface, const Spec& spec, vk::UniqueDevice& logicalDevice) const;
		std::vector<vk::UniqueImageView> CreateSwapchainImageViews(vk::UniqueDevice& logicalDevice, std::vector<vk::Image> swapchainImages, vk::Format format) const;

		void Run();

	private:
		Spec m_Spec; // Custom Spec struct, check Spec.h for options

		GLFWwindow* m_Window{ nullptr };

		vk::UniqueInstance m_Instance{ nullptr };

		vk::DispatchLoaderDynamic m_DLDI;
		vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> m_DebugMessenger{ nullptr };

		vk::UniqueSurfaceKHR m_Surface{ nullptr };

		vk::PhysicalDevice m_PhysicalDevice{ nullptr };
		std::vector<uint32_t> m_QueueFamilyIndices; // Graphics, Present - would a struct be nicer?
		vk::UniqueDevice m_LogicalDevice{ nullptr };

		vk::Queue m_GraphicsQueue{ nullptr };
		vk::Queue m_PresentQueue{ nullptr };

		vk::Extent2D m_SwapchainExtent; // Should move this stuff into a struct
		vk::Format m_SwapchainFormat;
		vk::UniqueSwapchainKHR m_Swapchain{ nullptr };
		std::vector<vk::Image> m_SwapchainImages;
		std::vector<vk::UniqueImageView> m_SwapchainImageViews;
	};
}