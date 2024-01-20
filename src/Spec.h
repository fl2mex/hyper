#pragma once
#include "Include.h"
namespace hyper
{
	struct Spec
	{
		bool Debug = DEBUG_OFF;
		std::string Name = "App";
		uint32_t Width = 1600, Height = 900;
		uint32_t ApiVersion = VK_MAKE_API_VERSION(0,1,0,0);
	};
}