#pragma once
#include <iostream>
#include <filesystem>
#include <optional>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>

#include "Buffer.h"

namespace hyper
{
	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec4 color;
		glm::vec2 uv;

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
			attributeDescriptions[3] = vk::VertexInputAttributeDescription2EXT{ 3, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, uv) };
			return attributeDescriptions;
		}
	};
	struct GeoSurface
	{
		uint32_t startIndex;
		uint32_t count;
	};
	struct MeshAsset
	{
		std::string name;
		std::vector<GeoSurface> surfaces;
		Buffer vertexBuffer;
		Buffer indexBuffer;
	};

	static std::optional<std::vector<std::shared_ptr<MeshAsset>>> LoadModel(vk::CommandPool commandPool, vk::Device device, vk::Queue queue, VmaAllocator allocator,
		std::filesystem::path filePath)
	{
		auto gltfFile = fastgltf::GltfDataBuffer::FromPath(filePath);
		if (gltfFile.error() != fastgltf::Error::None) return {};
		constexpr auto gltfOptions = fastgltf::Options::LoadExternalBuffers;
		fastgltf::Parser parser{};
		auto load = parser.loadGltfBinary(gltfFile.get(), filePath.parent_path(), gltfOptions);
		fastgltf::Asset gltf;
		if (load) gltf = std::move(load.get());
		else return {};
		std::vector<std::shared_ptr<MeshAsset>> meshes;

		std::vector<uint32_t> indices;
		std::vector<Vertex> vertices;

		for (fastgltf::Mesh& mesh : gltf.meshes) {
			MeshAsset newmesh;

			newmesh.name = mesh.name;

			// clear the mesh arrays each mesh, we dont want to merge them by error
			indices.clear();
			vertices.clear();

			for (auto&& p : mesh.primitives) {
				GeoSurface newSurface;
				newSurface.startIndex = (uint32_t)indices.size();
				newSurface.count = (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

				size_t initial_vtx = vertices.size();

				// load indexes
				{
					fastgltf::Accessor& indexaccessor = gltf.accessors[p.indicesAccessor.value()];
					indices.reserve(indices.size() + indexaccessor.count);
					fastgltf::iterateAccessor<std::uint32_t>(gltf, indexaccessor,
						[&](std::uint32_t idx) {
							indices.push_back(idx + initial_vtx);
						});
				}

				// load vertex positions
				{
					fastgltf::Accessor& posAccessor = gltf.accessors[p.findAttribute("POSITION")->accessorIndex];
					vertices.resize(vertices.size() + posAccessor.count);

					fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor,
						[&](glm::vec3 v, size_t index) {
							Vertex newvtx;
							newvtx.position = v;
							newvtx.normal = { 1, 0, 0 };
							newvtx.color = glm::vec4{ 1.f };
							newvtx.uv = { 0, 0 };
							vertices[initial_vtx + index] = newvtx;
						});
				}
				// load vertex normals
				auto normals = p.findAttribute("NORMAL");
				if (normals != p.attributes.end()) {

					fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[(*normals).accessorIndex],
						[&](glm::vec3 v, size_t index) {
							vertices[initial_vtx + index].normal = v;
						});
				}

				// load UVs
				auto uv = p.findAttribute("TEXCOORD_0");
				if (uv != p.attributes.end()) {

					fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[(*uv).accessorIndex],
						[&](glm::vec2 v, size_t index) {
							vertices[initial_vtx + index].uv = v;
						});
				}

				// load vertex colors
				auto colors = p.findAttribute("COLOR_0");
				if (colors != p.attributes.end()) {

					fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*colors).accessorIndex],
						[&](glm::vec4 v, size_t index) {
							vertices[initial_vtx + index].color = v;
						});
				}
				newmesh.surfaces.push_back(newSurface);
			}

			// display the vertex normals
			constexpr bool OverrideColors = true;
			if (OverrideColors) {
				for (Vertex& vtx : vertices) {
					vtx.color = glm::vec4(vtx.normal, 1.f);
				}
			}
			newmesh.vertexBuffer = CreateBufferStaged(allocator, commandPool, device, queue, vertices.size() * sizeof(vertices[0]),
				vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress, vertices.data());
			newmesh.indexBuffer = CreateBufferStaged(allocator, commandPool, device, queue, indices.size() * sizeof(indices[0]),
				vk::BufferUsageFlagBits::eIndexBuffer, indices.data());

			meshes.emplace_back(std::make_shared<MeshAsset>(std::move(newmesh)));
		}

		return meshes;
	}
}