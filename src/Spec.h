#pragma once
#include <string>

#define DEBUG_ON true;
#define DEBUG_OFF false;

namespace hyper
{
	struct Spec
	{ // Default options, just in case
		bool Debug = DEBUG_ON;
		bool InfoDebug = DEBUG_ON;
		std::string Title = "App";
		uint32_t Width = 1600, Height = 900;
		uint32_t ApiVersion = 4206881; // 1.3.289
		// VK_MAKE_API_VERSION(0,1,3,0); = 4206592
		// VK_MAKE_API_VERSION(0,1,3,289); = 4206881
		// VK_MAKE_API_VERSION(0,1,4,0); = 4210688
	};
}