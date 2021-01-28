#pragma once

#include <vulkan/vulkan_core.h>
#include <glm/glm.hpp>

#include <array>
#include <vector>

struct PkGraphicsModelData;

struct InstanceData
{
    glm::vec3 pos;
    float rot;
};

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    bool operator==(const Vertex& other) const
    {
        return pos == other.pos && color == other.color && texCoord == other.texCoord;
    }
};

static std::array<VkVertexInputBindingDescription, 2> getBindingDescriptions()
{
    std::array<VkVertexInputBindingDescription, 2> bindingDescriptions{};

    bindingDescriptions[0].binding = 0;
    bindingDescriptions[0].stride = sizeof(Vertex);
    bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    bindingDescriptions[1].binding = 1;
    bindingDescriptions[1].stride = sizeof(InstanceData);
    bindingDescriptions[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

    return bindingDescriptions;
}

static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions() 
{
    std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, Vertex::pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, Vertex::color);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, Vertex::texCoord);

    attributeDescriptions[3].binding = 1;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[3].offset = offsetof(InstanceData, InstanceData::pos);

    attributeDescriptions[4].binding = 1;
    attributeDescriptions[4].location = 4;
    attributeDescriptions[4].format = VK_FORMAT_R32_SFLOAT;
    attributeDescriptions[4].offset = offsetof(InstanceData, InstanceData::rot);

    return attributeDescriptions;
}

class PkGraphicsModel
{
public:
    PkGraphicsModel() = delete;
	PkGraphicsModel(VkCommandPool commandPool, const char* pModelPath, const char* pTexturePath);
	~PkGraphicsModel();

    void SetMatrix(glm::mat4& rMat);

    void UpdateUniformBuffer(const uint32_t imageIndex);
    void DrawModel(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, const uint32_t imageIndex);

	void OnSwapChainCreate(VkDescriptorSetLayout descriptorSetLayout, VkRenderPass renderPass, VkPipelineLayout pipelineLayout, VkPipeline pipeline, std::vector<VkFramebuffer>& rFramebuffers);
	void OnSwapChainDestroy();

private:
	PkGraphicsModelData* m_pData;
};
