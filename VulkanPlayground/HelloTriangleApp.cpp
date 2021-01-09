#include "HelloTriangleApp.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <chrono>
#include <cstdint>
#include <fstream>
#include <set>
#include <unordered_map>

/* ************************************************************************************************
 * Global Functions
 * ************************************************************************************************/
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}

/*! ***********************************************************************************************
 * \class   HelloTriangleApplication
 * \author  Leon Vincii
 * \date    2020.11.20
 * ************************************************************************************************/
 /* ***********************************************************************************************
  * Public Ctor & Dtor
  * ***********************************************************************************************/
HelloTriangleApplication::HelloTriangleApplication() :
    m_commandBuffers            ()
  , m_commandPool               ()
  , m_commandPoolTransient      ()
  , m_currentFrame              (0)
  , m_debugMessenger            ()
  , m_depthImage                ()
  , m_depthImageMemory          ()
  , m_depthImageView            ()
  , m_descriptorPool            ()
  , m_descriptorSetLayout       ()
  , m_descriptorSets            ()
  , m_device                    ()
  , m_graphicsPipeline          ()
  , m_graphicsQueue             ()
  , m_indexBuffer               ()
  , m_indexBufferMemory         ()
  , m_indices                   ()
  , m_instance                  ()
  , m_physicalDevice            (VK_NULL_HANDLE)
  , m_pipelineLayout            ()
  , m_presentQueue              ()
  , m_renderPass                ()
  , m_surface                   ()
  , m_swapchain                 ()
  , m_swapchainExtent           ()
  , m_swapchainFramebuffers     ()
  , m_swapchainImages           ()
  , m_swapchainImageFormat      ()
  , m_swapchainImageViews       ()
  , m_textureImage              ()
  , m_textureImageMemory        ()
  , m_textureImageView          ()
  , m_textureSampler            ()
  , m_uniformBuffers            ()
  , m_uniformBuffersMemory      ()
  , m_vertices                  ()
  , m_vertexBuffer              ()
  , m_vertexBufferMemory        ()
  , m_window                    ()
    // Auxiliaries --------------------------------------------------------------------------------/
  , m_framebufferResized        (false)
    // Constants ----------------------------------------------------------------------------------/
  , m_MAX_FRAMES_IN_FLIGHT      (2)
    // Semaphores ---------------------------------------------------------------------------------/
  , m_imageAvailableSemaphores  ()
  , m_imageUsageFences          ()
  , m_cmdBufferExecFences       ()
  , m_renderFinishedSemaphores  ()
{}

/* ************************************************************************************************
 * Public Functions
 * ************************************************************************************************/
void HelloTriangleApplication::run()
{
    // Initialise GLFW window.
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

/* ************************************************************************************************
 * Private Functions
 * ************************************************************************************************/
void HelloTriangleApplication::cleanup()
{
    // Destroy swapchain.
    destroySwapchain();

    // Destroy descriptor set layouts.
    vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);

    // Destroy samplers.
    vkDestroySampler(m_device, m_textureSampler, nullptr);

    // Destroy image views, images and free their memory.
    vkDestroyImageView(m_device, m_textureImageView, nullptr);
    vkDestroyImage(m_device, m_textureImage, nullptr);
    vkFreeMemory(m_device, m_textureImageMemory, nullptr);

    // Destroy buffers and free their memory.
    vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
    vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);

    vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
    vkFreeMemory(m_device, m_indexBufferMemory, nullptr);

    // Destroy semaphores.
    for (size_t i = 0; i < m_MAX_FRAMES_IN_FLIGHT; ++ i)
    {
        vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(m_device, m_cmdBufferExecFences[i], nullptr);
    }

    // Destroy command pools.
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    vkDestroyCommandPool(m_device, m_commandPoolTransient, nullptr);

    // Destroy logical device.
    vkDestroyDevice(m_device, nullptr);

    // Destroy debug utils messenger.
    if (enableValidationLayers)
    {
        DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
    }

    // Destroy window surface.
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);

    // Destroy Vulkan instance.
    vkDestroyInstance(m_instance, nullptr);

    // Destroy window and terminate GLFW.
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void HelloTriangleApplication::createCommandBuffers()
{
    m_commandBuffers.resize(m_swapchainFramebuffers.size());

    VkCommandBufferAllocateInfo commandBufferAllocInfo {};
    commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocInfo.commandPool = m_commandPool;
    commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

    if (vkAllocateCommandBuffers(m_device, &commandBufferAllocInfo, m_commandBuffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate command buffers");
    }

    for (size_t i = 0; i < m_commandBuffers.size(); ++ i)
    {
        VkCommandBufferBeginInfo beginInfo {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;

        // Start command buffer recording.
        if (vkBeginCommandBuffer(m_commandBuffers[i], &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to begin recording command buffer");
        }

        // Begin render pass.
        std::array<VkClearValue, 2> clearValues {};
        clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        clearValues[1].depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo renderPassInfo {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_renderPass;
        renderPassInfo.framebuffer = m_swapchainFramebuffers[i];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = m_swapchainExtent;
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Bind graphics pipeline.
        vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

        // Bind the vertex buffer.
        VkBuffer vertexBuffers[] = { m_vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(m_commandBuffers[i], 0, 1, vertexBuffers, offsets);

        // Bind the index buffer.
        vkCmdBindIndexBuffer(m_commandBuffers[i], m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        // Bind the right descriptor set for each swapchain image to the descriptors in the shader.
        vkCmdBindDescriptorSets(
            m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
            &m_descriptorSets[i], 0, nullptr
        );

        // Draw using the command in the command buffer.
        vkCmdDrawIndexed(m_commandBuffers[i], static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);

        // End render pass.
        vkCmdEndRenderPass(m_commandBuffers[i]);

        // Finish command buffer recording.
        if (vkEndCommandBuffer(m_commandBuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to record command buffer");
        }
    }
}

void HelloTriangleApplication::createCommandPools()
{
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_physicalDevice);

    // Create normal command pool.
    VkCommandPoolCreateInfo poolInfo {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    poolInfo.flags = 0;

    if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create command pool");
    }

    // Create a separate transient command pool for short-lived command buffers (e.g., buffer copy commands).
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

    if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPoolTransient) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create transient command pool");
    }
}

void HelloTriangleApplication::createDepthResources()
{
    VkFormat depthFormat = findDepthFormat();

    // Create depth image.
    createImage(
        m_swapchainExtent.width, m_swapchainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_depthImage, m_depthImageMemory);

    // Create depth image view.
    m_depthImageView = createImageView(m_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void HelloTriangleApplication::createDescriptorSetlayout()
{
    // Describe the binding for uniform buffer object.
    VkDescriptorSetLayoutBinding uboLayoutBinding {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    // Describe the binding for combined image sampler.
    VkDescriptorSetLayoutBinding samplerLayoutBinding {};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutBinding.pImmutableSamplers = nullptr;

    // Create descriptor set layout object.
    std::array<VkDescriptorSetLayoutBinding, 2> layoutBindings = {
        uboLayoutBinding, samplerLayoutBinding
    };
    VkDescriptorSetLayoutCreateInfo layoutInfo {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings = layoutBindings.data();

    if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor set layout");
    }
}

void HelloTriangleApplication::createDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts(m_swapchainImages.size(), m_descriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(m_swapchainImages.size());
    allocInfo.pSetLayouts = descriptorSetLayouts.data();

    m_descriptorSets.resize(m_swapchainImages.size());
    if (vkAllocateDescriptorSets(m_device, &allocInfo, m_descriptorSets.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate descriptor sets");
    }

    // Update every descriptor within the descriptor sets.
    for (size_t i = 0; i < m_swapchainImages.size(); ++ i)
    {
        VkDescriptorBufferInfo bufferInfo {};
        bufferInfo.buffer = m_uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo {};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_textureImageView;
        imageInfo.sampler = m_textureSampler;

        std::array<VkWriteDescriptorSet, 2> writeDescriptors {};

        writeDescriptors[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptors[0].dstSet = m_descriptorSets[i];
        writeDescriptors[0].dstBinding = 0;
        writeDescriptors[0].dstArrayElement = 0;
        writeDescriptors[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptors[0].descriptorCount = 1;
        writeDescriptors[0].pBufferInfo = &bufferInfo;

        writeDescriptors[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptors[1].dstSet = m_descriptorSets[i];
        writeDescriptors[1].dstBinding = 1;
        writeDescriptors[1].dstArrayElement = 0;
        writeDescriptors[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptors[1].descriptorCount = 1;
        writeDescriptors[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(
            m_device, static_cast<uint32_t>(writeDescriptors.size()), writeDescriptors.data(), 0,
            nullptr
        );
    }
}

void HelloTriangleApplication::createDescriptorPool()
{
    // Describe which descriptor types the descriptor sets are going to contain and how many of them.
    std::array<VkDescriptorPoolSize, 2> poolSizes {};

    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(m_swapchainImages.size());

    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(m_swapchainImages.size());

    // Create descriptor pool.
    VkDescriptorPoolCreateInfo poolInfo {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(m_swapchainImages.size());

    if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor pool");
    }
}

void HelloTriangleApplication::createFramebuffers()
{
    m_swapchainFramebuffers.resize(m_swapchainImageViews.size());

    // Iterate image views and create framebuffers for each of them.
    for (size_t i = 0; i < m_swapchainImageViews.size(); ++ i)
    {
        std::array<VkImageView, 2> attachments = {
            m_swapchainImageViews[i],
            m_depthImageView
        };

        VkFramebufferCreateInfo framebufferInfo {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = m_swapchainExtent.width;
        framebufferInfo.height = m_swapchainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapchainFramebuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create framebuffer");
        }
    }
}

void HelloTriangleApplication::createGraphicsPipeline()
{
    auto vertShader = readFile("shaders/vert.spv");
    auto fragShader = readFile("shaders/frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShader);
    VkShaderModule fragShaderModule = createShaderModule(fragShader);

    // Create shader stages.
    VkPipelineShaderStageCreateInfo vertShaderStageInfo {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    // Vertex input.
    auto bindDescription = Vertex::getBindingDescription();
    auto attrDescriptions = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attrDescriptions.data();

    // Input assembly.
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo {};
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    // Viewports and scissors.
    VkViewport viewport {};
    viewport.x = 0.f;
    viewport.y = 0.f;
    viewport.width = static_cast<float>(m_swapchainExtent.width);
    viewport.height = static_cast<float>(m_swapchainExtent.height);
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;

    VkRect2D scissor {};
    scissor.offset = { 0, 0 };
    scissor.extent = m_swapchainExtent;

    VkPipelineViewportStateCreateInfo viewportStateInfo {};
    viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateInfo.viewportCount = 1;
    viewportStateInfo.pViewports = &viewport;
    viewportStateInfo.scissorCount = 1;
    viewportStateInfo.pScissors = &scissor;

    // Rasterizer.
    VkPipelineRasterizationStateCreateInfo rasterizerInfo {};
    rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerInfo.depthClampEnable = VK_FALSE;
    rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizerInfo.lineWidth = 1.f;
    rasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizerInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizerInfo.depthBiasEnable = VK_FALSE;
    rasterizerInfo.depthBiasConstantFactor = 0.f;
    rasterizerInfo.depthBiasClamp = 0.f;
    rasterizerInfo.depthBiasSlopeFactor = 0.f;

    // Multisampling.
    VkPipelineMultisampleStateCreateInfo multisamplingInfo {};
    multisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisamplingInfo.sampleShadingEnable = VK_FALSE;
    multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisamplingInfo.minSampleShading = 1.f;
    multisamplingInfo.pSampleMask = nullptr;
    multisamplingInfo.alphaToCoverageEnable = VK_FALSE;
    multisamplingInfo.alphaToOneEnable = VK_FALSE;

    // Depth and stencil.
    VkPipelineDepthStencilStateCreateInfo depthStencilInfo {};
    depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilInfo.depthTestEnable = VK_TRUE;
    depthStencilInfo.depthWriteEnable = VK_TRUE;
    depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilInfo.stencilTestEnable = VK_FALSE;

    // Color blending.
    VkPipelineColorBlendAttachmentState colorBlendAttachment {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlendInfo {};
    colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendInfo.logicOpEnable = VK_FALSE;
    colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendInfo.attachmentCount = 1;
    colorBlendInfo.pAttachments = &colorBlendAttachment;
    colorBlendInfo.blendConstants[0] = 0.f;
    colorBlendInfo.blendConstants[1] = 0.f;
    colorBlendInfo.blendConstants[2] = 0.f;
    colorBlendInfo.blendConstants[3] = 0.f;

    // Specify uniform values (pipeline layout).
    VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout");
    }

    // Actually create graphics pipeline.
    VkGraphicsPipelineCreateInfo pipelineInfo {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
    pipelineInfo.pViewportState = &viewportStateInfo;
    pipelineInfo.pRasterizationState = &rasterizerInfo;
    pipelineInfo.pMultisampleState = &multisamplingInfo;
    pipelineInfo.pDepthStencilState = &depthStencilInfo;
    pipelineInfo.pColorBlendState = &colorBlendInfo;
    pipelineInfo.pDynamicState = nullptr;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics pipeline");
    }

    // Clean up shader modules.
    vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
    vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
}

void HelloTriangleApplication::createImageViews()
{
    // Resize the image views to fit all of the images we'll be creating.
    m_swapchainImageViews.resize(m_swapchainImages.size());

    // Iterates all of the swapchain images.
    for (size_t i = 0; i < m_swapchainImageViews.size(); ++i)
    {
        m_swapchainImageViews[i] = createImageView(m_swapchainImages[i], m_swapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void HelloTriangleApplication::createIndexBuffer()
{
    // Create staging buffer (visible on CPU).
    VkDeviceSize bufferSize = sizeof(m_indices[0]) * m_indices.size();
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(
        bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingBufferMemory
    );

    // Map the staging buffer memory into CPU accessible memory.
    void* data;
    vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &data);

    // Copy the vertices to the (mapped) buffer memory.
    memcpy(data, m_indices.data(), static_cast<size_t>(bufferSize));

    // Unmap the staging buffer memory.
    vkUnmapMemory(m_device, stagingBufferMemory);

    // Create destination buffer (not visible on CPU).
    createBuffer(
        bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_indexBuffer, m_indexBufferMemory
    );

    // Copy buffer from staging buffer to destination buffer.
    copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);

    // Clean up staging buffer and its memory.
    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingBufferMemory, nullptr);
}

void HelloTriangleApplication::createInstance()
{
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available");
    }

    VkApplicationInfo appInfo {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan Playground";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(g_validationLayers.size());
        createInfo.ppEnabledLayerNames = g_validationLayers.data();
        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    auto extensions = getRequiredExtensions();

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create the instance");
    }
}

void HelloTriangleApplication::createLogicalDevice()
{
    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfoVec;
    std::set<uint32_t> uniqueQueueFamilies = {
        indices.graphicsFamily.value(), indices.presentFamily.value()
    };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        // Specify the queues to be used.
        VkDeviceQueueCreateInfo queueCreateInfo {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfoVec.push_back(queueCreateInfo);
    }

    // Specify the used device features.
    VkPhysicalDeviceFeatures deviceFeatures {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    // Create logical device.
    VkDeviceCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfoVec.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfoVec.size());
    createInfo.pEnabledFeatures = &deviceFeatures;
    // Enable required extensions.
    createInfo.enabledExtensionCount = static_cast<uint32_t>(g_deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = g_deviceExtensions.data();

    // These two fields are ignored in the latest Vulkan implamentation, they are set to be compatible
    // with older APIs.
    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(g_validationLayers.size());
        createInfo.ppEnabledLayerNames = g_validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    // Actually create the logical device.
    if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create logical device");
    }

    // Retrieve graphics queue handle from logical device and queue family.
    vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);
}

void HelloTriangleApplication::createRenderPass()
{
    VkAttachmentDescription colorAttachment {};
    colorAttachment.format = m_swapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment {};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

    VkSubpassDescription subpass {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render pass");
    }
}

void HelloTriangleApplication::createSyncObjects()
{
    m_imageAvailableSemaphores.resize(m_MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(m_MAX_FRAMES_IN_FLIGHT);
    m_imageUsageFences.resize(m_swapchainImages.size(), VK_NULL_HANDLE);
    m_cmdBufferExecFences.resize(m_MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < m_MAX_FRAMES_IN_FLIGHT; ++ i)
    {
        if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(m_device, &fenceInfo, nullptr, &m_cmdBufferExecFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create sync objects for a frame");
        }
    }
}

void HelloTriangleApplication::createSurface()
{
    if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create window surface!");
    }
}

void HelloTriangleApplication::createSwapchain()
{
    SwapchainSupportDetails swapchainSupport = querySwapchainSupport(m_physicalDevice);
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.formats);
    VkPresentModeKHR presentMode = choosePresentModeFormat(swapchainSupport.presentModes);
    m_swapchainExtent = chooseSwapExtent(swapchainSupport.capabilities);
    m_swapchainImageFormat = surfaceFormat.format;

    // The number of images in the swap chain. Set it at least one image more than the min to make
    // sure that we don't need to wait for the internal operations to finish before we can acquire
    // another image to render to. We also make sure that it doesn't exceed the max limit.
    uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
    if (swapchainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapchainSupport.capabilities.maxImageCount)
    {
        imageCount = swapchainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = m_swapchainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // Handle swap chain images that will be used across multiple queue families.
    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);
    uint32_t graphicPresentIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };
    if (indices.graphicsFamily != indices.presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = graphicPresentIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    // Specify a pre-hand transform (no transform in this case).
    createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
    // Specify if the alpha channel should be used for blending with other windows in the system.
    // Ignore the alpha channel here.
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    // For now assume that only one swap chain will ever be created.
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    // Create the swap chain.
    if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create swap chain");
    }

    // Retrieve swap chain image handles.
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);
    m_swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, m_swapchainImages.data());
}

void HelloTriangleApplication::createTextureImage()
{
    int textureWidth, textureHeight, textureChannels;
    stbi_uc* pixels = stbi_load(TEXTURE_DIR.c_str(), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = static_cast<uint64_t>(textureWidth) * static_cast<uint64_t>(textureHeight) * 4;

    if (!pixels)
    {
        throw std::runtime_error("failed to load texture image source");
    }

    // Create a staging buffer for the texture image.
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(
        imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingBufferMemory
    );

    // Copy the pixel value to the staging buffer.
    void* data;
    vkMapMemory(m_device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(m_device, stagingBufferMemory);

    // Clean up the original pixel array.
    stbi_image_free(pixels);

    // Create texture image object on GPU and bind with its allocated memory.
    createImage(
        textureWidth, textureHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_textureImage, m_textureImageMemory
    );

    // Copy the staging buffer to the texture image, which involves two steps:
    // 1. transition the texture image to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    // 2. execute the buffer to image copy operation.
    transitionImageLayout(
        m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );
    copyBufferToImage(
        stagingBuffer, m_textureImage, static_cast<uint32_t>(textureWidth),
        static_cast<uint32_t>(textureHeight)
    );

    // Transition the image into VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL for shader access.
    transitionImageLayout(
        m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );

    // Clean up staging buffer and its memory.
    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingBufferMemory, nullptr);
}

void HelloTriangleApplication::createTextureImageView()
{
    m_textureImageView = createImageView(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

void HelloTriangleApplication::createTextureSampler()
{
    VkPhysicalDeviceProperties properties {};
    vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo {};
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
    samplerInfo.mipLodBias = 0.f;
    samplerInfo.minLod = 0.f;
    samplerInfo.maxLod = 0.f;

    if (vkCreateSampler(m_device, &samplerInfo, nullptr, &m_textureSampler) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture sampler");
    }
}

void HelloTriangleApplication::createUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    m_uniformBuffers.resize(m_swapchainImages.size());
    m_uniformBuffersMemory.resize(m_swapchainImages.size());

    // Create a uniform buffer for each swapchain image.
    for (size_t i = 0; i < m_swapchainImages.size(); ++ i)
    {
        createBuffer(
            bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_uniformBuffers[i], m_uniformBuffersMemory[i]
        );
    }
}

void HelloTriangleApplication::createVertexBuffer()
{
    // Create staging buffer (visible on CPU).
    VkDeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(
        bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingBufferMemory
    );

    // Map the staging buffer memory into CPU accessible memory.
    void* data;
    vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &data);

    // Copy the vertices to the (mapped) buffer memory.
    memcpy(data, m_vertices.data(), static_cast<size_t>(bufferSize));

    // Unmap the staging buffer memory.
    vkUnmapMemory(m_device, stagingBufferMemory);

    // Create destination buffer (not visible on CPU).
    createBuffer(
        bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertexBuffer, m_vertexBufferMemory
    );

    // Copy buffer from staging buffer to destination buffer.
    copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

    // Clean up staging buffer and its memory.
    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingBufferMemory, nullptr);
}

void HelloTriangleApplication::destroySwapchain()
{
    // Destroy depth image, image view and memory.
    vkDestroyImageView(m_device, m_depthImageView, nullptr);
    vkDestroyImage(m_device, m_depthImage, nullptr);
    vkFreeMemory(m_device, m_depthImageMemory, nullptr);

    // Destroy framebuffers.
    for (auto framebuffer : m_swapchainFramebuffers)
    {
        vkDestroyFramebuffer(m_device, framebuffer, nullptr);
    }

    // Free command buffers.
    vkFreeCommandBuffers(m_device, m_commandPool, static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());

    // Destroy uniform buffers.
    for (size_t i = 0; i < m_swapchainImages.size(); ++ i)
    {
        vkDestroyBuffer(m_device, m_uniformBuffers[i], nullptr);
        vkFreeMemory(m_device, m_uniformBuffersMemory[i], nullptr);
    }

    // Destroy descriptor pool.
    vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);

    // Destroy pipeline.
    vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);

    // Destroy pipeline layout.
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);

    // Destroy render pass.
    vkDestroyRenderPass(m_device, m_renderPass, nullptr);

    // Destroy image views.
    for (auto imageView : m_swapchainImageViews)
    {
        vkDestroyImageView(m_device, imageView, nullptr);
    }

    // Destroy swapchain.
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
}

void HelloTriangleApplication::drawFrame()
{
    // Wait for the frame to be finished.
    vkWaitForFences(m_device, 1, &m_cmdBufferExecFences[m_currentFrame], VK_TRUE, UINT64_MAX);

    // Acquire an image from the swapchain.
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        m_device, m_swapchain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE,
        &imageIndex
    );

    // Check if swapchain is out-of-date.
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapchain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("failed to acquire image from swapchain");
    }

    // Check if this image is used by another (previous) frame; if so, wait for its fence.
    if (m_imageUsageFences[imageIndex] != VK_NULL_HANDLE)
    {
        vkWaitForFences(m_device, 1, &m_imageUsageFences[imageIndex], VK_TRUE, UINT64_MAX);
    }

    // Mark the image as being in use by this frame.
    m_imageUsageFences[imageIndex] = m_cmdBufferExecFences[m_currentFrame];

    // Update uniform buffer.
    updateUniformBuffer(imageIndex);

    // Prepare to submit command buffer to the queue.
    VkSubmitInfo submitInfo {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[imageIndex];

    VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    // Reset the fence before using it.
    vkResetFences(m_device, 1, &m_cmdBufferExecFences[m_currentFrame]);

    // Actually submit the command buffer.
    if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_cmdBufferExecFences[m_currentFrame]) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to submit draw command buffer");
    }

    // Return the image to the swapchain for presentation.
    VkPresentInfoKHR presentInfo {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = { m_swapchain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    presentInfo.pResults = nullptr;

    // Submit the request to present an image to the swapchain.
    result = vkQueuePresentKHR(m_presentQueue, &presentInfo);

    // Check if swapchain is out-of-date OR suboptimal OR framebuffer is resized.
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebufferResized)
    {
        m_framebufferResized = false;
        recreateSwapchain();
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to present image from swapchain");
    }

    // Advance current frame.
    m_currentFrame = (m_currentFrame + 1) % m_MAX_FRAMES_IN_FLIGHT;
}

void HelloTriangleApplication::initWindow()
{
    // Initialise GLFW.
    glfwInit();

    // Do not create with OpenGL context.
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // Create a window.
    m_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

    // Hook window resize callback.
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, &framebufferResizeCallback);
}

void HelloTriangleApplication::initVulkan()
{
    createInstance();
    setupDebugMessenger();
    createSurface();
    selectPhysicalDevice();
    createLogicalDevice();
    createSwapchain();
    createImageViews();
    createRenderPass();
    createDescriptorSetlayout();
    createGraphicsPipeline();
    createCommandPools();
    createDepthResources();
    createFramebuffers();
    createTextureImage();
    createTextureImageView();
    createTextureSampler();
    loadModel();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createSyncObjects();
}

void HelloTriangleApplication::loadModel()
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector < tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_DIR.c_str()))
    {
        throw std::runtime_error(warn + err);
    }

    for (const auto& shape : shapes)
    {
        std::unordered_map<Vertex, uint32_t> uniqueVertices {};

        for (const auto& index : shape.mesh.indices)
        {
            Vertex vertex {};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };
            vertex.textureCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            if (uniqueVertices.count(vertex) == 0)
            {
                uniqueVertices[vertex] = static_cast<uint32_t>(m_vertices.size());
                m_vertices.push_back(vertex);
            }

            m_indices.push_back(uniqueVertices[vertex]);
        }
    }
}

void HelloTriangleApplication::mainLoop()
{
    while (!glfwWindowShouldClose(m_window))
    {
        glfwPollEvents();
        drawFrame();
    }

    // Wait for the device to finish its work before going to next step (cleanup etc.).
    vkDeviceWaitIdle(m_device);
}

void HelloTriangleApplication::recreateSwapchain()
{
    // Pause swapchain recreation when window is minimised.
    int width = 0, height = 0;
    glfwGetFramebufferSize(m_window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwWaitEvents();
        glfwGetFramebufferSize(m_window, &width, &height);
    }

    // Wait for the device to finish its work before doing anything else.
    vkDeviceWaitIdle(m_device);

    // Clean up the old version of swapchain and its related dependencies.
    destroySwapchain();

    // Recreate swapchain and its dependencies.
    createSwapchain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createDepthResources();
    createFramebuffers();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
}

void HelloTriangleApplication::setupDebugMessenger()
{
    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to set up debug messenger");
    }
}

void HelloTriangleApplication::transitionImageLayout(VkImage image, VkFormat format,
    VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkAccessFlags srcAccessMask, dstAccessMask;
    VkPipelineStageFlags srcStage, dstStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        srcAccessMask = 0;
        dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        throw std::invalid_argument("unsupported layout transition");
    }

    VkImageMemoryBarrier barrier {};
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
    barrier.srcAccessMask = srcAccessMask;
    barrier.dstAccessMask = dstAccessMask;

    vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    endSingleTimeCommands(commandBuffer);
}

void HelloTriangleApplication::updateUniformBuffer(uint32_t imageIndex)
{
    /***
     * This function will generate a new transformation every frame to make the geometry spin around
     * 90 degrees per second regardless of frame rate.
     ***/

    // Mark start time.
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo {};
    // Define model transformation in UBO.
    ubo.model = glm::rotate(glm::mat4(1.f), deltaTime * glm::radians(90.f), glm::vec3(0.f, 0.f, 1.f));
    // Define view transformation in UBO.
    ubo.view = glm::lookAt(glm::vec3(2.f, 2.f, 2.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 0.f, 1.f));
    // Define proj transformation in UBO.
    ubo.proj = glm::perspective(
        glm::radians(45.f),
        m_swapchainExtent.width / static_cast<float>(m_swapchainExtent.height),
        .1f, 10.f
    );
    // Flip the Y coordinate as GLM was originally designed for OpenGL, where the Y coordinate is inverted.
    ubo.proj[1][1] *= -1;

    // Copy the uniform buffer object to the uniform buffer.
    void* data;
    vkMapMemory(m_device, m_uniformBuffersMemory[imageIndex], 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(m_device, m_uniformBuffersMemory[imageIndex]);
}

void HelloTriangleApplication::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
    app->m_framebufferResized = true;
}

/* ************************************************************************************************
 * Private Helper Functions
 * ************************************************************************************************/
std::vector<char> HelloTriangleApplication::readFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
        throw std::runtime_error("failed to open file");
    }
    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

VkCommandBuffer HelloTriangleApplication::beginSingleTimeCommands()
{
    // Create command buffers for copying buffer.
    VkCommandBufferAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPoolTransient;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

    // Start recording command buffer.
    VkCommandBufferBeginInfo beginInfo {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

/* ************************************************************************************************
 * Private Helper Functions
 * ************************************************************************************************/
bool HelloTriangleApplication::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(g_deviceExtensions.begin(), g_deviceExtensions.end());

    // Check if all required extensions are available.
    for (const auto& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

bool HelloTriangleApplication::checkValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (auto layerName : g_validationLayers)
    {
        bool layerFound = false;
        for (const auto& layerProperties : availableLayers)
        {
            if (std::strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }
        if (!layerFound) return false;
    }
    return true;
}

VkSurfaceFormatKHR HelloTriangleApplication::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    _ASSERT(!availableFormats.empty());

    // Go through the available list and see if the preferred format and color space is available.
    for (const auto& format : availableFormats)
    {
        // The preferred format and color space.
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return format;
        }
    }
    // Otherwise return the first available format.
    return availableFormats[0];
}

VkPresentModeKHR HelloTriangleApplication::choosePresentModeFormat(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    _ASSERT(!availablePresentModes.empty());

    // Go through the available present mode and see if the preferred mode is available.
    for (const auto& presentMode : availablePresentModes)
    {
        if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
        {
            return presentMode;
        }
    }
    // The FIFO mode is guaranteed to be available.
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D HelloTriangleApplication::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        // If Vulkan fills in the swap extent for us, return the extent.
        return capabilities.currentExtent;
    }
    else
    {
        // If Vulkan doesn't fill in the swap extent for us, then we need to query the GLFW buffer
        // size to get the resolution of the window in pixels.
        int width, height;
        glfwGetFramebufferSize(m_window, &width, &height);
        VkExtent2D extent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height),
        };
        extent.width = std::max(
            capabilities.minImageExtent.width,
            std::min(capabilities.maxImageExtent.width, extent.width)
        );
        extent.height = std::max(
            capabilities.minImageExtent.height,
            std::min(capabilities.maxImageExtent.height, extent.height)
        );
        return extent;
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL HelloTriangleApplication::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    std::cerr << "Validation Layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

void HelloTriangleApplication::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    // Begin a single time command buffer recording.
    auto commandBuffer = beginSingleTimeCommands();

    // Specify copy region.
    VkBufferCopy copyRegion {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;

    // Issue actual copy command.
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    // Finish a single time command buffer recording, execute and clean up.
    endSingleTimeCommands(commandBuffer);
}

void HelloTriangleApplication::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferImageCopy region {};
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

    endSingleTimeCommands(commandBuffer);
}

void HelloTriangleApplication::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    VkBufferCreateInfo bufferInfo {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create buffer");
    }

    // Query vertex buffer memory requirements.
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, buffer, &memRequirements);

    // Allocate memory for the buffer.
    VkMemoryAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(
        memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate buffer memory");
    }

    // Bind the memory to the buffer.
    vkBindBufferMemory(m_device, buffer, bufferMemory, 0);
}

void HelloTriangleApplication::createImage(uint32_t width, uint32_t height, VkFormat format,
    VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image,
    VkDeviceMemory& imageMemory)
{
    // Create image object.
    VkImageCreateInfo imageInfo {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(width);
    imageInfo.extent.height = static_cast<uint32_t>(height);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0;

    if (vkCreateImage(m_device, &imageInfo, nullptr, &image) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture image");
    }

    // Allocate memory for the image object.
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate texture image memory");
    }

    // Bind the image object with its allocated memory.
    vkBindImageMemory(m_device, image, imageMemory, 0);
}

VkImageView HelloTriangleApplication::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = image;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;

    // Specify the purpose of the image and which part of the image should be accessed.
    // For this case, the images will be used as colour targets without any mipmaapping levels
    // nor mutiple layers.
    createInfo.subresourceRange.aspectMask = aspectFlags;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    // Create image view.
    VkImageView imageView;
    if (vkCreateImageView(m_device, &createInfo, nullptr, &imageView) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture image view");
    }

    return imageView;
}

VkShaderModule HelloTriangleApplication::createShaderModule(const std::vector<char>& code)
{
    VkShaderModuleCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create shader module");
    }
    return shaderModule;
}

void HelloTriangleApplication::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
    // End recording command buffer.
    vkEndCommandBuffer(commandBuffer);

    // Submit the command buffer.
    VkSubmitInfo submitInfo {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

    // Wait for the execution to complete.
    vkQueueWaitIdle(m_graphicsQueue);

    // Free command buffer.
    vkFreeCommandBuffers(m_device, m_commandPoolTransient, 1, &commandBuffer);
}

VkFormat HelloTriangleApplication::findDepthFormat()
{
    return findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

uint32_t HelloTriangleApplication::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    /***
     * Graphics cards offer different types of memory to allocate from. Each type of memory varies
     * in terms of allowed operations and performance characteristics. We need to combine the
     * requirements of the buffer and our application requirements to find the right type of memory
     * to use.
     ***/

    // First we need to query info about the available types of memory.
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

    // Find a memory type that is suitable for the buffer.
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++ i)
    {
        /***
         * VkMemoryRequirements::memoryTypeBits (typeFilter) is a bitmask and contains one bit set
         * for every supported memory type for the resource. Bit i is set if and only if the memory
         * type i in the VkPhysicalDeviceMemoryProperties structure for the physical device is
         * supported for the resource (vertices in this case).
         ***/

        // The bits set in typeFilter indicates the indices of the memory types that are supported
        // for the resource in memProperties.
        if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("failed to find a suitable memory type");
}

HelloTriangleApplication::QueueFamilyIndices HelloTriangleApplication::findQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices {};

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    // Find at least one queue family that supports VK_QUEUE_GRAPHICS_BIT.
    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        // Find a queue family that supports drawing (graphics queue).
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }

        // Find a queue family that supports presentation to the window surface (present queue).
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);
        if (presentSupport)
        {
            indices.presentFamily = i;
        }

        if (indices.isComplete())
        {
            break;
        }

        ++ i;
    }

    return indices;
}

VkFormat HelloTriangleApplication::findSupportedFormat(const std::vector<VkFormat>& candidates,
    VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
        {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }
    throw std::runtime_error("failed to find supported format");
}

std::vector<const char*> HelloTriangleApplication::getRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    // Wrap the extension list into a vector.
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    // Add debug utils extension if validation layers are enabled.
    if (enableValidationLayers)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

bool HelloTriangleApplication::hasStencilComponent(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

bool HelloTriangleApplication::isDeviceSuitable(VkPhysicalDevice device)
{
    // Check queue family.
    if (!findQueueFamilies(device).isComplete()) { return false; }

    // Check device extension support.
    if (!checkDeviceExtensionSupport(device)) { return false; }

    // Check swap chain support. For this application it needs at least one supported image format
    // and one supported presentation mode.
    bool swapChainAdequate = false;
    SwapchainSupportDetails swapChainDetails = querySwapchainSupport(device);
    if (swapChainDetails.formats.empty() || swapChainDetails.presentModes.empty()) { return false; }

    // Check device features support.
    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
    if (!supportedFeatures.samplerAnisotropy) { return false; }

    return true;
}

void HelloTriangleApplication::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

HelloTriangleApplication::SwapchainSupportDetails HelloTriangleApplication::querySwapchainSupport(VkPhysicalDevice device)
{
    SwapchainSupportDetails details;

    // Query the basic surface capabilities.
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

    // Query the supported surface formats.
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);
    if (formatCount != 0)
    {
        // Make sure the vector is resized to hold all available formats.
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
    }

    // Query the supported presentation modes.
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);
    if (presentModeCount != 0)
    {
        // Make sure the vector is resized to hold all available formats.
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

void HelloTriangleApplication::selectPhysicalDevice()
{
    // List all physical devices.
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

    if (deviceCount == 0)
    {
        throw std::runtime_error("failed to find GPUs with Vulkan support");
    }

    // Store all physical devices.
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    // Check if any device is suitable.
    for (const auto& device : devices)
    {
        if (isDeviceSuitable(device))
        {
            // Only pick the first suitable device.
            m_physicalDevice = device;
            break;
        }
    }

    if (m_physicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("failed to find a suitable GPU");
    }
}
