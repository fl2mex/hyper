#pragma once
#include <iostream>
#include <filesystem>
#include <optional>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "Buffer.h"

namespace hyper
{
	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec4 color;
		glm::vec2 texCoord;

		static vk::VertexInputBindingDescription2EXT getBindingDescription()
		{
			return vk::VertexInputBindingDescription2EXT{ 0, sizeof(Vertex), vk::VertexInputRate::eVertex, 1 };
		}
		static std::array<vk::VertexInputAttributeDescription2EXT, 4> getAttributeDescriptions()
		{
			std::array<vk::VertexInputAttributeDescription2EXT, 4> attributeDescriptions;
			attributeDescriptions[0] = vk::VertexInputAttributeDescription2EXT{ 0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position) };
			attributeDescriptions[1] = vk::VertexInputAttributeDescription2EXT{ 1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal) };
			attributeDescriptions[2] = vk::VertexInputAttributeDescription2EXT{ 2, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, color) };
			attributeDescriptions[3] = vk::VertexInputAttributeDescription2EXT{ 3, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord) };

			return attributeDescriptions;
		}
	};

	// Removed fastgltf, it's really hard to use, will try out tinygltf because it seems to be better documented and more widely used
}