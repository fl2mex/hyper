#pragma once
#include <iostream>
#include <fstream>
#include <vector> // Somewhere in either vulkan.hpp, glfw3.h, glm.hpp, or matrix_transform.hpp, vector is being included, weird

// Macros
#define DEBUG_ON true;
#define DEBUG_OFF false;

// My Spec
namespace hyper
{
	struct Spec
	{ // Default options, just in case
		bool Debug = DEBUG_ON;
		bool InfoDebug = DEBUG_ON;
		std::string Title = "App";
		uint32_t Width = 1600, Height = 900;
		uint32_t ApiVersion = 4206592; // VK_MAKE_API_VERSION(0,1,3,0); // But without the dependency :)
	};
}

// SPIRV file read func
static std::vector<char> readFile(std::string filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	if (!file.is_open())
	{ // Has no access to the logger :O it's fine
		std::cerr << "Failed to load \"" << filename << "\" \nMake sure your shader's location or filepath are correct!" << std::endl;
		return {}; // Need to add error handling to the constructor
	}
	size_t filesize{ static_cast<size_t>(file.tellg()) }; // Get the size of the file by seeking to the end and getting the position
	std::vector<char> buffer(filesize);
	file.seekg(0); // You'd think C++ would have a cleaner way of doing this
	file.read(buffer.data(), filesize);
	file.close();
	return buffer;
}