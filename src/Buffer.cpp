#include "Buffer.h"

namespace hyper
{
	Buffer CreateBuff(VmaAllocator allocator, vk::DeviceSize size, vk::BufferUsageFlags usage, VmaMemoryUsage memoryUsage)
	{
		vk::BufferCreateInfo bufferInfo{ {}, size, usage, vk::SharingMode::eExclusive };
		VmaAllocationCreateInfo allocCreateInfo{ VMA_ALLOCATION_CREATE_MAPPED_BIT, memoryUsage };
		Buffer buffer;
		vmaCreateBuffer(allocator, reinterpret_cast<VkBufferCreateInfo*>(&bufferInfo), &allocCreateInfo,
			reinterpret_cast<VkBuffer*>(&buffer.Buffer), &buffer.Allocation, &buffer.AllocationInfo);
		return buffer;
	}

	void CopyBuff(vk::CommandPool commandPool, vk::Device device, vk::Queue deviceQueue, Buffer& src, Buffer& dst, vk::DeviceSize size)
	{
		vk::BufferCopy copyRegion{ 0, 0, size };
		vk::CommandBufferAllocateInfo allocInfo{ commandPool, vk::CommandBufferLevel::ePrimary, 1 };
		std::vector<vk::UniqueCommandBuffer> commandBuffer = device.allocateCommandBuffersUnique(allocInfo);
		commandBuffer[0]->begin(vk::CommandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
		commandBuffer[0]->copyBuffer(src.Buffer, dst.Buffer, 1, &copyRegion);
		commandBuffer[0]->end();
		vk::SubmitInfo submitInfo{ 0, nullptr, nullptr, 1, &commandBuffer[0].get() };
		deviceQueue.submit(submitInfo, nullptr);
		deviceQueue.waitIdle();
	}

	Buffer UltimateCreateBuff(vk::CommandPool commandPool, vk::Device device, vk::Queue deviceQueue, VmaAllocator allocator, vk::DeviceSize size,
		vk::BufferUsageFlags usage, const void* data)
	{
		Buffer buffer = CreateBuff(allocator, size, usage | vk::BufferUsageFlagBits::eTransferDst, VMA_MEMORY_USAGE_GPU_ONLY);
		Buffer stagingBuffer = CreateBuff(allocator, size, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY);
		void* bufferData;
		vmaMapMemory(allocator, stagingBuffer.Allocation, &bufferData);
		memcpy(bufferData, data, size);
		vmaUnmapMemory(allocator, stagingBuffer.Allocation);
		CopyBuff(commandPool, device, deviceQueue, stagingBuffer, buffer, size);
		DestroyBuff(allocator, stagingBuffer);
		return buffer;
	}

	void DestroyBuff(VmaAllocator allocator, Buffer& buffer)
	{
		vmaDestroyBuffer(allocator, buffer.Buffer, buffer.Allocation);
	}
}