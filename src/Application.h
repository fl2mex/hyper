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
		
		vk::Instance CreateInstance(const Spec& spec) const;

		vk::DispatchLoaderDynamic CreateDLDI(vk::Instance instance) const;
		vk::DebugUtilsMessengerEXT CreateDebugMessenger(vk::Instance instance, const vk::DispatchLoaderDynamic& dldi) const;

		vk::SurfaceKHR CreateSurface(vk::Instance instance, GLFWwindow* window) const;

		vk::PhysicalDevice ChoosePhysicalDevice(vk::Instance instance) const;
		std::vector<uint32_t> FindQueueFamilies(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface) const;
		vk::Device CreateLogicalDevice(std::vector<uint32_t> queueFamilyIndices, vk::SurfaceKHR surface, vk::PhysicalDevice, const Spec& spec) const;

		vk::Queue GetQueue(vk::Device logicalDevice, uint32_t queueIndex) const;

		vk::Format ChooseSwapchainFormat(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface) const;
		vk::Extent2D ChooseSwapchainExtent(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface, const Spec& spec) const;
		vk::SwapchainKHR CreateSwapchain(std::vector<uint32_t> queueFamilyIndices, vk::Format format, vk::Extent2D extent, vk::PhysicalDevice physicalDevice,
			vk::SurfaceKHR surface, const Spec& spec, vk::Device logicalDevice) const;
		std::vector<vk::ImageView> CreateSwapchainImageViews(vk::Device logicalDevice, std::vector<vk::Image> swapchainImages, vk::Format format) const;

		void Run();

	private:
		Spec m_Spec; // Custom Spec struct, check Spec.h for options

		GLFWwindow* m_Window{ nullptr };

		vk::Instance m_Instance{ nullptr };

		vk::DebugUtilsMessengerEXT m_DebugMessenger{ nullptr };
		vk::DispatchLoaderDynamic m_DLDI;

		vk::SurfaceKHR m_Surface{ nullptr };

		vk::PhysicalDevice m_PhysicalDevice{ nullptr };
		std::vector<uint32_t> m_QueueFamilyIndices; // Graphics, Present - would a struct be nicer?
		vk::Device m_LogicalDevice{ nullptr };

		vk::Queue m_GraphicsQueue{ nullptr };
		vk::Queue m_PresentQueue{ nullptr };

		vk::Extent2D m_SwapchainExtent;
		vk::Format m_SwapchainFormat;
		vk::SwapchainKHR m_Swapchain{ nullptr };
		std::vector<vk::Image> m_SwapchainImages{ nullptr };
		std::vector<vk::ImageView> m_SwapchainImageViews;

	};
}