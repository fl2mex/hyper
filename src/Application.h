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

		void Run();

	private:
		Spec m_Spec; // Custom Spec struct, check Spec.h for options

		GLFWwindow* m_Window{ nullptr };

		vk::Instance m_Instance{ nullptr };
	};
}