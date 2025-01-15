#pragma once
#include <iostream>
#include <fstream>
#include <vector>

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