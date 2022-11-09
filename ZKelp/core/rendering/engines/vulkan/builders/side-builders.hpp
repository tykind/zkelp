/*
*	Desc: Side functions which are used within the main builder
*	Note: This isn't used in the main engine, it's just on the builder
*/
#pragma once
#include <cstdint>
#include <algorithm>
#include <vector>
#include <optional>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


#define STB_IMAGE_IMPLEMENTATION  
#include "../../../../../3rd party/stb/image.hpp"

#include "../../../../../utilities/utilFlags.hpp"
#include "../../../../../utilities/files/fs.hpp"
#include "../types/vtypes.hpp"

#undef min
#undef max

namespace vulkan
{
	static bool getMemoryType(VkPhysicalDeviceMemoryProperties deviceMemoryProps, uint32_t typeBits, VkFlags props, uint32_t* typeIndex)
	{
		auto ret{ false };
		 
		for (auto i = 0u; i < 32; i++)
		{
			if ((typeBits & 1) == 1)
				if ((deviceMemoryProps.memoryTypes[i].propertyFlags & props) == props) {
					*typeIndex = i;
					ret = true;
					break;
				}
			typeBits >>= 1;
		}

		return ret;
	}

	static auto beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool) {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
	}

	static void endSingleTimeCommands(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkCommandBuffer commandBuffer) {
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue);

		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	}

	static auto findMemoryType(VkPhysicalDevice physicalDevice, std::int32_t typeFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
				return i;

		throw err::err("failed to find memory type");
	}

	static auto choosePresentMode(const std::vector<VkPresentModeKHR> presentModes)
	{
		VkPresentModeKHR choosenPresentMode{ VK_PRESENT_MODE_FIFO_KHR };

		for (const auto& presentMode : presentModes) {
			if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				choosenPresentMode = presentMode;
				break;
			}
		}

		return choosenPresentMode;
	}

	static auto chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities)
	{
		if (surfaceCapabilities.currentExtent.width == -1)
		{
			VkExtent2D swapChainExtent;

			swapChainExtent.width = std::min(std::max(utils::windowInformation[0], surfaceCapabilities.minImageExtent.width), surfaceCapabilities.maxImageExtent.width);
			swapChainExtent.height = std::min(std::max(utils::windowInformation[1], surfaceCapabilities.minImageExtent.height), surfaceCapabilities.maxImageExtent.height);

			return swapChainExtent;
		}

		return surfaceCapabilities.currentExtent;
	}

	static auto chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		std::optional<VkSurfaceFormatKHR> ret{ std::nullopt };
		if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
			ret = { VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
		}

		if (!ret.has_value())
		{
			for (const auto& availableSurfaceFormat : availableFormats)
				if (availableSurfaceFormat.format == VK_FORMAT_R8G8B8A8_UNORM) {
					ret = availableSurfaceFormat;
					break;
				}
		}

		return ret.has_value() ? ret.value() : availableFormats[0];
	}

	static auto createDescriptorPool(VkDevice device)
	{
		VkDescriptorPool descriptorPool;

		VkDescriptorPoolSize typeCount;
		typeCount.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		typeCount.descriptorCount = 1;

		VkDescriptorPoolCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		createInfo.poolSizeCount = 1;
		createInfo.pPoolSizes = &typeCount;
		createInfo.maxSets = 1;

		if (vkCreateDescriptorPool(device, &createInfo, nullptr, &descriptorPool) != VK_SUCCESS)
			throw err::err("failed to create a description pool");

		return descriptorPool;
	}

	static auto createBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {

		VkBuffer buffer;
		VkDeviceMemory bufferMemory;

		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
			throw std::runtime_error("failed to allocate buffer memory!");

		vkBindBufferMemory(device, buffer, bufferMemory, 0);

		return std::tuple(buffer, bufferMemory);
	}

	static void copyBuffer(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
		VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

		VkBufferCopy copyRegion{};
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		endSingleTimeCommands(device, graphicsQueue, commandPool, commandBuffer);
	}

	static auto createDescriptorSet(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout, types::UniformBuffer* uniformBuffer)
	{
		VkDescriptorSet descriptorSet;

		// Allocate the set

		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &descriptorSetLayout;

		if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
			throw err::err("cannot create a descriptor set");

		// Update descriptor set with uniform binding

		VkDescriptorBufferInfo descriptorBufferInfo = {};
		descriptorBufferInfo.buffer = uniformBuffer->buffer;
		descriptorBufferInfo.offset = 0;
		descriptorBufferInfo.range = sizeof(uniformBuffer->uniformData);

		VkWriteDescriptorSet writeDescriptorSet = {};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstSet = descriptorSet;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;
		writeDescriptorSet.dstBinding = 0;

		vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);

		return descriptorSet;
	}

	static auto createShaderModules(VkDevice device)
	{
		fs::Document vertexShader("D:\\Projects\\ZKelp\\x64\\Debug\\resources\\shaders\\vert.spv");
		fs::Document fragmentShader("D:\\Projects\\ZKelp\\x64\\Debug\\resources\\shaders\\frag.spv");

		// Check if the files were found
		
		if (!vertexShader.exists || !fragmentShader.exists)
			throw err::err("one of the shaders were not found!");

		// Create the vertex module

		const auto createModule = [&] (fs::Document* file) -> VkShaderModule {
			auto data = file->read();
			data.seekp(0, std::ios::end);

			auto dataSz = data.tellp();

			if (dataSz < 0)
				throw err::err("one of the shaders has no contents");

			auto source = data.str();
			std::vector<char> dataListed(source.begin(), source.end());

			VkShaderModuleCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

			createInfo.codeSize = dataListed.size();
			createInfo.pCode = std::bit_cast<std::uint32_t*>(dataListed.data());

			VkShaderModule shaderModule;
			const auto res = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
			if (res != VK_SUCCESS)
				throw err::err("failed to create a shader module");
			return shaderModule;
		};

		const auto vertexModule = createModule(&vertexShader);
		const auto fragmentModule = createModule(&fragmentShader);

		return std::make_tuple(vertexModule, fragmentModule);
	}

	static auto createImage(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
		VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
	{
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS)
			throw err::err("failed to create image");

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(device, image, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
			throw err::err("could not allocate image memory");

		vkBindImageMemory(device, image, imageMemory, 0);
	}

	static auto createImageView(VkDevice device, VkImage image, VkFormat format)
	{
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
			throw err::err("couldn't create image view for an image");

		return imageView;
	}

	static auto createSampler(VkDevice device, VkPhysicalDevice physicalDevice)
	{
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;

		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

		samplerInfo.anisotropyEnable = VK_TRUE;

		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);

		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		VkSampler sampler;

		if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
			throw err::err("failed to create a texture sampler");

		return sampler;
	}

	static void copyBufferToImage(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
		VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = {
			width,
			height,
			1
		};

		vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		endSingleTimeCommands(device, graphicsQueue, commandPool, commandBuffer);
	}


	static void transitionImageLayout(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
		VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;

		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = 0;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else {
			throw err::err("unsupported layout transition");
		}

		vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		endSingleTimeCommands(device, graphicsQueue, commandPool, commandBuffer);
	}

	static auto createTexture(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, VkCommandPool commandPool, const std::string_view& path)
	{
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(path.data(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		if (!pixels)
			throw std::runtime_error("failed to load texture image!");

		const auto imageBufferInfo = createBuffer(device, physicalDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void* data;
		vkMapMemory(device, std::get<1>(imageBufferInfo), 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(device, std::get<1>(imageBufferInfo));

		stbi_image_free(pixels);

		types::Texture texture;

		createImage(device, physicalDevice, texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture.image, texture.memory);

		transitionImageLayout(device, graphicsQueue, commandPool, texture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		copyBufferToImage(device, graphicsQueue, commandPool, std::get<0>(imageBufferInfo), texture.image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
		transitionImageLayout(device, graphicsQueue, commandPool, texture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(device, std::get<0>(imageBufferInfo), nullptr);
		vkFreeMemory(device, std::get<1>(imageBufferInfo), nullptr);

		texture.view = createImageView(device, texture.image, VK_FORMAT_R8G8B8A8_SRGB);
		texture.sampler = createSampler(device, physicalDevice);

		return texture;
	}
}