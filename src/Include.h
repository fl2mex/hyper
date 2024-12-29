#pragma once

#pragma warning(push, 0) // Disable warnings from external libraries, mostly to shut intellisense up
#pragma warning (disable: 26495) // Unitialised member warning code
#pragma warning (disable: 4098) // MSVC warning to do with debug mode
#pragma warning(disable : 4996) // Something wrong with a new version of the API changes using strcpy_s to use strcpy, flipping off msvc's alarms
#define _CRT_SECURE_NO_WARNINGS // Shut up MSVC
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h> // From the pre-compiled win64 lib on their website
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp> // This came with the vulkan sdk but if you don't have it, just download it from github
#include <glm/gtc/matrix_transform.hpp>
#pragma warning (pop)
#undef _CRT_SECURE_NO_WARNINGS

#include <iostream> // Used everywhere
#include <fstream> // Used for reading files
#include <iomanip> // Used for time formatting
#include <chrono> // Used for timestamping logging

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
		uint32_t ApiVersion = VK_MAKE_API_VERSION(0, 1, 3, 0);
		std::string VertexShader = "res/shader/vertex.spv", FragmentShader = "res/shader/fragment.spv";
	};
}

// SPIRV file read func
static std::vector<char> readFile(std::string filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	if (!file.is_open())
	{
		std::cerr << "Failed to load \"" << filename << "\" \nMake sure your shader's location or filepath are correct!" << std::endl;
		return {}; // Need to add error handling to the constructor
	}
	size_t filesize{ static_cast<size_t>(file.tellg()) }; // Get the size of the file by seeking to the end and getting the position
	std::vector<char> buffer(filesize);
	file.seekg(0); // Seek back to the beginning
	file.read(buffer.data(), filesize);
	file.close();
	return buffer;
}