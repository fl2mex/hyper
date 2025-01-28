#include "Buffer.h"

namespace hyper
{
	Buffer CreateBuffer(VmaAllocator& allocator, vk::DeviceSize size, vk::BufferUsageFlags usage, VmaMemoryUsage memoryUsage)
	{
		vk::BufferCreateInfo bufferInfo{ {}, size, usage, vk::SharingMode::eExclusive };
		VmaAllocationCreateInfo allocCreateInfo{ VMA_ALLOCATION_CREATE_MAPPED_BIT, memoryUsage };
		Buffer buffer;
		vmaCreateBuffer(allocator, reinterpret_cast<VkBufferCreateInfo*>(&bufferInfo), &allocCreateInfo,
			reinterpret_cast<VkBuffer*>(&buffer.Buffer), &buffer.Allocation, &buffer.AllocationInfo);
		return buffer;
	}

	Buffer CreateBufferStaged(VmaAllocator& allocator, vk::CommandPool& commandPool, vk::Device& device, vk::Queue& deviceQueue, vk::DeviceSize size,
		vk::BufferUsageFlags usage, const void* data)
	{
		Buffer buffer = CreateBuffer(allocator, size, usage | vk::BufferUsageFlagBits::eTransferDst, VMA_MEMORY_USAGE_GPU_ONLY);
		Buffer stagingBuffer = CreateBuffer(allocator, size, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY);
		memcpy(stagingBuffer.AllocationInfo.pMappedData, data, size);
		CopyBuffer(commandPool, device, deviceQueue, stagingBuffer, buffer, size);
		DestroyBuffer(allocator, stagingBuffer);
		return buffer;
	}

	void CopyBuffer(vk::CommandPool& commandPool, vk::Device& device, vk::Queue& deviceQueue, Buffer& src, Buffer& dst, vk::DeviceSize size)
	{
		vk::CommandBufferAllocateInfo allocInfo{ commandPool, vk::CommandBufferLevel::ePrimary, 1 };
		std::vector<vk::UniqueCommandBuffer> commandBuffer = device.allocateCommandBuffersUnique(allocInfo);
		vk::BufferCopy copyRegion{ 0, 0, size };

		vk::BufferMemoryBarrier2 topBufferMemoryBarrier2{ vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone,
			vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
			src.Buffer, 0, size };
		vk::BufferMemoryBarrier2 bottomBufferMemoryBarrier2{ vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite,
			vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
			dst.Buffer, 0, size };

		commandBuffer[0]->begin(vk::CommandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
		commandBuffer[0]->pipelineBarrier2({ vk::DependencyFlagBits::eByRegion, 0, nullptr, 1, &topBufferMemoryBarrier2 });
		commandBuffer[0]->copyBuffer(src.Buffer, dst.Buffer, 1, &copyRegion);
		commandBuffer[0]->pipelineBarrier2({ vk::DependencyFlagBits::eByRegion, 0, nullptr, 1, &bottomBufferMemoryBarrier2 });
		commandBuffer[0]->end();
		vk::SubmitInfo submitInfo{ 0, nullptr, nullptr, 1, &commandBuffer[0].get() };
		deviceQueue.submit(submitInfo, nullptr);
		deviceQueue.waitIdle();
	}

	void DestroyBuffer(VmaAllocator& allocator, Buffer& buffer)
	{
		vmaDestroyBuffer(allocator, buffer.Buffer, buffer.Allocation);
	}
}