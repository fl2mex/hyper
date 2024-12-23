#pragma once
#include "Include.h"
#include "Renderer.h"

namespace hyper
{
	class Application
	{
	public:
		Application(Spec _spec);
		void Run();
		~Application();

		Renderer& GetRenderer() { return m_Renderer; }
	private:
		Spec m_Spec;

		GLFWwindow* m_Window{};
		Renderer m_Renderer;
	};
}