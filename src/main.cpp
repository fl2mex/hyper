#include "Application.h"

int main()
{
	hyper::Spec spec{ DEBUG_ON, /* Title of window */ "hyper app :D", /* Width */ 1600, /* Height */ 900, VK_MAKE_API_VERSION(0,1,3,0),
		"res/shader/vertex.spv", "res/shader/fragment.spv" }; // These two are relative paths from Visual Studio's debugger!! Make sure your filepath is correct
	hyper::Application app(spec);
	app.Run();
}