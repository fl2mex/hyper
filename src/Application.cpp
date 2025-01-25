#include "Application.h"

namespace hyper
{
	static bool keys[348] = { false };

	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		if (action == GLFW_PRESS)
			keys[key] = true;
		if (action == GLFW_RELEASE)
			keys[key] = false;
	}

	static void FramebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		reinterpret_cast<Application*>(glfwGetWindowUserPointer(window))->GetRenderer().SetFramebufferResized();
	}

	Application::Application(Spec _spec)
		: m_Spec(_spec)
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // GLFW doesn't need to set this for vulkan
		m_Window = glfwCreateWindow(m_Spec.Width, m_Spec.Height, m_Spec.Title.c_str(), nullptr, nullptr);
		glfwSetKeyCallback(m_Window, KeyCallback);
		glfwSetFramebufferSizeCallback(m_Window, FramebufferResizeCallback);
		glfwSetWindowUserPointer(m_Window, this);

		m_Renderer.SetupRenderer(m_Spec, m_Window); // Can't use constructors because it needs to be in order
	}

	void Application::Run()
	{
		while (!glfwWindowShouldClose(m_Window))
		{
			glfwPollEvents();

			double currentTime = glfwGetTime();
			frameCount++;
			if (currentTime - previousTime >= 1.0)
			{
				glfwSetWindowTitle(m_Window, (m_Spec.Title + " | FPS: " + std::to_string(frameCount)).c_str());
				frameCount = 0;
				previousTime = currentTime;
			}

			if (keys[GLFW_KEY_ESCAPE])
				glfwSetWindowShouldClose(m_Window, GLFW_TRUE);
			m_Renderer.DrawFrame(); // Can add more things later, like audio :)
		}
	}

	Application::~Application()
	{
		glfwDestroyWindow(m_Window);
		glfwTerminate();
	}
}