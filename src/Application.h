#pragma once
#include <GLFW/glfw3.h>

#include "Spec.h"
#include "Renderer.h"
#include "Logger.h"

namespace hyper
{
	class Application
	{
	public:
		Application(Spec _spec = {});
		void Run();
		~Application();

		Renderer& GetRenderer() { return m_Renderer; }
	private:
		Spec m_Spec;

		GLFWwindow* m_Window{};
		Renderer m_Renderer;
	};
}