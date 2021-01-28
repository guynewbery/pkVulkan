#include "graphicsRenderPassScene.h"

#include "graphics/graphicsCore.h"
#include "graphics/graphicsModel.h"
#include "graphics/graphicsSwapChain.h"
#include "graphics/graphicsUtils.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <fstream>

struct PkGrapicsRenderPassSceneData 
{
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    VkDescriptorSetLayout descriptorSetLayout;

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    std::vector<VkFramebuffer> framebuffers;

    VkImage colourImage;
    VkImageView colourImageView;
    VmaAllocation colourImageAllocation;

    VkImage depthImage;
    VkImageView depthImageView;
    VmaAllocation depthImageAllocation;

    std::vector<PkGraphicsModel*> pModels;
};

static PkGrapicsRenderPassSceneData* s_pData = nullptr;

static VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(PkGraphicsCore::GetPhysicalDevice(), format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
        {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

static VkFormat findDepthFormat()
{
    return findSupportedFormat
    (
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

static VkShaderModule createShaderModule(const std::vector<char>& code)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(PkGraphicsCore::GetDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

static std::vector<char> readFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

static void createCommandPool()
{
    PkGraphicsQueueFamilyIndices queueFamilyIndices = PkGraphicsUtils::FindQueueFamilies(PkGraphicsCore::GetPhysicalDevice(), PkGraphicsCore::GetSurface());

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(PkGraphicsCore::GetDevice(), &poolInfo, nullptr, &s_pData->commandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics command pool!");
    }
}

static void createDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(PkGraphicsCore::GetDevice(), &layoutInfo, nullptr, &s_pData->descriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

static void createColourResources()
{
    VkFormat colourFormat = PkGraphicsSwapChain::GetSwapChainImageFormat();

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = PkGraphicsSwapChain::GetSwapChainExtent().width;
    imageInfo.extent.height = PkGraphicsSwapChain::GetSwapChainExtent().height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = PkGraphicsSwapChain::GetSwapChainImageFormat();
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    imageInfo.samples = PkGraphicsCore::GetMaxMsaaSampleCount();
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    if (vmaCreateImage(PkGraphicsCore::GetAllocator(), &imageInfo, &allocInfo, &s_pData->colourImage, &s_pData->colourImageAllocation, nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create buffer!");
    }

    s_pData->colourImageView = PkGraphicsUtils::CreateImageView(PkGraphicsCore::GetDevice(), s_pData->colourImage, colourFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

static void destroyColourResources()
{
    vkDestroyImageView(PkGraphicsCore::GetDevice(), s_pData->colourImageView, nullptr);
    vmaDestroyImage(PkGraphicsCore::GetAllocator(), s_pData->colourImage, s_pData->colourImageAllocation);
}

static void createDepthResources()
{
    VkFormat depthFormat = findDepthFormat();

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = PkGraphicsSwapChain::GetSwapChainExtent().width;
    imageInfo.extent.height = PkGraphicsSwapChain::GetSwapChainExtent().height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = findDepthFormat();
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = PkGraphicsCore::GetMaxMsaaSampleCount();
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    if (vmaCreateImage(PkGraphicsCore::GetAllocator(), &imageInfo, &allocInfo, &s_pData->depthImage, &s_pData->depthImageAllocation, nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create buffer!");
    }

    s_pData->depthImageView = PkGraphicsUtils::CreateImageView(PkGraphicsCore::GetDevice(), s_pData->depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
}

static void destroyDepthResources()
{
    vkDestroyImageView(PkGraphicsCore::GetDevice(), s_pData->depthImageView, nullptr);
    vmaDestroyImage(PkGraphicsCore::GetAllocator(), s_pData->depthImage, s_pData->depthImageAllocation);
}

static void createRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = PkGraphicsSwapChain::GetSwapChainImageFormat();
    colorAttachment.samples = PkGraphicsCore::GetMaxMsaaSampleCount();
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = PkGraphicsCore::GetMaxMsaaSampleCount();
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription colorAttachmentResolve{};
    colorAttachmentResolve.format = PkGraphicsSwapChain::GetSwapChainImageFormat();
    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentResolveRef{};
    colorAttachmentResolveRef.attachment = 2;
    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    subpass.pResolveAttachments = &colorAttachmentResolveRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(PkGraphicsCore::GetDevice(), &renderPassInfo, nullptr, &s_pData->renderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render pass!");
    }
}

static void createPipeline()
{
    auto vertShaderCode = readFile("data/shaders/vert.spv");
    auto fragShaderCode = readFile("data/shaders/frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto bindingDescriptions = getBindingDescriptions();
    auto attributeDescriptions = getAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)PkGraphicsSwapChain::GetSwapChainExtent().width;
    viewport.height = (float)PkGraphicsSwapChain::GetSwapChainExtent().height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = PkGraphicsSwapChain::GetSwapChainExtent();

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = PkGraphicsCore::GetMaxMsaaSampleCount();

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &s_pData->descriptorSetLayout;

    if (vkCreatePipelineLayout(PkGraphicsCore::GetDevice(), &pipelineLayoutInfo, nullptr, &s_pData->pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = s_pData->pipelineLayout;
    pipelineInfo.renderPass = s_pData->renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(PkGraphicsCore::GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &s_pData->pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(PkGraphicsCore::GetDevice(), fragShaderModule, nullptr);
    vkDestroyShaderModule(PkGraphicsCore::GetDevice(), vertShaderModule, nullptr);
}

static void destroyPipeline()
{
    vkDestroyPipeline(PkGraphicsCore::GetDevice(), s_pData->pipeline, nullptr);
    vkDestroyPipelineLayout(PkGraphicsCore::GetDevice(), s_pData->pipelineLayout, nullptr);
}

static void createFramebuffers()
{
    s_pData->framebuffers.resize(PkGraphicsSwapChain::GetNumSwapChainImages());

    for (uint32_t i = 0; i < PkGraphicsSwapChain::GetNumSwapChainImages(); i++)
    {
        std::array<VkImageView, 3> attachments =
        {
            s_pData->colourImageView,
            s_pData->depthImageView,
            PkGraphicsSwapChain::GetSwapChainImageView(i)
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = s_pData->renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = PkGraphicsSwapChain::GetSwapChainExtent().width;
        framebufferInfo.height = PkGraphicsSwapChain::GetSwapChainExtent().height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(PkGraphicsCore::GetDevice(), &framebufferInfo, nullptr, &s_pData->framebuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

static void createCommandBuffers()
{
    s_pData->commandBuffers.resize(s_pData->framebuffers.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = s_pData->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)s_pData->commandBuffers.size();

    if (vkAllocateCommandBuffers(PkGraphicsCore::GetDevice(), &allocInfo, s_pData->commandBuffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    for (size_t i = 0; i < s_pData->commandBuffers.size(); i++)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(s_pData->commandBuffers[i], &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = s_pData->renderPass;
        renderPassInfo.framebuffer = s_pData->framebuffers[i];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = PkGraphicsSwapChain::GetSwapChainExtent();

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        clearValues[1].depthStencil = { 1.0f, 0 };

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(s_pData->commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(s_pData->commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, s_pData->pipeline);

        for (PkGraphicsModel* pModel : s_pData->pModels)
        {
            pModel->DrawModel(s_pData->commandBuffers[i], s_pData->pipelineLayout, static_cast<uint32_t>(i));
        }

        vkCmdEndRenderPass(s_pData->commandBuffers[i]);

        if (vkEndCommandBuffer(s_pData->commandBuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to record command buffer!");
        }
    }
}

static void destroyFramebuffers()
{
    for (VkFramebuffer framebuffer : s_pData->framebuffers)
    {
        vkDestroyFramebuffer(PkGraphicsCore::GetDevice(), framebuffer, nullptr);
    }
}

/*static*/ VkCommandBuffer& PkGraphicsRenderPassScene::GetCommandBuffer(uint32_t imageIndex)
{
    return s_pData->commandBuffers[imageIndex];
}

/*static*/ void PkGraphicsRenderPassScene::UpdateResourceDescriptors(const uint32_t imageIndex)
{
    for (PkGraphicsModel* pModel : s_pData->pModels)
    {
        pModel->UpdateUniformBuffer(imageIndex);
    }
}

/*static*/ void PkGraphicsRenderPassScene::OnSwapChainCreate()
{
    for (PkGraphicsModel* pModel : s_pData->pModels)
    {
        pModel->OnSwapChainCreate(s_pData->descriptorSetLayout);
    }

    createColourResources();
    createDepthResources();
    createRenderPass();
    createPipeline();
    createFramebuffers();
    createCommandBuffers();
}

/*static*/ void PkGraphicsRenderPassScene::OnSwapChainDestroy()
{
    vkFreeCommandBuffers(PkGraphicsCore::GetDevice(), s_pData->commandPool, static_cast<uint32_t>(s_pData->commandBuffers.size()), s_pData->commandBuffers.data());
    destroyFramebuffers();
    destroyPipeline();
    vkDestroyRenderPass(PkGraphicsCore::GetDevice(), s_pData->renderPass, nullptr);
    destroyDepthResources();
    destroyColourResources();

    for (PkGraphicsModel* pModel : s_pData->pModels)
    {
        pModel->OnSwapChainDestroy();
    }

}

/*static*/ void PkGraphicsRenderPassScene::InitialiseGraphicsRenderPassScene()
{
    s_pData = new PkGrapicsRenderPassSceneData();
    createCommandPool();
    createDescriptorSetLayout();

    s_pData->pModels.resize(2);

    s_pData->pModels[0] = new PkGraphicsModel(s_pData->commandPool, "data/models/viking_room.obj", "data/textures/viking_room.png");
    glm::mat4 m0 = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    s_pData->pModels[0]->SetMatrix(m0);

    s_pData->pModels[1] = new PkGraphicsModel(s_pData->commandPool, "data/models/viking_room.obj", "data/textures/viking_room.png");
    glm::mat4 m1 = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    s_pData->pModels[1]->SetMatrix(m1);

    OnSwapChainCreate();
}

/*static*/ void PkGraphicsRenderPassScene::CleanupGraphicsRenderPassScene()
{
    OnSwapChainDestroy();

    for (uint32_t i = 0; i < s_pData->pModels.size(); ++i)
    {
        delete s_pData->pModels[i];
    }

    vkDestroyDescriptorSetLayout(PkGraphicsCore::GetDevice(), s_pData->descriptorSetLayout, nullptr);
    vkDestroyCommandPool(PkGraphicsCore::GetDevice(), s_pData->commandPool, nullptr);
    delete s_pData;
}
