#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <array>
#include <cstdlib>
#include <cstring>
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
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 textureCoord;

    bool operator==(const Vertex& other) const
    {
        return pos == other.pos && color == other.color && textureCoord == other.textureCoord;
    }

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
    {
        // Return two attribute description structs, one for position and one for color.
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions {};
        // Read position attributes.
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);
        // Read color attributes.
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);
        // Read texture coordinate attributes.
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, textureCoord);

        return attributeDescriptions;
    }
};

namespace std
{
template<> struct hash<Vertex>
{
    size_t operator()(Vertex const& vertex) const
    {
        return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
            (hash<glm::vec2>()(vertex.textureCoord) << 1);
    }
};
}

struct UniformBufferObject
{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

/* ************************************************************************************************
 * Global Constants
 * ************************************************************************************************/
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::string MODEL_DIR = "models/viking_room.obj";
const std::string TEXTURE_DIR = "textures/viking_room.png";

/* ************************************************************************************************
 * Global Variables
 * ************************************************************************************************/
const std::vector<const char*> g_validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> g_deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
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
    void createColorResources();
    void createCommandBuffers();
    void createCommandPools();
    void createDepthResources();
    void createDescriptorSetlayout();
    void createDescriptorSets();
    void createDescriptorPool();
    void createFramebuffers();
    void createGraphicsPipeline();
    void createImageViews();
    void createIndexBuffer();
    void createInstance();
    void createLogicalDevice();
    void createRenderPass();
    void createSyncObjects();
    void createSurface();
    void createSwapchain();
    void createTextureImage();
    void createTextureImageView();
    void createTextureSampler();
    void createUniformBuffers();
    void createVertexBuffer();
    void destroySwapchain();
    void drawFrame();
    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t textureWidth, int32_t textureHeight,
        uint32_t mipLevels);
    void initWindow();
    void initVulkan();
    void loadModel();
    void mainLoop();
    void recreateSwapchain();
    void setupDebugMessenger();
    void transitionImageLayout(VkImage image, uint32_t mipLevels, VkFormat format, VkImageLayout oldLayout,
        VkImageLayout newLayout);
    void updateUniformBuffer(uint32_t imageIndex);

    // Static Functions ---------------------------------------------------------------------------/
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    /* ********************************************************************************************
     * Private Helper Functions
     * ********************************************************************************************/
    static std::vector<char> readFile(const std::string& filename);

    VkCommandBuffer beginSingleTimeCommands();
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    bool checkValidationLayerSupport();
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR choosePresentModeFormat(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
        VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples,
        VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
        VkImage& image, VkDeviceMemory& imageMemory);
    VkImageView createImageView(VkImage image, uint32_t mipLevels, VkFormat format, VkImageAspectFlags aspectFlags);
    VkShaderModule createShaderModule(const std::vector<char>& code);
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    VkFormat findDepthFormat();
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
        VkFormatFeatureFlags features);
    VkSampleCountFlagBits getMaxSampleCount();
    std::vector<const char*> getRequiredExtensions();
    bool hasStencilComponent(VkFormat format);
    bool isDeviceSuitable(VkPhysicalDevice device);
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device);
    void selectPhysicalDevice();

    /* ********************************************************************************************
     * Private Attributes
     * ********************************************************************************************/
    VkImage                         m_colorImage;
    VkDeviceMemory                  m_colorImageMemory;
    VkImageView                     m_colorImageView;
    std::vector<VkCommandBuffer>    m_commandBuffers;
    VkCommandPool                   m_commandPool;
    VkCommandPool                   m_commandPoolTransient;
    size_t                          m_currentFrame;
    VkDebugUtilsMessengerEXT        m_debugMessenger;
    VkImage                         m_depthImage;
    VkDeviceMemory                  m_depthImageMemory;
    VkImageView                     m_depthImageView;
    VkDescriptorPool                m_descriptorPool;
    VkDescriptorSetLayout           m_descriptorSetLayout;
    std::vector<VkDescriptorSet>    m_descriptorSets;
    VkDevice                        m_device;
    VkPipeline                      m_graphicsPipeline;
    VkQueue                         m_graphicsQueue;
    VkBuffer                        m_indexBuffer;
    VkDeviceMemory                  m_indexBufferMemory;
    std::vector<uint32_t>           m_indices;
    VkInstance                      m_instance;
    uint32_t                        m_mipLevels;
    VkSampleCountFlagBits           m_msaaSamples;
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
    VkImage                         m_textureImage;
    VkDeviceMemory                  m_textureImageMemory;
    VkImageView                     m_textureImageView;
    VkSampler                       m_textureSampler;
    std::vector<VkBuffer>           m_uniformBuffers;
    std::vector<VkDeviceMemory>     m_uniformBuffersMemory;
    std::vector<Vertex>             m_vertices;
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

