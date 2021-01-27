#include "graphicsModel.h"

#include "graphics/graphicsCore.h"
#include "graphics/graphicsSwapChain.h"
#include "graphics/graphicsUtils.h"

#include <vk_mem_alloc.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include <stb_image.h>
#include <tiny_obj_loader.h>

#include <unordered_map>
#include <iostream>

namespace std
{
    template<> struct hash<Vertex>
    {
        size_t operator()(Vertex const& vertex) const
        {
            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

const std::string MODEL_PATH = "data/models/viking_room.obj";
const std::string TEXTURE_PATH = "data/textures/viking_room.png";

static uint32_t s_numModels = 0;
static PkGraphicsModelViewProjection* s_pGraphicsModelViewProjection = nullptr;
static VkCommandPool s_commandPool;

struct UniformBufferObject
{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

struct PkGraphicsModelData
{
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VmaAllocation> uniformBufferAllocations;
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
    std::vector<VkCommandBuffer> commandBuffers;

    VkImage textureImage;
    VkImageView textureImageView;
    VmaAllocation textureImageAllocation;
    uint32_t mipLevels;
    VkSampler textureSampler;

    std::vector<InstanceData> instances;
    VkBuffer instanceBuffer;
    VmaAllocation instanceBufferAllocation;

    std::vector<Vertex> vertices;
    VkBuffer vertexBuffer;
    VmaAllocation vertexBufferAllocation;

    std::vector<uint32_t> indices;
    VkBuffer indexBuffer;
    VmaAllocation indexBufferAllocation;
};

static void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBuffer commandBuffer = pkGraphicsUtils_BeginSingleTimeCommands(PkGraphicsCore::GetDevice(), s_commandPool);

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    pkGraphicsUtils_EndSingleTimeCommands(PkGraphicsCore::GetDevice(), PkGraphicsCore::GetGraphicsQueue(), s_commandPool, commandBuffer);
}

static void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer = pkGraphicsUtils_BeginSingleTimeCommands(PkGraphicsCore::GetDevice(), s_commandPool);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { width, height, 1 };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    pkGraphicsUtils_EndSingleTimeCommands(PkGraphicsCore::GetDevice(), PkGraphicsCore::GetGraphicsQueue(), s_commandPool, commandBuffer);
}

static void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* pBuffer, VmaAllocation* pBufferAllocation)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = properties;

    if (vmaCreateBuffer(PkGraphicsCore::GetAllocator(), &bufferInfo, &allocInfo, pBuffer, pBufferAllocation, nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create buffer!");
    }
}

static void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageAspectFlags aspectFlags, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkImageView& imageView, VmaAllocation& imageAllocation)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = numSamples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = properties;

    if (vmaCreateImage(PkGraphicsCore::GetAllocator(), &imageInfo, &allocInfo, &image, &imageAllocation, nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create buffer!");
    }

    imageView = pkGraphicsUtils_CreateImageView(PkGraphicsCore::GetDevice(), image, format, aspectFlags, mipLevels);
}

static void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
{
    // Check if image format supports linear blitting
    VkFormatProperties formatProperties;
    PkGraphicsCore::GetFormatProperties(imageFormat, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
    {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }

    VkCommandBuffer commandBuffer = pkGraphicsUtils_BeginSingleTimeCommands(PkGraphicsCore::GetDevice(), s_commandPool);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < mipLevels; i++)
    {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(commandBuffer,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    pkGraphicsUtils_EndSingleTimeCommands(PkGraphicsCore::GetDevice(), PkGraphicsCore::GetGraphicsQueue(), s_commandPool, commandBuffer);
}

static void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels)
{
    VkCommandBuffer commandBuffer = pkGraphicsUtils_BeginSingleTimeCommands(PkGraphicsCore::GetDevice(), s_commandPool);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    pkGraphicsUtils_EndSingleTimeCommands(PkGraphicsCore::GetDevice(), PkGraphicsCore::GetGraphicsQueue(), s_commandPool, commandBuffer);
}

static void createCommandPool()
{
    PkGraphicsQueueFamilyIndices queueFamilyIndices = pkGraphicsUtils_FindQueueFamilies(PkGraphicsCore::GetPhysicalDevice(), PkGraphicsCore::GetSurface());

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(PkGraphicsCore::GetDevice(), &poolInfo, nullptr, &s_commandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics command pool!");
    }
}

static void createUniformBuffers(PkGraphicsModelData& rData)
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    size_t bufferCount = PkGraphicsSwapChain::GetNumSwapChainImages();

    rData.uniformBuffers.resize(bufferCount);
    rData.uniformBufferAllocations.resize(bufferCount);

    for (size_t i = 0; i < bufferCount; i++)
    {
        createBuffer
        (
            bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &rData.uniformBuffers[i],
            &rData.uniformBufferAllocations[i]
        );
    }
}

static void destroyUniformBuffers(PkGraphicsModelData& rData)
{
    size_t bufferCount = PkGraphicsSwapChain::GetNumSwapChainImages();

    for (size_t i = 0; i < bufferCount; i++)
    {
        vmaDestroyBuffer(PkGraphicsCore::GetAllocator(), rData.uniformBuffers[i], rData.uniformBufferAllocations[i]);
    }
}

static void createDescriptorPool(PkGraphicsModelData& rData)
{
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = PkGraphicsSwapChain::GetNumSwapChainImages();
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = PkGraphicsSwapChain::GetNumSwapChainImages();

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = PkGraphicsSwapChain::GetNumSwapChainImages();

    if (vkCreateDescriptorPool(PkGraphicsCore::GetDevice(), &poolInfo, nullptr, &rData.descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

static void createDescriptorSets(PkGraphicsModelData& rData, VkDescriptorSetLayout descriptorSetLayout)
{
    std::vector<VkDescriptorSetLayout> layouts(PkGraphicsSwapChain::GetNumSwapChainImages(), descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = rData.descriptorPool;
    allocInfo.descriptorSetCount = PkGraphicsSwapChain::GetNumSwapChainImages();
    allocInfo.pSetLayouts = layouts.data();

    rData.descriptorSets.resize(PkGraphicsSwapChain::GetNumSwapChainImages());
    if (vkAllocateDescriptorSets(PkGraphicsCore::GetDevice(), &allocInfo, rData.descriptorSets.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < PkGraphicsSwapChain::GetNumSwapChainImages(); i++)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = rData.uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = rData.textureImageView;
        imageInfo.sampler = rData.textureSampler;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = rData.descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = rData.descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(PkGraphicsCore::GetDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

static void createCommandBuffers(PkGraphicsModelData& rData, VkRenderPass renderPass, VkPipelineLayout pipelineLayout, VkPipeline pipeline, std::vector<VkFramebuffer>& rFramebuffers)
{
    rData.commandBuffers.resize(rFramebuffers.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = s_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)rData.commandBuffers.size();

    if (vkAllocateCommandBuffers(PkGraphicsCore::GetDevice(), &allocInfo, rData.commandBuffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    for (size_t i = 0; i < rData.commandBuffers.size(); i++)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(rData.commandBuffers[i], &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = rFramebuffers[i];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = PkGraphicsSwapChain::GetSwapChainExtent();

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        clearValues[1].depthStencil = { 1.0f, 0 };

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(rData.commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(rData.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        VkBuffer vertexBuffers[] = { rData.vertexBuffer };
        VkDeviceSize vertexOffsets[] = { 0 };
        vkCmdBindVertexBuffers(rData.commandBuffers[i], 0, 1, vertexBuffers, vertexOffsets);

        VkBuffer instanceBuffers[] = { rData.instanceBuffer };
        VkDeviceSize instanceOffsets[] = { 0 };
        vkCmdBindVertexBuffers(rData.commandBuffers[i], 1, 1, instanceBuffers, instanceOffsets);

        vkCmdBindIndexBuffer(rData.commandBuffers[i], rData.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(rData.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &rData.descriptorSets[i], 0, nullptr);

        vkCmdDrawIndexed(rData.commandBuffers[i], static_cast<uint32_t>(rData.indices.size()), static_cast<uint32_t>(rData.instances.size()), 0, 0, 0);

        vkCmdEndRenderPass(rData.commandBuffers[i]);

        if (vkEndCommandBuffer(rData.commandBuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to record command buffer!");
        }
    }
}

static void createTextureImage(PkGraphicsModelData& rData)
{
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    rData.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

    if (!pixels)
    {
        throw std::runtime_error("failed to load texture image!");
    }

    VkBuffer stagingBuffer;
    VmaAllocation stagingBufferAllocation;
    createBuffer
    (
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &stagingBuffer,
        &stagingBufferAllocation
    );

    void* data;
    vmaMapMemory(PkGraphicsCore::GetAllocator(), stagingBufferAllocation, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vmaUnmapMemory(PkGraphicsCore::GetAllocator(), stagingBufferAllocation);

    stbi_image_free(pixels);

    createImage
    (
        texWidth,
        texHeight,
        rData.mipLevels,
        VK_SAMPLE_COUNT_1_BIT,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        rData.textureImage,
        rData.textureImageView,
        rData.textureImageAllocation
    );

    transitionImageLayout(rData.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, rData.mipLevels);
    copyBufferToImage(stagingBuffer, rData.textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    //transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps

    vmaDestroyBuffer(PkGraphicsCore::GetAllocator(), stagingBuffer, stagingBufferAllocation);

    generateMipmaps(rData.textureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, rData.mipLevels);
}

static void createTextureSampler(PkGraphicsModelData& rData)
{
    VkPhysicalDeviceProperties properties{};
    PkGraphicsCore::GetPhysicalDeviceProperties(&properties);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(rData.mipLevels);
    samplerInfo.mipLodBias = 0.0f;

    if (vkCreateSampler(PkGraphicsCore::GetDevice(), &samplerInfo, nullptr, &rData.textureSampler) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

static void populateInstanceData(PkGraphicsModelData& rData)
{
    uint32_t boardDimensions = 1;

    const uint32_t instanceCount = boardDimensions * boardDimensions;

    rData.instances.resize(instanceCount);

    float tileSize = 1.5f;

    float rowStat = tileSize * boardDimensions / static_cast<float>(2.0f);

    float x = 0.0f;// -rowStat;
    float y = 0.0f;// -rowStat;
    uint32_t xStep = 0;

    float rot = 0.0f;

    for (uint32_t i = 0; i < instanceCount; i++)
    {
        rData.instances[i].pos = glm::vec3(x, y, 0.0f);
        rData.instances[i].rot = glm::radians(rot);

        rot += 90.0f;

        xStep++;
        if (xStep >= boardDimensions)
        {
            xStep = 0;
            y += tileSize;
            x = -rowStat;
        }
        else
        {
            x += tileSize;
        }
    }
}

static void loadModel(PkGraphicsModelData& rData)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
        throw std::runtime_error(warn + err);
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    for (const auto& shape : shapes) 
    {
        for (const auto& index : shape.mesh.indices) 
        {
            Vertex vertex{};

            vertex.pos = 
            {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.texCoord = 
            {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.color = { 1.0f, 1.0f, 1.0f };

            if (uniqueVertices.count(vertex) == 0) 
            {
                uniqueVertices[vertex] = static_cast<uint32_t>(rData.vertices.size());
                rData.vertices.push_back(vertex);
            }

            rData.indices.push_back(uniqueVertices[vertex]);
        }
    }
}

static void createInstanceBuffer(PkGraphicsModelData& rData)
{
    VkDeviceSize bufferSize = sizeof(rData.instances[0]) * rData.instances.size();

    VkBuffer stagingBuffer;
    VmaAllocation stagingBufferAllocation;
    createBuffer
    (
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &stagingBuffer,
        &stagingBufferAllocation
    );

    void* data;
    vmaMapMemory(PkGraphicsCore::GetAllocator(), stagingBufferAllocation, &data);
    memcpy(data, rData.instances.data(), (size_t)bufferSize);
    vmaUnmapMemory(PkGraphicsCore::GetAllocator(), stagingBufferAllocation);

    createBuffer
    (
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &rData.instanceBuffer,
        &rData.instanceBufferAllocation
    );

    copyBuffer(stagingBuffer, rData.instanceBuffer, bufferSize);

    vmaDestroyBuffer(PkGraphicsCore::GetAllocator(), stagingBuffer, stagingBufferAllocation);
}

static void createVertexBuffer(PkGraphicsModelData& rData)
{
    VkDeviceSize bufferSize = sizeof(rData.vertices[0]) * rData.vertices.size();

    VkBuffer stagingBuffer;
    VmaAllocation stagingBufferAllocation;
    createBuffer
    (
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &stagingBuffer,
        &stagingBufferAllocation
    );

    void* data;
    vmaMapMemory(PkGraphicsCore::GetAllocator(), stagingBufferAllocation, &data);
    memcpy(data, rData.vertices.data(), (size_t)bufferSize);
    vmaUnmapMemory(PkGraphicsCore::GetAllocator(), stagingBufferAllocation);

    createBuffer
    (
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &rData.vertexBuffer,
        &rData.vertexBufferAllocation
    );

    copyBuffer(stagingBuffer, rData.vertexBuffer, bufferSize);

    vmaDestroyBuffer(PkGraphicsCore::GetAllocator(), stagingBuffer, stagingBufferAllocation);
}

static void createIndexBuffer(PkGraphicsModelData& rData)
{
    VkDeviceSize bufferSize = sizeof(rData.indices[0]) * rData.indices.size();

    VkBuffer stagingBuffer;
    VmaAllocation stagingBufferAllocation;
    createBuffer
    (
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &stagingBuffer,
        &stagingBufferAllocation
    );

    void* data;
    vmaMapMemory(PkGraphicsCore::GetAllocator(), stagingBufferAllocation, &data);
    memcpy(data, rData.indices.data(), (size_t)bufferSize);
    vmaUnmapMemory(PkGraphicsCore::GetAllocator(), stagingBufferAllocation);

    createBuffer
    (
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &rData.indexBuffer,
        &rData.indexBufferAllocation
    );

    copyBuffer(stagingBuffer, rData.indexBuffer, bufferSize);

    vmaDestroyBuffer(PkGraphicsCore::GetAllocator(), stagingBuffer, stagingBufferAllocation);
}

void updateUniformBuffer(PkGraphicsModelData& rData, const uint32_t imageIndex)
{
    float fieldOfView = s_pGraphicsModelViewProjection->fieldOfView;
    float aspectRatio = PkGraphicsSwapChain::GetSwapChainExtent().width / (float)PkGraphicsSwapChain::GetSwapChainExtent().height;
    float nearViewPlane = s_pGraphicsModelViewProjection->nearViewPlane;
    float farViewPlane = s_pGraphicsModelViewProjection->farViewPlane;

    UniformBufferObject ubo{};
    ubo.model = s_pGraphicsModelViewProjection->model;
    ubo.view = s_pGraphicsModelViewProjection->view;
    ubo.proj = glm::perspective(glm::radians(fieldOfView), aspectRatio, nearViewPlane, farViewPlane);
    ubo.proj[1][1] *= -1;

    void* data;
    vmaMapMemory(PkGraphicsCore::GetAllocator(), rData.uniformBufferAllocations[imageIndex], &data);
    memcpy(data, &ubo, sizeof(ubo));
    vmaUnmapMemory(PkGraphicsCore::GetAllocator(), rData.uniformBufferAllocations[imageIndex]);
}

VkCommandBuffer& PkGraphicsModel::GetCommandBuffer(const uint32_t imageIndex)
{
    updateUniformBuffer(*m_pData, imageIndex);
    return m_pData->commandBuffers[imageIndex];
}

void PkGraphicsModel::OnSwapChainCreate(VkDescriptorSetLayout descriptorSetLayout, VkRenderPass renderPass, VkPipelineLayout pipelineLayout, VkPipeline pipeline, std::vector<VkFramebuffer>& rFramebuffers)
{
    createUniformBuffers(*m_pData);
    createDescriptorPool(*m_pData);
    createDescriptorSets(*m_pData, descriptorSetLayout);
    createCommandBuffers(*m_pData, renderPass, pipelineLayout, pipeline, rFramebuffers);
}

void PkGraphicsModel::OnSwapChainDestroy()
{
    vkFreeCommandBuffers(PkGraphicsCore::GetDevice(), s_commandPool, static_cast<uint32_t>(m_pData->commandBuffers.size()), m_pData->commandBuffers.data());
    vkDestroyDescriptorPool(PkGraphicsCore::GetDevice(), m_pData->descriptorPool, nullptr);
    destroyUniformBuffers(*m_pData);
}

PkGraphicsModel::PkGraphicsModel(PkGraphicsModelViewProjection& rModelViewProjection)
{
    m_pData = new PkGraphicsModelData();
    s_pGraphicsModelViewProjection = &rModelViewProjection;

    if (s_numModels == 0)
    {
        createCommandPool();
    }
    s_numModels++;

    createTextureImage(*m_pData);
    createTextureSampler(*m_pData);
    populateInstanceData(*m_pData);
    loadModel(*m_pData);
    createInstanceBuffer(*m_pData);
    createVertexBuffer(*m_pData);
    createIndexBuffer(*m_pData);
}

PkGraphicsModel::~PkGraphicsModel()
{
    vkDestroySampler(PkGraphicsCore::GetDevice(), m_pData->textureSampler, nullptr);
    vkDestroyImageView(PkGraphicsCore::GetDevice(), m_pData->textureImageView, nullptr);
    vmaDestroyImage(PkGraphicsCore::GetAllocator(), m_pData->textureImage, m_pData->textureImageAllocation);
    vmaDestroyBuffer(PkGraphicsCore::GetAllocator(), m_pData->indexBuffer, m_pData->indexBufferAllocation);
    vmaDestroyBuffer(PkGraphicsCore::GetAllocator(), m_pData->vertexBuffer, m_pData->vertexBufferAllocation);
    vmaDestroyBuffer(PkGraphicsCore::GetAllocator(), m_pData->instanceBuffer, m_pData->instanceBufferAllocation);

    s_numModels--;
    if (s_numModels == 0)
    {
        vkDestroyCommandPool(PkGraphicsCore::GetDevice(), s_commandPool, nullptr);
    }

    s_pGraphicsModelViewProjection = nullptr;
    delete m_pData;
}
