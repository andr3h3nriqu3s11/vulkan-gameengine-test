#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>
#include <array>
#include <cstring>
#include <optional>
#include <vulkan/vulkan.h>
#include <algorithm>
#include <GLFW/glfw3.h>

#define MAX_FRAMES_IN_FLIGHT 2

struct UniformBufferObject
{
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::mat4 model;
};

struct ModelBuffer {
    glm::mat4 model;
};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

uint32_t findMemoryType(VkPhysicalDevice phyDevice, uint32_t typeFilter, VkMemoryPropertyFlags props);
void createBuffer(VkDevice device, VkPhysicalDevice phyDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory);
void copyBuffer(VkDevice device, VkCommandPool pool, VkQueue queue, VkBuffer source, VkBuffer dstBuffer, VkDeviceSize size);
void createImage(VkDevice device, VkPhysicalDevice phyDevice, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags mempProps, VkImage image, VkDeviceMemory imageMemory);
VkCommandBuffer beginCommands(VkDevice device, VkCommandPool commandPool, VkCommandBufferUsageFlags flags);
VkCommandBuffer beginSingleCommands(VkDevice device, VkCommandPool commandPool);
void endSigleTimeCommands(VkQueue graphicsQueue, VkDevice device, VkCommandPool pool, VkCommandBuffer commandBuffer);
void transitionImageLayout(VkQueue queue, VkDevice device, VkCommandPool pool, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
void copyBufferToImage(VkDevice device, VkCommandPool pool, VkQueue queue, uint32_t width, uint32_t height, VkBuffer buffer, VkImage image);
bool hasStencilComponent(VkFormat format);
std::vector<const char *> getRequiredExtensions(bool enableValidationLayers);
bool checkValidationLayerSupport(std::vector<const char *> validationLayers);
std::string printVec3(glm::vec3);

namespace UniverseGen
{
    VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags flags);
}

class UniverseEngine;

class MImage {
public:
    MImage();
    MImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags mempProps, VkImageAspectFlags flags);
    void clean(VkDevice device);
    VkImage getImage(void);
    void create(VkDevice device, VkPhysicalDevice phyDevice);
    void changeLayout(VkDevice device, VkQueue queue, VkCommandPool pool, VkImageLayout newLayout);
    void createImageView(VkDevice device);
    VkImageView getImageView();

private:
    VkImage image;
    VkDeviceMemory imageMemory;
    VkImageView imageView;

    uint32_t width;
    uint32_t height;
    VkFormat format;
    VkImageTiling tiling;
    VkImageUsageFlags usage;
    VkMemoryPropertyFlags memProps;
    VkImageLayout lastLayout;
    VkImageAspectFlags viewFlags;
};

class MSampler {
public:
    MSampler();
    VkSampler getSampler(void);
    void create(VkPhysicalDevice phyDevice, VkDevice device);
    void clean(VkDevice device);

private:
    VkSampler sampler;
};

struct Vertex {
    alignas(16) glm::vec3 pos;
    alignas(16) glm::vec3 color;
    alignas(16) glm::vec2 texCoord;
    alignas(16) float colorOn;
    alignas(16) uint32_t gameObjId;

    //How to read the data
    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription desc;
        desc.binding = 0;
        desc.stride = sizeof(Vertex);
        desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return desc;
    }

    //what the data means
    static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescription()
    {
        std::array<VkVertexInputAttributeDescription, 5> array;

        array[0].binding = 0;
        array[0].location = 0;
        array[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        array[0].offset = offsetof(Vertex, pos);

        array[1].binding = 0;
        array[1].location = 1;
        array[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        array[1].offset = offsetof(Vertex, color);

        array[2].binding = 0;
        array[2].location = 2;
        array[2].format = VK_FORMAT_R32G32_SFLOAT;
        array[2].offset = offsetof(Vertex, texCoord);

        array[3].binding = 0;
        array[3].location = 3;
        array[3].format = VK_FORMAT_R32_SFLOAT;
        array[3].offset = offsetof(Vertex, color);

        array[4].binding = 0;
        array[4].location = 4;
        array[4].format = VK_FORMAT_R32_UINT;
        array[4].offset = offsetof(Vertex, gameObjId);

        return array;
    }
};

struct Mesh {
    std::vector<Vertex> v;
    std::vector<uint32_t> i;
};

class GameObject {
public:
GameObject();
    GameObject(UniverseEngine * e);
    GameObject(UniverseEngine * e, std::vector<Vertex> vertecies, std::vector<uint32_t> indicies, glm::vec3 pos);
    //Basic method that will add the accelaration the speed and the speed to the pos
    void tick(void);
    //Basic method that will add a with accelaration then add the accelaration the speed and the speed to the pos
    void tick(glm::vec3 a);

    void updatePos(glm::vec3 pos);
    void updateVec(glm::vec3 vec);
    void updateAcc(glm::vec3 acc);
    void updateRot(glm::mat4 rot);
    
    glm::vec3 getPos();
    glm::vec3 getVec();
    glm::vec3 getAcc();
    Mesh getMesh();
    bool hasMeshChanged();
    void setId(uint32_t id);

protected:
    //Position
    glm::vec3 pos;
    //velocity
    glm::vec3 vec;
    //accelaration
    glm::vec3 acc;

    glm::mat4 rot;

    bool meshChanged;

    UniverseEngine * e;

    uint32_t id;
    void updateVerticeId();
    //protected:
    std::vector<Vertex> vertecies;
    std::vector<uint32_t> indicies;
};

class PaneObject : public GameObject {
public:
    PaneObject();
    PaneObject(UniverseEngine * e, glm::vec3 pos, glm::vec2 vec, glm::vec3 color);
    PaneObject(UniverseEngine * e, glm::vec3 pos, glm::vec2 vec, glm::vec3 color, glm::mat4 rot);
};

/*class GraphicPipeline {
    public: 
        GraphicPipeline(UniverseEngine * en);
        void create(void);
        void cleanup(void);
    private:
        bool created;
};*/

class Shader {
private:
    VkShaderModule module;
    VkShaderStageFlagBits flags;
    std::string path;
    UniverseEngine *en;

public:
    Shader();
    Shader(UniverseEngine *en, std::string path, VkShaderStageFlagBits flags);
    VkPipelineShaderStageCreateInfo getPipelineCreateInfo();
    void destroyModule();
};

//Interface
/*class Descriptor {
    protected: 
        size_t id;
        VkDescriptorType type;
        VkShaderStageFlags flags;
        UniverseEngine *en;

        //For uniform buffer
        std::vector<VkBuffer> uniformBuffers;
        std::vector<VkDeviceMemory> uniformBuffersMemory;
    public:
        VkDescriptorSetLayoutBinding virtual getDescriptorSetLayoutBinding() = 0;
        void virtual preSwapChainCreate(size_t swapChainSize) = 0;
        void virtual cleanUp() = 0;
        VkDescriptorType getType();
        VkWriteDescriptorSet virtual getWriterDescriptorSet(size_t index) = 0;
        void setId(size_t id);
}; */

//TODO: maybe add generics
class UniformBuffer {
    private:
        size_t id;
        VkShaderStageFlags flags;
        UniverseEngine *en;
        std::vector<VkBuffer> uniformBuffers;
        std::vector<VkDeviceMemory> uniformBuffersMemory;
    public:
        UniformBuffer();
        UniformBuffer(UniverseEngine *en, VkShaderStageFlags flags);
        VkWriteDescriptorSet getWriterDescriptorSet();
        //VkDescriptorBufferInfo getWriterDescriptorBufferInfo(uint32_t index);

        VkBuffer getBuffer(uint32_t index);
        VkDeviceSize getSize();

        VkDescriptorSetLayoutBinding getDescriptorSetLayoutBinding();
        void preSwapChainCreate(size_t swapChainSize);
        void cleanUp();
        VkDescriptorType getType();
        void setId(size_t);
        size_t getId();
        // TODO: maybe change
        void updateUniformBuffer(uint32_t index, UniformBufferObject obj);
};
template <typename T>
class ShaderStorageBuffer {
    private:
        size_t id;
        VkShaderStageFlags flags;
        UniverseEngine *en;
        std::vector<VkBuffer> ssBuffers;
        std::vector<VkDeviceMemory> ssBuffersMemory;
        VkDeviceSize bufferSize;
    public:
        ShaderStorageBuffer<T>();
        ShaderStorageBuffer<T>(UniverseEngine *en, VkShaderStageFlags flags);
        VkWriteDescriptorSet getWriterDescriptorSet();
        //VkDescriptorBufferInfo getWriterDescriptorBufferInfo(uint32_t index);

        VkBuffer getBuffer(uint32_t index);
        VkDeviceSize getSize();
        
        VkDescriptorSetLayoutBinding getDescriptorSetLayoutBinding();
        void preSwapChainCreate(size_t swapChainSize, size_t gameobjs);
        void cleanUp();
        VkDescriptorType getType();
        void setId(size_t);
        size_t getId();
        void updateBuffer(uint32_t index, std::vector<T> obj);
};

/*class ImageDescriptor : public Descriptor {
    private:
        VkSampler sampler;
};*/

class UniverseEngine {
public:
    UniverseEngine();
    UniverseEngine(GLFWwindow *window, std::vector<const char *> validationLayers);
    void cleanup();

    void addGameobject(GameObject *obj);
    void updateObject(GameObject obj);

    void lockPipelineData();

    void createPipeline();
    void getDevices();

    //Shaders ----
    VkShaderModule getShaderModule(std::vector<char> code);
    void addShader(Shader* shader);
    // ----

    //Helpers ----
    void createSyncObjects();
    // ----

    //Adders ----
    void addDeviceExtencions(char *);
    void addUniformBuffer(UniformBuffer* buffer);
    //void addDescriptor(Descriptor *);
    //----

    //Creates the vertex and index buffers for the current loaded gameobjects
    void createModelData();

    //GET
    uint32_t getLastId();
    std::vector<GameObject *> getGameObjs();
    VkInstance getInstance(void);
    VkSurfaceKHR getSurface(void);
    VkSurfaceKHR *getSurfaceP(void);
    VkPhysicalDevice getPhyDevice(void);
    VkDevice getDevice(void);
    GLFWwindow *getWindow(void);
    std::vector<char *> getDeviceExtensions();
    VkExtent2D getExtent();
    size_t getDescriptorsSize();
    //std::vector<Descriptor *> getDescriptors();
    std::vector<UniformBuffer *> getUniformBuffers();

    // Drawing  ----
    uint32_t getCurrentImage();
    void draw();
    void framBufferResize();
    void recreateModel();
    void tick();
    // ----

    //public fileds
    std::vector<Vertex> vertecies;
    std::vector<uint32_t> indicies;

private:
    GLFWwindow *window;

    std::vector<char *> deviceExtensions;
    std::vector<const char *> validationLayers;

    uint32_t lastId;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debuggerMenssenger;
    bool enableValidationLayers;
    VkSurfaceKHR surface;
    std::vector<GameObject *> gameObjs;
    QueueFamilyIndices queueFamilyIndices;

    bool pipelineCreated = false;

    VkPhysicalDevice phyDevice;
    VkDevice device;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkRenderPass renderPass;

    std::vector<VkShaderModule> modules;

    // ========== Sync Objects ==========
    //Semaphores GPU-GPU Sync
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    //Fences CPU-GPU Sync
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;

    // ========== Vertex Buffer ==========

    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory;

    VkDeviceSize lastVertexBufferSize;
    VkDeviceSize lastIndexBufferSize;

    /*
            ========== Pipeline ========== 
        */
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    std::vector<Shader *> shaders;

    // Commands
    std::vector<VkCommandBuffer> commandBuffers;
    // swapchain
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat swapChainImageFormat;
    // the size of the swapchain images
    VkExtent2D swapChainExtent;

    //Other resourses
    MImage depthImage;

    //Descriptors
    //std::vector<Descriptor *> descriptors;
    std::vector<UniformBuffer *> unifromBuffers;
    //std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    //Can also be used as a transfer queue
    VkCommandPool commandPool = VK_NULL_HANDLE;

    void cleanupPipeline();

    void createSwapChainInternal();
    void createRenderpass();

    // ----
    void createDescriptorSetLayout();
    void createDescriptorPool();
    void createDescriptorSets();

    void createCommandPool();
    void createDepthResourses();
    void createFramebuffers();
    void createCommandBuffers();
    void createGraphicsPipeline();
    // ----

    //Helper funcions
    VkFormat findDepthFormat();

    /*
            ==== Drawing  ====
        */
    size_t currentFrame = 0;
    bool framebufferResized = false;
    uint32_t imageIndex;
    bool positionChanged = true;
    bool hasCurrentImage = false;
};
