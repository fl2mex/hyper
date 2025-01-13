#pragma once
#define _CRT_SECURE_NO_WARNINGS // Me when MSVC-only problem that only affects vulkan versions
#include <GLFW/glfw3.h>			// >1.3.275 and I didn't originally notice because I was ON 1.3.275
#undef _CRT_SECURE_NO_WARNINGS	// and only then noticed when I had to reinstall the SDK :)))))

#include "Include.h"
#include "Renderer.h"

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