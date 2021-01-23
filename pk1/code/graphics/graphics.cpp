#include "graphics.h"

#include "graphics/graphicsCore.h"
#include "graphics/graphicsRenderPassImgui.h"
#include "graphics/graphicsRenderPassScene.h"
#include "graphics/graphicsSwapChain.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>

#define MAX_FRAMES_IN_FLIGHT 2

struct PkGraphicsData
{
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame = 0;
};

static PkGraphicsData* s_pData = nullptr;

static void onSwapChainCreate()
{
    pkGraphicsRenderPassScene_OnSwapChainCreate();
    pkGraphicsRenderPassImgui_OnSwapChainCreate();
}

static void onSwapChainDestroy()
{
    pkGraphicsRenderPassImgui_OnSwapChainDestroy();
    pkGraphicsRenderPassScene_OnSwapChainDestroy();
}

static void getCommandBuffers(uint32_t imageIndex, std::vector<VkCommandBuffer>& buffers)
{
    buffers.resize(2);
    buffers[0] = pkGraphicsRenderPassScene_GetCommandBuffer(imageIndex);
    buffers[1] = pkGraphicsRenderPassImgui_GetCommandBuffer(imageIndex);
}

void createSyncObjects()
{
    s_pData->imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    s_pData->renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    s_pData->inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    s_pData->imagesInFlight.resize(pkGraphicsSwapChain_GetSwapChain().swapChainImages.size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(PkGraphicsCore::GetDevice(), &semaphoreInfo, nullptr, &s_pData->imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(PkGraphicsCore::GetDevice(), &semaphoreInfo, nullptr, &s_pData->renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(PkGraphicsCore::GetDevice(), &fenceInfo, nullptr, &s_pData->inFlightFences[i]) != VK_SUCCESS) 
        {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

void destroySyncObjects()
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(PkGraphicsCore::GetDevice(), s_pData->renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(PkGraphicsCore::GetDevice(), s_pData->imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(PkGraphicsCore::GetDevice(), s_pData->inFlightFences[i], nullptr);
    }
}

static void recreateSwapChain()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(PkGraphicsCore::GetWindow(), &width, &height);

    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(PkGraphicsCore::GetWindow(), &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(PkGraphicsCore::GetDevice());

    {
        onSwapChainDestroy();
        pkGraphicsSwapChain_Destroy(PkGraphicsCore::GetDevice());
    }

    {
        pkGraphicsSwapChain_Create(PkGraphicsCore::GetInstance(), PkGraphicsCore::GetPhysicalDevice(), PkGraphicsCore::GetDevice(), PkGraphicsCore::GetMaxMsaaSampleCount());
        onSwapChainCreate();
    }
}

/*static*/ bool PkGraphics::WindowShouldClose()
{
    return glfwWindowShouldClose(PkGraphicsCore::GetWindow());
}

/*static*/ void PkGraphics::RenderAndPresentFrame()
{
    vkWaitForFences(PkGraphicsCore::GetDevice(), 1, &s_pData->inFlightFences[s_pData->currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(PkGraphicsCore::GetDevice(), pkGraphicsSwapChain_GetSwapChain().swapChain, UINT64_MAX, s_pData->imageAvailableSemaphores[s_pData->currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapChain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    if (s_pData->imagesInFlight[imageIndex] != VK_NULL_HANDLE)
    {
        vkWaitForFences(PkGraphicsCore::GetDevice(), 1, &s_pData->imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    s_pData->imagesInFlight[imageIndex] = s_pData->inFlightFences[s_pData->currentFrame];

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { s_pData->imageAvailableSemaphores[s_pData->currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    std::vector<VkCommandBuffer> buffers;
    getCommandBuffers(imageIndex, buffers);
    submitInfo.commandBufferCount = static_cast<uint32_t>(buffers.size());
    submitInfo.pCommandBuffers = buffers.data();

    VkSemaphore signalSemaphores[] = { s_pData->renderFinishedSemaphores[s_pData->currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(PkGraphicsCore::GetDevice(), 1, &s_pData->inFlightFences[s_pData->currentFrame]);

    if (vkQueueSubmit(PkGraphicsCore::GetGraphicsQueue(), 1, &submitInfo, s_pData->inFlightFences[s_pData->currentFrame]) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { pkGraphicsSwapChain_GetSwapChain().swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(PkGraphicsCore::GetPresentQueue(), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || PkGraphicsCore::HasWindowBeenResized())
    {
        PkGraphicsCore::ResetWindowResizedFlag();
        recreateSwapChain();
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to present swap chain image!");
    }

    s_pData->currentFrame = (s_pData->currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

/*static*/ void PkGraphics::InitialiseGraphics(const char* pWindowName, PkGraphicsModelViewProjection& rModelViewProjection)
{
    s_pData = new PkGraphicsData();

    PkGraphicsCore::InitialiseGraphicsCore(pWindowName);

    pkGraphicsSwapChain_Create(PkGraphicsCore::GetInstance(), PkGraphicsCore::GetPhysicalDevice(), PkGraphicsCore::GetDevice(), PkGraphicsCore::GetMaxMsaaSampleCount());

    pkGraphicsRenderPassScene_Initialise(rModelViewProjection);
    pkGraphicsRenderPassImgui_Initialise();

    createSyncObjects();
}

/*static*/ void PkGraphics::CleanupGraphics()
{
    vkDeviceWaitIdle(PkGraphicsCore::GetDevice());

    destroySyncObjects();

    pkGraphicsRenderPassImgui_Cleanup();
    pkGraphicsRenderPassScene_Cleanup();

    pkGraphicsSwapChain_Destroy(PkGraphicsCore::GetDevice());

    PkGraphicsCore::CleanupGraphicsCore();

    delete s_pData;
}
