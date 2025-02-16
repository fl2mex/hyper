#pragma once
#include <GLFW/glfw3.h>

#include "Spec.h"
#include "Renderer.h"
#include "Logger.h"
#include "UserActions.h"

namespace hyper
{
	class Application
	{
	public:
		Application(Spec _spec = {});
		void Run();
		~Application();

		Renderer* GetRenderer() { return &m_Renderer; } // Pointer so hopefully i'm not copying 7 KILOBYTES of data every resize
	private:
		Spec m_Spec;

		double previousTime = 0.0;
		uint32_t frameCount = 0;

		GLFWwindow* m_Window{};
		Renderer m_Renderer;
	};
}