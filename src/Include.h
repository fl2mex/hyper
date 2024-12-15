#pragma once

#pragma warning(push, 0) // Disable warnings from external libraries, mostly to shut intellisense up
#pragma warning (disable: 26495) // Unitialised member warning code
#pragma warning (disable: 4098) // MSVC warning to do with debug mode
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_structs.hpp> // Intellisense please shut up, it only whines about the structs file
#include <GLFW/glfw3.h>
#pragma warning (pop)

#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <set>

// Macros
#define DEBUG_ON true
#define DEBUG_OFF false

// My Spec
namespace hyper
{
	struct Spec
	{ // Default options, just in case
		bool Debug = DEBUG_OFF;
		std::string Title = "App";
		uint32_t Width = 1600, Height = 900;
		uint32_t ApiVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
		std::string VertexShader = "vertex.spv", FragmentShader = "fragment.spv";
	};
}

// Debug Logger helper func
#define log(str) \
	do { \
		if (m_Spec.Debug) { \
			std::cout << str << "\n"; \
		} \
	} while(0)