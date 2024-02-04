#include "Application.h"

int main()
{
	static const hyper::Spec spec{ DEBUG_ON, "App", 1600, 900, VK_MAKE_API_VERSION(0,1,3,0) };
	hyper::Application* app = new hyper::Application(spec);
	delete app;
}