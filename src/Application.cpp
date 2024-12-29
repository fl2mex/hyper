#include "Application.h"

namespace hyper
{
	static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		}
	}

	static void framebuffer_resize_callback(GLFWwindow* window, int width, int height)
	{
		reinterpret_cast<Application*>(glfwGetWindowUserPointer(window))->GetRenderer().m_FramebufferResized = true;
	}

	Application::Application(Spec _spec = {})
		: m_Spec(_spec)
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // GLFW doesn't need to set this for vulkan
		m_Window = glfwCreateWindow(m_Spec.Width, m_Spec.Height, m_Spec.Title.c_str(), nullptr, nullptr);
		glfwSetKeyCallback(m_Window, key_callback);
		glfwSetWindowUserPointer(m_Window, this);
		glfwSetFramebufferSizeCallback(m_Window, framebuffer_resize_callback);

		m_Renderer.SetupRenderer(m_Spec, m_Window); // Can't use constructors because it needs to be in order
	}

	void Application::Run()
	{
		while (!glfwWindowShouldClose(m_Window))
		{
			glfwPollEvents();
			m_Renderer.DrawFrame();
		}
	}

	Application::~Application()
	{
		glfwDestroyWindow(m_Window);
		glfwTerminate();
	}
}