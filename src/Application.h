#pragma once
#include "Include.h"
#include "Spec.h"

namespace hyper
{
	class Application
	{
	public:
		Application(Spec spec);
		~Application();

	private:
		GLFWwindow* CreateWindow(const Spec& spec) const;
		
		vk::Instance CreateInstance(const Spec& spec) const;

		vk::DispatchLoaderDynamic CreateDLDI(const vk::Instance& instance) const;
		vk::DebugUtilsMessengerEXT CreateDebugMessenger(const vk::Instance& instance, const vk::DispatchLoaderDynamic& dldi) const;

		vk::SurfaceKHR CreateSurface(const vk::Instance& instance, GLFWwindow* window) const;

		vk::PhysicalDevice ChoosePhysicalDevice(const vk::Instance& instance) const;

		std::vector<uint32_t> FindQueueFamilies(const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface) const;

		vk::Device CreateLogicalDevice(std::vector<uint32_t> queueFamilyIndices, const vk::SurfaceKHR& surface, const vk::PhysicalDevice&,
			const Spec& spec) const;

		vk::Queue GetQueue(vk::Device logicalDevice, uint32_t queueIndex) const;

		vk::Format ChooseSwapchainFormat(const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface) const;
		vk::Extent2D ChooseSwapchainExtent(const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface, const Spec& spec) const;
		vk::SwapchainKHR CreateSwapchain(std::vector<uint32_t> queueFamilyIndices, const vk::Format& format, const vk::Extent2D& extent,
			const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface, const Spec& spec, const vk::Device& logicalDevice) const;
		std::vector<vk::ImageView> CreateSwapchainImageViews(const vk::Device& logicalDevice, std::vector<vk::Image> swapchainImages,
			const vk::Format& format) const;

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