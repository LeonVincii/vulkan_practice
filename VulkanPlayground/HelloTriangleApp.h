#pragma once

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#include <array>
#include <cstdlib>
#include <cstring>
#include <glm/glm.hpp>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <vector>

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

/* ************************************************************************************************
 * Global Structs
 * ************************************************************************************************/
struct Vertex
{
    glm::vec2 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
    {
        // Return two attribute description structs, one for position and one for color.
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions {};
        // Read position attributes.
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);
        // Read color attributes.
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        return attributeDescriptions;
    }
};

/* ************************************************************************************************
 * Global Constants
 * ************************************************************************************************/
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

/* ************************************************************************************************
 * Global Variables
 * ************************************************************************************************/
const std::vector<const char*> g_validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> g_deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const std::vector<Vertex> g_vertices = {
    { {  .0f, -.5f }, { 1.f, .0f, .0f } },
    { {  .5f,  .5f }, { .0f, 1.f, .0f } },
    { { -.5f,  .5f }, { .0f, .0f, 1.f } }
};

/* ************************************************************************************************
 * Global Functions
 * ************************************************************************************************/
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator);

/*! ***********************************************************************************************
 * \class   HelloTriangleApplication
 * \author  Leon Vincii
 * \date    2020.11.20
 * ************************************************************************************************/
class HelloTriangleApplication
{
public:
    /* ********************************************************************************************
     * Public Ctor & Dtor
     * ********************************************************************************************/
    HelloTriangleApplication();

    /* ********************************************************************************************
     * Public Functions
     * ********************************************************************************************/
    void run();

private:
    /* ********************************************************************************************
     * Private Structs
     * ********************************************************************************************/
    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete()
        {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    struct SwapchainSupportDetails
    {
        VkSurfaceCapabilitiesKHR        capabilities {};
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR>   presentModes;
    };

    /* ********************************************************************************************
     * Private Functions
     * ********************************************************************************************/
    void cleanup();
    void createCommandBuffers();
    void createCommandPools();
    void createFramebuffers();
    void createGraphicsPipeline();
    void createImageViews();
    void createInstance();
    void createLogicalDevice();
    void createRenderPass();
    void createSyncObjects();
    void createSurface();
    void createSwapchain();
    void createVertexBuffer();
    void destroySwapchain();
    void drawFrame();
    void initWindow();
    void initVulkan();
    void mainLoop();
    void recreateSwapchain();
    void setupDebugMessenger();

    // Static Functions ---------------------------------------------------------------------------/
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    /* ********************************************************************************************
     * Private Helper Functions
     * ********************************************************************************************/
    static std::vector<char> readFile(const std::string& filename);

    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    bool checkValidationLayerSupport();
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR choosePresentModeFormat(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
        VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    VkShaderModule createShaderModule(const std::vector<char>& code);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    std::vector<const char*> getRequiredExtensions();
    bool isDeviceSuitable(VkPhysicalDevice device);
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device);
    void selectPhysicalDevice();

    /* ********************************************************************************************
     * Private Attributes
     * ********************************************************************************************/
    std::vector<VkCommandBuffer>    m_commandBuffers;
    VkCommandPool                   m_commandPool;
    VkCommandPool                   m_commandPoolTransient;
    size_t                          m_currentFrame;
    VkDebugUtilsMessengerEXT        m_debugMessenger;
    VkDevice                        m_device;
    VkPipeline                      m_graphicsPipeline;
    VkQueue                         m_graphicsQueue;
    VkInstance                      m_instance;
    VkPhysicalDevice                m_physicalDevice;
    VkPipelineLayout                m_pipelineLayout;
    VkQueue                         m_presentQueue;
    VkRenderPass                    m_renderPass;
    VkSurfaceKHR                    m_surface;
    VkSwapchainKHR                  m_swapchain;
    VkExtent2D                      m_swapchainExtent;
    std::vector<VkFramebuffer>      m_swapchainFramebuffers;
    std::vector<VkImage>            m_swapchainImages;
    VkFormat                        m_swapchainImageFormat;
    std::vector<VkImageView>        m_swapchainImageViews;
    VkBuffer                        m_vertexBuffer;
    VkDeviceMemory                  m_vertexBufferMemory;
    GLFWwindow*                     m_window;

    // Auxiliaries --------------------------------------------------------------------------------/
    bool                            m_framebufferResized;

    // Constants ----------------------------------------------------------------------------------/
    const int                       m_MAX_FRAMES_IN_FLIGHT;

    // Synchronisation ----------------------------------------------------------------------------/
    std::vector<VkSemaphore>        m_imageAvailableSemaphores;
    std::vector<VkFence>            m_imageUsageFences;
    std::vector<VkFence>            m_cmdBufferExecFences;
    std::vector<VkSemaphore>        m_renderFinishedSemaphores;
};

