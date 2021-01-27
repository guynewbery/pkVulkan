#include "graphicsRenderPassImgui.h"

#include "graphics/graphicsCore.h"
#include "graphics/graphicsSwapChain.h"
#include "graphics/graphicsUtils.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_vulkan.h"
#include "imgui/imgui_impl_glfw.h"

#include <iostream>
#include <array>

struct PkGraphicsRenderPassImguiData
{
    std::vector<VkCommandPool> commandPools;

    VkDescriptorPool descriptorPool;
    VkRenderPass renderPass;

    std::vector<VkFramebuffer> frameBuffers;
    std::vector<VkCommandBuffer> commandBuffers;
};

static PkGraphicsRenderPassImguiData* s_pData = nullptr;

static void check_vk_result(VkResult err)
{
/*
    if (err == 0)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort()
*/
}

static void createCommandPools(PkGraphicsRenderPassImguiData& rData)
{
    rData.commandPools.resize(PkGraphicsSwapChain::GetNumSwapChainImages());

    PkGraphicsQueueFamilyIndices queueFamilyIndices = pkGraphicsUtils_FindQueueFamilies(PkGraphicsCore::GetPhysicalDevice(), PkGraphicsCore::GetSurface());

    for (size_t i = 0; i < PkGraphicsSwapChain::GetNumSwapChainImages(); i++)
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        if (vkCreateCommandPool(PkGraphicsCore::GetDevice(), &poolInfo, nullptr, &rData.commandPools[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create graphics command pool!");
        }
    }
}

static void destroyCommandPools(PkGraphicsRenderPassImguiData& rData)
{
    for (VkCommandPool commandPool : rData.commandPools)
    {
        vkDestroyCommandPool(PkGraphicsCore::GetDevice(), commandPool, nullptr);
    }
}

static void createDescriptorPool(PkGraphicsRenderPassImguiData& rData)
{
    VkDescriptorPoolSize poolSizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 1000 * IM_ARRAYSIZE(poolSizes);
    poolInfo.poolSizeCount = (uint32_t)IM_ARRAYSIZE(poolSizes);
    poolInfo.pPoolSizes = poolSizes;

    if (vkCreateDescriptorPool(PkGraphicsCore::GetDevice(), &poolInfo, nullptr, &rData.descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

static void destroyDescriptorPool(PkGraphicsRenderPassImguiData& rData)
{
    vkDestroyDescriptorPool(PkGraphicsCore::GetDevice(), rData.descriptorPool, nullptr);
}

static void createRenderPass(PkGraphicsRenderPassImguiData& rData)
{
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = PkGraphicsSwapChain::GetSwapChainImageFormat();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;  // or VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(PkGraphicsCore::GetDevice(), &renderPassInfo, nullptr, &rData.renderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("Could not create Dear ImGui's render pass");
    }
}

static void destroyRenderPass(PkGraphicsRenderPassImguiData& rData)
{
    vkDestroyRenderPass(PkGraphicsCore::GetDevice(), rData.renderPass, nullptr);
}

static void createFrameBuffers(PkGraphicsRenderPassImguiData& rData)
{
    rData.frameBuffers.resize(PkGraphicsSwapChain::GetNumSwapChainImages());

    for (uint32_t i = 0; i < PkGraphicsSwapChain::GetNumSwapChainImages(); i++)
    {
        std::array<VkImageView, 1> attachments =
        {
            PkGraphicsSwapChain::GetSwapChainImageView(i),
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = rData.renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = PkGraphicsSwapChain::GetSwapChainExtent().width;
        framebufferInfo.height = PkGraphicsSwapChain::GetSwapChainExtent().height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(PkGraphicsCore::GetDevice(), &framebufferInfo, nullptr, &rData.frameBuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

static void destroyFrameBuffers(PkGraphicsRenderPassImguiData& rData)
{
    for (VkFramebuffer framebuffer : rData.frameBuffers)
    {
        vkDestroyFramebuffer(PkGraphicsCore::GetDevice(), framebuffer, nullptr);
    }
}

static void createCommandBuffers(PkGraphicsRenderPassImguiData& rData)
{
    rData.commandBuffers.resize(PkGraphicsSwapChain::GetNumSwapChainImages());

    for (size_t i = 0; i < PkGraphicsSwapChain::GetNumSwapChainImages(); i++)
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = rData.commandPools[i];
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(PkGraphicsCore::GetDevice(), &allocInfo, &rData.commandBuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }
}

static void destroyCommandBuffers(PkGraphicsRenderPassImguiData& rData)
{
    for (size_t i = 0; i < PkGraphicsSwapChain::GetNumSwapChainImages(); i++)
    {
        vkFreeCommandBuffers(PkGraphicsCore::GetDevice(), rData.commandPools[i], 1, &rData.commandBuffers[i]);
    }
}

/*static*/ VkCommandBuffer& PkGraphicsRenderPassImgui::GetCommandBuffer(uint32_t imageIndex)
{
    ImDrawData* pDrawData = ImGui::GetDrawData();

    if (vkResetCommandPool(PkGraphicsCore::GetDevice(), s_pData->commandPools[imageIndex], 0) != VK_SUCCESS)
    {
        throw std::runtime_error("vkResetCommandPool error");
    }

    VkCommandBufferBeginInfo commandBufferBeginInfo = {};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(s_pData->commandBuffers[imageIndex], &commandBufferBeginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("commandBufferBeginInfo error");
    }

    VkClearValue clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = s_pData->renderPass;
    renderPassInfo.framebuffer = s_pData->frameBuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = PkGraphicsSwapChain::GetSwapChainExtent();
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearValue;

    vkCmdBeginRenderPass(s_pData->commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Record dear imgui primitives into command buffer
    ImGui_ImplVulkan_RenderDrawData(pDrawData, s_pData->commandBuffers[imageIndex]);

    // Submit command buffer
    vkCmdEndRenderPass(s_pData->commandBuffers[imageIndex]);

    if (vkEndCommandBuffer(s_pData->commandBuffers[imageIndex]) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer!");
    }


    return s_pData->commandBuffers[imageIndex];
}

/*static*/ void PkGraphicsRenderPassImgui::BeginImguiFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

/*static*/ void PkGraphicsRenderPassImgui::EndImguiFrame()
{
    ImGui::Render();
}

/*static*/ void PkGraphicsRenderPassImgui::OnSwapChainCreate()
{
    createRenderPass(*s_pData);
    createFrameBuffers(*s_pData);
    createCommandBuffers(*s_pData);
}

/*static*/ void PkGraphicsRenderPassImgui::OnSwapChainDestroy()
{
    destroyCommandBuffers(*s_pData);
    destroyFrameBuffers(*s_pData);
    destroyRenderPass(*s_pData);
}

/*static*/ void PkGraphicsRenderPassImgui::InitialiseGraphicsRenderPassImgui()
{
    s_pData = new PkGraphicsRenderPassImguiData();

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    PkGraphicsQueueFamilyIndices indices = pkGraphicsUtils_FindQueueFamilies(PkGraphicsCore::GetPhysicalDevice(), PkGraphicsCore::GetSurface());

    createCommandPools(*s_pData);
    createDescriptorPool(*s_pData);

    OnSwapChainCreate();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(PkGraphicsCore::GetWindow(), true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = PkGraphicsCore::GetInstance();
    init_info.PhysicalDevice = PkGraphicsCore::GetPhysicalDevice();
    init_info.Device = PkGraphicsCore::GetDevice();
    init_info.QueueFamily = indices.graphicsFamily.value();
    init_info.Queue = PkGraphicsCore::GetGraphicsQueue();
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = s_pData->descriptorPool;
    init_info.Allocator = nullptr;// PkGraphicsCore::GetAllocator()->GetAllocationCallbacks();
    init_info.MinImageCount = 2;
    init_info.ImageCount = PkGraphicsSwapChain::GetNumSwapChainImages();
    init_info.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&init_info, s_pData->renderPass);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // Upload Fonts
    {
        VkCommandBuffer commandBuffer = pkGraphicsUtils_BeginSingleTimeCommands(PkGraphicsCore::GetDevice(), s_pData->commandPools[0]);
        ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
        pkGraphicsUtils_EndSingleTimeCommands(PkGraphicsCore::GetDevice(), PkGraphicsCore::GetGraphicsQueue(), s_pData->commandPools[0], commandBuffer);

        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }
}

/*static*/ void PkGraphicsRenderPassImgui::CleanupGraphicsRenderPassImgui()
{
    OnSwapChainDestroy();

    destroyDescriptorPool(*s_pData);
    destroyCommandPools(*s_pData);

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    delete s_pData;
}
