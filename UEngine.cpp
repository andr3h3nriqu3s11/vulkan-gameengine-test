#include <vulkan/vulkan.h>
#include <iostream>
#include <stdexcept>
#include <set>
#include <fstream>
#include "UEngine.hpp"

VkCommandBuffer beginSingleCommands(VkDevice device, VkCommandPool commandPool) { return beginCommands(device, commandPool, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); }

uint32_t findMemoryType(VkPhysicalDevice phyDevice, uint32_t typeFilter, VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(phyDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & props)) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type");
}
void createBuffer(VkDevice device, VkPhysicalDevice phyDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
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
    allocInfo.memoryTypeIndex = findMemoryType(phyDevice, memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}
void copyBuffer(VkDevice device, VkCommandPool pool, VkQueue queue, VkBuffer source, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleCommands(device, pool);

    VkBufferCopy copyRegion {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, source, dstBuffer, 1, &copyRegion);

    endSigleTimeCommands(queue, device, pool, commandBuffer);
}
void createImage( VkDevice device, VkPhysicalDevice phyDevice, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memProps, VkImage image, VkDeviceMemory imageMemory) {
    VkImageCreateInfo imageInfo {};
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

    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        std::runtime_error("failed to create image");
    }

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(device, image, &memReqs);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = findMemoryType(phyDevice, memReqs.memoryTypeBits, memProps);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate memory for the texture");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
}
VkCommandBuffer beginCommands(VkDevice device, VkCommandPool commandPool, VkCommandBufferUsageFlags flags) {
    VkCommandBufferAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
    VkCommandBufferBeginInfo beginInfo {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = flags;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}
void endSigleTimeCommands ( VkQueue graphicsQueue, VkDevice device, VkCommandPool pool, VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);
    VkSubmitInfo submitInfo {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);
    vkFreeCommandBuffers(device, pool, 1, &commandBuffer);
}
void transitionImageLayout ( VkQueue queue, VkDevice device, VkCommandPool pool, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = beginSingleCommands(device, pool);

    VkImageMemoryBarrier barrier {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (hasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else {
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

    endSigleTimeCommands(queue, device, pool, commandBuffer);
}
void copyBufferToImage( VkDevice device, VkCommandPool pool, VkQueue queue, uint32_t width, uint32_t height, VkBuffer buffer, VkImage image) {
    VkCommandBuffer cmdBuffer = beginSingleCommands(device, pool);

    VkBufferImageCopy region {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0,0,0};
    region.imageExtent = {
        width,
        height,
        1
    };

    vkCmdCopyBufferToImage(cmdBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSigleTimeCommands(queue, device, pool, cmdBuffer);
}
namespace UniverseGen {
    VkImageView createImageView( VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags flags) {
        VkImageViewCreateInfo createInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = image;
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = format;
        createInfo.subresourceRange.aspectMask = flags;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        VkImageView imageView;
        if (vkCreateImageView(device, &createInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image view");
        }
        return imageView;
    }
}
bool hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}
std::vector<const char*> getRequiredExtensions(bool enableValidationLayers) {
    //Get Extencions required by glfw;
    uint32_t glfwExtensionsCount = 0;

    const char** glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);

    std::vector<const char*> extensions (glfwExtensions, glfwExtensions + glfwExtensionsCount);
    
    //Adding others

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    //Validate Extensions
    uint32_t extensionsCount;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, nullptr);
    std::vector<VkExtensionProperties> instanceExtensions(extensionsCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, instanceExtensions.data());

    for (const char* exName : extensions) {
        bool found = false;
        for (const auto& ex : instanceExtensions) {
            if (strcmp(exName, ex.extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            throw std::runtime_error("Extencion \"" + std::string(exName) + "\" not found");
        }
    }

    return extensions;
}
bool checkValidationLayerSupport(std::vector<const char*> validationLayers) {
    uint32_t layerCount;
    
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    
    if (layerCount < validationLayers.size()) return false;

    std::vector<VkLayerProperties> layers(layerCount);

    vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layer : layers) {
            if (strcmp (layerName, layer.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) return false;
    }
    return true;
}



/*
    Create pipeline stuff
*/

    VkSurfaceFormatKHR chooseSwapSurfaceFormat (const std::vector<VkSurfaceFormatKHR>& formats) {
        for (const auto& format : formats) {
            if (format.format == VK_FORMAT_B8G8R8_SRGB)
                return format;
        }
        //If the best is not found;
        return formats[0];
    }
    VkPresentModeKHR choossePresentMode (const std::vector<VkPresentModeKHR> modes) {
        for (const auto& mode: modes) {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return mode;
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }
    VkExtent2D chooseSwapExtend (UniverseEngine * en, const VkSurfaceCapabilitiesKHR capabilities) {
        if (capabilities.currentExtent.width != UINT32_MAX) {
            return capabilities.currentExtent;
        } else {
            int width, height;

            glfwGetFramebufferSize(en->getWindow(), &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t> (width),
                static_cast<uint32_t> (height),
            };

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
            return actualExtent;
        }
    }

    //Get queue families
    QueueFamilyIndices findQueueFamilies(UniverseEngine* en, std::optional<VkPhysicalDevice> device) {

        VkPhysicalDevice testDevice = en->getPhyDevice();

        if (device.has_value()) testDevice = device.value();

        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;

        vkGetPhysicalDeviceQueueFamilyProperties(testDevice, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> devicesQueues(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(testDevice, &queueFamilyCount, devicesQueues.data());

        int i = 0;

        for (const auto& queueFamily : devicesQueues) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            } 
            VkBool32 presentSuport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(testDevice, i, en->getSurface(), &presentSuport);
            if (presentSuport) {
                indices.presentFamily = i;
            }
            if(indices.isComplete()) break;
            i++;
        }

        return indices;
    }

    bool checkDeviceExtensionSupport(UniverseEngine * en, std::optional<VkPhysicalDevice> deviceIn) {
        VkPhysicalDevice device = en->getPhyDevice();

        if (deviceIn.has_value()) device = deviceIn.value();
        
        uint32_t extensionsCount = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, nullptr);
        
        if (extensionsCount < en->getDeviceExtensions().size()) return false;

        std::vector<VkExtensionProperties> props(extensionsCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, props.data());

        std::set<std::string> requiredExtensions;
        for (const auto a : en->getDeviceExtensions()) {
            requiredExtensions.insert(std::string(a));
        }
        for (const auto& extension : props ) {
            requiredExtensions.erase(extension.extensionName);
            if (requiredExtensions.empty()) return true;
        }

        return false;
    }

    SwapChainSupportDetails querySwapChainSupport (UniverseEngine* en, std::optional<VkPhysicalDevice> device) {
        VkPhysicalDevice testDevice = en->getPhyDevice();

        if (device.has_value()) testDevice = device.value();

        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(testDevice, en->getSurface(), &details.capabilities);
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(testDevice, en->getSurface(), &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount);

            vkGetPhysicalDeviceSurfaceFormatsKHR(testDevice, en->getSurface(), &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;

        vkGetPhysicalDeviceSurfacePresentModesKHR(testDevice, en->getSurface(), &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(testDevice, en->getSurface(), &presentModeCount, details.presentModes.data());
        }

        return details;
    }
    bool isDeviceSuitable (UniverseEngine * en, std::optional<VkPhysicalDevice> device) {
        VkPhysicalDevice testDevice = en->getPhyDevice();

        if (device.has_value()) testDevice = device.value();

        //MAYBE: Maybe imporove later if needed
        //VkPhysicalDeviceProperties deviceProps;
        //vkGetPhysicalDeviceProperties(device, &deviceProps);

        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(testDevice, &deviceFeatures);

        QueueFamilyIndices indices = findQueueFamilies(en, testDevice);

        bool extensionsSupported = checkDeviceExtensionSupport(en, std::optional(testDevice));

        bool swapChainAdequate = false;

        if (extensionsSupported) {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(en, std::optional(testDevice));
            swapChainAdequate = !swapChainSupport.formats.empty();
        }

        return indices.isComplete() && extensionsSupported && swapChainAdequate && deviceFeatures.samplerAnisotropy;
    }
    VkFormat findSupportedFormat(UniverseEngine * en, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(en->getPhyDevice(), format, &props);
    
            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }
    
        throw std::runtime_error("failed to find supported format!");
    }

/*
    End Create pipeline stuff
*/




















//Debug Stuff
VkResult CreateDebugUtilsMessengerEXT( VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    else return VK_ERROR_EXTENSION_NOT_PRESENT;
}
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) func(instance, debugMessenger, pAllocator);
}
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}
std::string printVec3(glm::vec3 v) {
    return "{" + std::to_string(v.x) + "," + std::to_string(v.y) + "," + std::to_string(v.z) + "}";
}

//End Debuf Stuff

//
//
//

std::vector<char> readFile(const std::string& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("could not open the file");
    }

    size_t fileSize = (size_t) file.tellg();

    std::vector<char> buffer(fileSize);

    file.seekg(0);

    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

//
//
//

//MImage
MImage::MImage() {
    this->imageView = VK_NULL_HANDLE;
    this->image = VK_NULL_HANDLE;
    this->imageMemory = VK_NULL_HANDLE;
};
MImage::MImage( uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memProps, VkImageAspectFlags flags) {
    this->width = width;
    this->height = height;
    this->format = format;
    this->tiling = tiling;
    this->usage = usage;
    this->memProps = memProps;
    this->lastLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    this->viewFlags = flags;
    this->imageView = VK_NULL_HANDLE;
    this->image = VK_NULL_HANDLE;
    this->imageMemory = VK_NULL_HANDLE;
}
VkImage MImage::getImage(void) {
    return image;
}
void MImage::create(VkDevice device, VkPhysicalDevice phyDevice) {
    VkImageCreateInfo imageInfo {};
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

    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        std::runtime_error("failed to create image");
    }

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(device, image, &memReqs);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = findMemoryType(phyDevice, memReqs.memoryTypeBits, memProps);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate memory for the texture");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
}
void MImage::clean(VkDevice device) {
    if (imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    if (image != VK_NULL_HANDLE) {
        vkDestroyImage(device, image, nullptr);
    }
    if (imageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, imageMemory, nullptr);
    }
}
void MImage::changeLayout(VkDevice device, VkQueue queue, VkCommandPool pool, VkImageLayout newLayout) {
    transitionImageLayout(queue, device, pool, image, format, lastLayout, newLayout);
    lastLayout = newLayout;
}
void MImage::createImageView(VkDevice device) {
    if (image == VK_NULL_HANDLE) {
        throw std::runtime_error("the image has not been created");
    }
    imageView = UniverseGen::createImageView(device, image, format, viewFlags);
}
VkImageView MImage::getImageView() {
    return imageView;
}

//MSampler
MSampler::MSampler() {
    sampler = VK_NULL_HANDLE;
};
VkSampler MSampler::getSampler(void) {
    return sampler;
}
void MSampler::create(VkPhysicalDevice phyDevice, VkDevice device) {
    VkSamplerCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = VK_FILTER_LINEAR;
    info.minFilter = VK_FILTER_LINEAR;
    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.anisotropyEnable = VK_TRUE;

    //TODO: make this generic
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(phyDevice, &props);

    info.maxAnisotropy = props.limits.maxSamplerAnisotropy;
    info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    info.unnormalizedCoordinates = VK_FALSE;
    info.compareEnable = VK_FALSE;
    info.compareOp = VK_COMPARE_OP_ALWAYS;
    info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    info.mipLodBias = 0.0f;
    info.minLod = 0.0f;
    info.maxLod = 0.0f;

    if (vkCreateSampler(device, &info, nullptr, &sampler) != VK_SUCCESS) {
        throw std::runtime_error("error while creating the sampler");
    }

};
void MSampler::clean(VkDevice device) {
    if (sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, sampler, nullptr);
    }
}

/* Shahder implementation */
Shader::Shader() {}
Shader::Shader(UniverseEngine * en, std::string path, VkShaderStageFlagBits flags) {
    this->path = path;
    this->en = en;
    this->flags = flags;
    this->module = VK_NULL_HANDLE;
}
VkPipelineShaderStageCreateInfo Shader::getPipelineCreateInfo() {
    if (module == VK_NULL_HANDLE) {
        auto shaderCode = readFile(this->path);
        this->module = en->getShaderModule(shaderCode);
    }
    VkPipelineShaderStageCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    createInfo.stage = this->flags;
    createInfo.module = this->module;
    createInfo.pName = "main";
    return createInfo;
}
void Shader::destroyModule() {
    if (this->module != VK_NULL_HANDLE) {
        vkDestroyShaderModule(en->getDevice(), this->module, nullptr);
        module = VK_NULL_HANDLE;   
    }
}

/* End Shader implementation */




/*Descriptor implentation start */

//Descriptor::Descriptor() {};
//void Descriptor::setId(size_t id) { this->id = id; };
//VkDescriptorType Descriptor::getType() { return type; };

/*Descriptor implentation end */

/* Uniform buffer implentation start */

UniformBuffer::UniformBuffer() {};
UniformBuffer::UniformBuffer(UniverseEngine* en, VkShaderStageFlags flags) {
    //TODO change
    this->en = en;
    //this->type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    this->flags = flags;
    //this is done by the UniverseEngine
    //this->id = en->getDescriptors().size();
    this->uniformBuffers.resize(0);
    this->uniformBuffersMemory.resize(0);
}
VkDescriptorSetLayoutBinding UniformBuffer::getDescriptorSetLayoutBinding() {
  std::cout << "get Descriptor set layout\n";
    VkDescriptorSetLayoutBinding layoutBinding {};
    layoutBinding.binding = id;    
    layoutBinding.descriptorType = getType();
    //TODO: see if needed to made dynamic
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = flags;
    return layoutBinding;
}
void UniformBuffer::preSwapChainCreate(size_t swapChainSize) {
    //TODO: Possible change
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    std::vector<VkBuffer> b;
    std::vector<VkDeviceMemory> bm;

    b.resize(swapChainSize);
    bm.resize(swapChainSize);

    for (size_t i = 0; i < swapChainSize; i++) {
        createBuffer(en->getDevice(), en->getPhyDevice(), bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, b[i], bm[i]);
    }

    this->uniformBuffers = b;
    this->uniformBuffersMemory = bm;
}
//TODO change
VkWriteDescriptorSet UniformBuffer::getWriterDescriptorSet() {
    VkWriteDescriptorSet set {};
    set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    set.dstBinding = id;
    set.dstArrayElement = 0;
    set.descriptorType = getType();
    set.descriptorCount = 1;
    return set;
}
VkBuffer UniformBuffer::getBuffer(uint32_t i) {
    return this->uniformBuffers[i];
}
VkDeviceSize UniformBuffer::getSize() {
    return sizeof(UniformBufferObject);
}
void UniformBuffer::cleanUp() {
    for (size_t i = 0; i < uniformBuffers.size(); i++) {
        vkDestroyBuffer(en->getDevice(), uniformBuffers[i], nullptr);
        vkFreeMemory(en->getDevice(), uniformBuffersMemory[i], nullptr);
    }
}
void UniformBuffer::updateUniformBuffer(uint32_t index, UniformBufferObject obj) {
    if (uniformBuffers.size() - 1 < index)
        throw std::runtime_error("The can not update an index that is out of bounds");
    void *data;
    vkMapMemory(en->getDevice(), uniformBuffersMemory[index], 0, sizeof(obj), 0, &data);
    memcpy(data, &obj, sizeof(obj));
    vkUnmapMemory(en->getDevice(), uniformBuffersMemory[index]);
}
VkDescriptorType UniformBuffer::getType() { return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; }
void UniformBuffer::setId(size_t id) { this->id=id; }
size_t UniformBuffer::getId() { return this->id; }






template <typename T>
ShaderStorageBuffer<T>::ShaderStorageBuffer() {};
template <typename T>
ShaderStorageBuffer<T>::ShaderStorageBuffer(UniverseEngine* en, VkShaderStageFlags flags) {
    this->en = en;
    this->flags = flags;
    //this is done by the UniverseEngine
    //this->id = en->getDescriptors().size();
    this->ssBuffers.resize(0);
    this->ssBuffersMemory.resize(0);
}
template <typename T>
VkDescriptorSetLayoutBinding ShaderStorageBuffer<T>::getDescriptorSetLayoutBinding() {
    VkDescriptorSetLayoutBinding layoutBinding {};
    layoutBinding.binding = id;    
    layoutBinding.descriptorType = getType();
    //TODO: see if needed to made dynamic
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = flags;
    return layoutBinding;
}
template <typename T>
void ShaderStorageBuffer<T>::preSwapChainCreate(size_t swapChainSize, size_t objssize) {
    bufferSize = sizeof(T) * objssize;

    std::vector<VkBuffer> b;
    std::vector<VkDeviceMemory> bm;

    b.resize(swapChainSize);
    bm.resize(swapChainSize);

    for (size_t i = 0; i < swapChainSize; i++) {
        createBuffer(en->getDevice(), en->getPhyDevice(), bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, b[i], bm[i]);
    }

    this->ssBuffers = b;
    this->ssBuffersMemory = bm;
}
template <typename T>
VkWriteDescriptorSet ShaderStorageBuffer<T>::getWriterDescriptorSet() {
    VkWriteDescriptorSet set {};
    set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    set.dstBinding = id;
    set.dstArrayElement = 0;
    set.descriptorType = getType();
    set.descriptorCount = 1;
    return set;
}

template <typename T>
VkBuffer ShaderStorageBuffer<T>::getBuffer(uint32_t i) {
    return this->ssBuffers[i];
}
template <typename T>
VkDeviceSize ShaderStorageBuffer<T>::getSize() {
    return this->bufferSize;
}
template <typename T>
void ShaderStorageBuffer<T>::cleanUp() {
    for (size_t i = 0; i < ssBuffers.size(); i++) {
        vkDestroyBuffer(en->getDevice(), ssBuffers[i], nullptr);
        vkFreeMemory(en->getDevice(), ssBuffersMemory[i], nullptr);
    }
}
template <typename T>
void ShaderStorageBuffer<T>::updateBuffer(uint32_t index, std::vector<T> obj) {
    if (this->ssBuffers.size() - 1 < index)
        throw std::runtime_error("The can not update an index that is out of bounds");
    void *data;

    vkMapMemory(en->getDevice(), ssBuffersMemory[index], 0, sizeof(obj), 0, &data);

    ModelBuffer* b = (ModelBuffer *) data;

    for (size_t i = 0; i < obj.size(); i++) {
        b[i].model = obj[i];
    }

    memcpy(data, &b, bufferSize);
    vkUnmapMemory(en->getDevice(), ssBuffersMemory[index]);
}
template <typename T>
VkDescriptorType ShaderStorageBuffer<T>::getType() { return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; }
template <typename T>
void ShaderStorageBuffer<T>::setId(size_t id) { this->id=id; }
template <typename T>
size_t ShaderStorageBuffer<T>::getId() { return this->id; }
/* Uniform buffer end */



/*

    UniverseEngine implemnetation

*/

UniverseEngine::UniverseEngine() {};
UniverseEngine::UniverseEngine(GLFWwindow * window, std::vector<const char *> validationLayers) {
    enableValidationLayers = validationLayers.size() != 0;
    this->validationLayers = validationLayers;

    VkApplicationInfo appInfo {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Universe";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Universe";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    auto extensions = getRequiredExtensions(enableValidationLayers);
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    
    if (enableValidationLayers && !checkValidationLayerSupport(validationLayers))
        throw std::runtime_error("Validation layers required!");

    createInfo.enabledLayerCount = static_cast<uint32_t> (validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
    
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
        throw std::runtime_error("failed to create instance");

    if (enableValidationLayers) {
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debuggerMenssenger) != VK_SUCCESS)
            throw std::runtime_error("cloud not initialize the debugger");
    }

    this->window = window;

    // Set defaults
    surface = VK_NULL_HANDLE;
    phyDevice = VK_NULL_HANDLE;
    lastId = 0;
};

/* Game object stuff */

void UniverseEngine::addGameobject(GameObject* o) {
    o->setId(lastId);
    lastId += 1;
    //Add the verticies to the known verticies
    gameObjs.push_back(o);

    recreateModel();
    createModelData();
}

/* Vulkan stuff */

void UniverseEngine::addDeviceExtencions(char * a) {
    if (phyDevice != VK_NULL_HANDLE)
        throw std::runtime_error("can not add device extencion after the device is created");
    
    deviceExtensions.push_back(a);
}

void UniverseEngine::cleanup(void) {

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }
            
    cleanupPipeline();
            
    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);
    
    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexBufferMemory, nullptr);

    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

    vkDestroyCommandPool(device, commandPool, nullptr);

    if (surface != VK_NULL_HANDLE)
        vkDestroySurfaceKHR(instance, surface , nullptr);

    if (enableValidationLayers)
        DestroyDebugUtilsMessengerEXT(instance, debuggerMenssenger, nullptr);


    vkDestroyDevice(device, nullptr);

    vkDestroyInstance(instance, nullptr);
}

void UniverseEngine::lockPipelineData() {
    createDescriptorSetLayout();
    createCommandPool();
}

void UniverseEngine::getDevices() {

    // Get physical device
    
    // Gets all physical devices ----
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with vulkan");
        }
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    // ----
    
    // check if each device is suitable ----
        for (const auto& device : devices) {
            if (isDeviceSuitable(this, std::optional(device))) {
                phyDevice = device;
                break;
            }
        }
    // ----

    // if no device is found then error out
    if (phyDevice == VK_NULL_HANDLE)
        throw std::runtime_error("No GPUs found with vulkan");

    queueFamilyIndices = findQueueFamilies(this, std::nullopt);

    /* 

        get logical device 

    */
    
    // create the queue info ----
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value() };
        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }
    // ----

    VkPhysicalDeviceFeatures deviceFeatures {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    //Change create info based if we have the validation layers
    if (validationLayers.size() != 0) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(phyDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device");
    }

    vkGetDeviceQueue(device, queueFamilyIndices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, queueFamilyIndices.presentFamily.value(), 0, &presentQueue);

}

void UniverseEngine::createCommandPool() {
    VkCommandPoolCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(device, &info, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create the command pool");
    }
}
 
void UniverseEngine::tick() {
    for (auto a : gameObjs) {
        a->tick();
    }
}

// Vertex ----

void UniverseEngine::createModelData() {
    if (device == VK_NULL_HANDLE || phyDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("Devices must be created first! run getDevices() to get those devices");
    }
    if (commandPool == VK_NULL_HANDLE) {
        throw std::runtime_error("commandPool must be created! run lockPipelineData() to the command pool");
    }

    //TODO: find a better way to do this
    if (vertexBuffer == VK_NULL_HANDLE || positionChanged) {
        std::vector<uint32_t> is;
        std::vector<Vertex> vs;
        uint32_t p = 0;
        for (auto g : gameObjs) {
            Mesh m = g->getMesh();
            for (auto v : m.v) {
                vs.push_back(v);
            }
            for (auto i : m.i) {
                is.push_back(i + p);
            }
            p += m.v.size();
        }
        this->vertecies = vs;
        this->indicies = is;
        positionChanged = false;
    }

    /*
        create vertex buffer
    */
    VkDeviceSize size = sizeof(vertecies[0]) * vertecies.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    //Create the staging buffer
    createBuffer(device, phyDevice, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;

    //Copy data to buffer
    vkMapMemory(device, stagingBufferMemory, 0, size, 0, &data);
        memcpy(data, vertecies.data(), (size_t) size);
    vkUnmapMemory(device, stagingBufferMemory);

    // Check if a buffer exist and if the size is different if so destory it
    // if the buffer does not exist create buffer on the gpu side
    if (vertexBuffer != VK_NULL_HANDLE && size != lastVertexBufferSize) {
        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, vertexBufferMemory, nullptr);
        vertexBuffer = VK_NULL_HANDLE;
    }
    if (vertexBuffer == VK_NULL_HANDLE)
        createBuffer(device, phyDevice, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory );
    lastVertexBufferSize = size;

    copyBuffer(device, commandPool, graphicsQueue, stagingBuffer, vertexBuffer, size);

    //Destory staging buffer
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);

    /*
        create index buffer
    */
    
    size = sizeof(indicies[0]) * indicies.size();
    
    // create new staging buffer for the index buffer
    createBuffer(device, phyDevice, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    //Copy data to buffer
    vkMapMemory(device, stagingBufferMemory, 0, size, 0, &data);
        memcpy(data, indicies.data(), (size_t) size);
    vkUnmapMemory(device, stagingBufferMemory);

    // Check if a buffer exist and if the size is different if so destory it
    // if the buffer does not exist create buffer on the gpu side
    if (indexBuffer != VK_NULL_HANDLE && lastIndexBufferSize != size) {
        vkDestroyBuffer(device, indexBuffer, nullptr);
        vkFreeMemory(device, indexBufferMemory, nullptr);
        indexBuffer = VK_NULL_HANDLE;
    }
    if (indexBuffer == VK_NULL_HANDLE)
        createBuffer(device, phyDevice, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory );
    lastIndexBufferSize = size;

    //Copy data from the staging buffer to the gpu buffer
    copyBuffer(device, commandPool, graphicsQueue, stagingBuffer, indexBuffer, size);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

// Helper functions ----
    VkFormat UniverseEngine::findDepthFormat() {
        return findSupportedFormat(this,
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }
// ----

// Draw stuff ----
    uint32_t UniverseEngine::getCurrentImage() {
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            createPipeline();
            return 0;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swapchain image");
        }

        if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
            vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
        }

        imagesInFlight[imageIndex] = inFlightFences[currentFrame];
        hasCurrentImage = true;

        if (positionChanged)
            createModelData();

        return imageIndex + 1;
    }

    //Run getCurrentImage 1st
    void UniverseEngine::draw() {

        if (!hasCurrentImage) {
            throw std::runtime_error("getCurrentImage needs to be run 1st");
        }

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStates[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        VkSubmitInfo info {};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = waitSemaphores;
        info.pWaitDstStageMask = waitStates;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &commandBuffers[imageIndex];
        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = signalSemaphores;

        vkResetFences(device, 1, &inFlightFences[currentFrame]);

        if (vkQueueSubmit(graphicsQueue, 1, &info, inFlightFences[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit to draw queue");
        }

        VkPresentInfoKHR presentInfo {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        VkSwapchainKHR swapChains[] = {swapchain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
            framebufferResized = false;
            createPipeline();
            return;
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to acquire swapchain image");
        }
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

        hasCurrentImage = false;
    }

    void UniverseEngine::framBufferResize() {
        framebufferResized = true;
    }
//----

// Pipeline --------------------

void UniverseEngine::createPipeline() {
    if (descriptorSetLayout == VK_NULL_HANDLE) { throw std::runtime_error("The data must be locked first"); }
    
    //If the pipeline is already created check for the screen size
    if (pipelineCreated) {
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);

        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }
        
        vkDeviceWaitIdle(device);
    }

    //Only cleans if the pipeline was created
    cleanupPipeline();

    createSwapChainInternal();

    imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

    createRenderpass();

    createDepthResourses();

    //Required renderpass, swapchainviews, depth resourses
    createFramebuffers();

    createModelData();

    createGraphicsPipeline();

    //Pre process descriptors
    // for (auto c : descriptors) {
    //     c->preSwapChainCreate(swapChainImages.size());
    // }
    //Pre process uniform buffers
    for (auto u : unifromBuffers) {
        u->preSwapChainCreate(swapChainImages.size());
    }

    //Depends on the swap chain, descriptors
    createDescriptorPool();

    createDescriptorSets();

    createCommandBuffers();

    pipelineCreated = true;
}

void UniverseEngine::cleanupPipeline() {
    if (!pipelineCreated) return;

    vkDestroyPipeline(device, graphicsPipeline, nullptr);

    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

    depthImage.clean(device);

    vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
    
    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    //Clean up descriptors
    // for (auto c : descriptors) {
    //     c->cleanUp();
    // }
        //Clean up unifrom buffers
        for (auto c : unifromBuffers) {
            c->cleanUp();
        }

    size_t descSize = unifromBuffers.size();

    if (descSize != 0)
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);

    vkDestroyRenderPass(device, renderPass, nullptr);

    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, swapchain, nullptr);
}

//Needs to be recreated each time because it depends on the swapchain
void UniverseEngine::createDescriptorPool() {
    size_t descSize = unifromBuffers.size();
    if (descSize == 0) return;
    std::vector<VkDescriptorPoolSize> pools;
    uint32_t count = static_cast<uint32_t>(swapChainImages.size());

    // for (auto desc : descriptors) {
    //     VkDescriptorPoolSize p {};
    //     p.type = desc->getType();
    //     p.descriptorCount = count;
    //     pools.push_back(p);
    // }

    // Added UnifromBuffer
        for (auto ub : unifromBuffers) {
            VkDescriptorPoolSize p {};
            p.type = ub->getType();
            p.descriptorCount = count;
            pools.push_back(p);
        }

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(pools.size());
    poolInfo.pPoolSizes = pools.data();
    poolInfo.maxSets = count;
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("faield to create descriptor pool");
    }
}

void UniverseEngine::createSwapChainInternal() {
    SwapChainSupportDetails details = querySwapChainSupport(this, std::nullopt);
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(details.formats);
    VkPresentModeKHR presentMode = choossePresentMode(details.presentModes);
    VkExtent2D extent = chooseSwapExtend(this, details.capabilities);
    uint32_t imageCount = static_cast<uint32_t>(std::min(details.capabilities.minImageCount + 1, details.capabilities.maxImageCount));

    //Get indices for the queues
    QueueFamilyIndices indices = queueFamilyIndices;
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    //Create swapchain info
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.preTransform = details.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    //Based on the indices change create info
    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.oldSwapchain = VK_NULL_HANDLE;

    //Try to create the swapchain
    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain) != VK_SUCCESS) {
        throw std::runtime_error("could not create swapchain");
    }

    //Get the swapchain images
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapChainImages.data());

    //Store the format and the extend size
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;

    //Create the image views
    swapChainImageViews.resize(swapChainImages.size());
    for(size_t i = 0; i < swapChainImages.size(); i++) {
        swapChainImageViews[i] = UniverseGen::createImageView(device, swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }

}

void UniverseEngine::createRenderpass() {
    
    //Color ----
        VkAttachmentDescription colorDesc{};
        colorDesc.format = swapChainImageFormat;
        colorDesc.samples = VK_SAMPLE_COUNT_1_BIT;
        colorDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorDescRef {};
        colorDescRef.attachment = 0;
        colorDescRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // ----

    // Depth ----
        VkAttachmentDescription depthDesc{};
        depthDesc.format = findDepthFormat();
        depthDesc.samples = VK_SAMPLE_COUNT_1_BIT;
        depthDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthDesc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthDescRef {};
        depthDescRef.attachment = 1;
        depthDescRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    // ----

    
    VkSubpassDescription subpass { };
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorDescRef;
    subpass.pDepthStencilAttachment = &depthDescRef;

    VkSubpassDependency dependency {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> descs = {colorDesc, depthDesc};

    VkRenderPassCreateInfo renderPassInfo {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(descs.size());
    renderPassInfo.pAttachments = descs.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("could not create renderpass");
    }
}

//void UniverseEngine::addDescriptor(Descriptor* desc) {
//    if (descriptorSetLayout != VK_NULL_HANDLE) {
//        throw std::runtime_error("The descriptor set layout is already created");
//    }
//    std::cout << "Added Descriptor\n";
//    desc->setId(descriptors.size());
//    descriptors.push_back(desc);
//}

void UniverseEngine::addUniformBuffer(UniformBuffer * desc) {
    if (descriptorSetLayout != VK_NULL_HANDLE) {
        throw std::runtime_error("The descriptor set layout is already created");
    }
    std::cout << "Added Descriptor\n";
    size_t descSize = getDescriptorsSize();
    desc->setId(descSize);
    unifromBuffers.push_back(desc);
}

VkShaderModule UniverseEngine::getShaderModule(std::vector<char> code) {
    VkShaderModuleCreateInfo createInfo{};

    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule module;

    if (vkCreateShaderModule(device, &createInfo, nullptr, &module) != VK_SUCCESS) {
        throw std::runtime_error("could not create shader module");
    }

    return module;
}

void UniverseEngine::addShader(Shader* shader) {
    shaders.push_back(shader);
}

void UniverseEngine::createGraphicsPipeline() {

    std::vector<VkPipelineShaderStageCreateInfo> shaderStates;

    for (Shader* shader : shaders) {
        shaderStates.push_back(shader->getPipelineCreateInfo());
    }

    //Got shaders

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescription();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};

    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo {};
    inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;
    
    VkViewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swapChainExtent.width;
    viewport.height = (float) swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 0.0f;

    VkRect2D scissor {};
    scissor.offset = {0,0};
    scissor.extent = swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    //Disables geometry so that there is no output if set to true
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    //Enable wireframe requires enableing a gpu feature
    //rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
    //Any wider than 1 requires enabeling the widLines GPU feature
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;//VK_CULL_MODE_FRONT_BIT & VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; //Optional

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; 

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo  {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Could not create pipeline layout");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};

    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStates.size());
    pipelineInfo.pStages = shaderStates.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr;

    pipelineInfo.layout = pipelineLayout;

    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("no pipeline created");
    }

    //Clean up
    for (Shader* a : shaders) {
        a->destroyModule();
    }
}
/*
void UniverseEngine::addDescriptorSetLayoutBindings(VkDescriptorType type, VkShaderStageFlags flags, VkSampler * sampler ) {
    if (descriptorSetLayout != VK_NULL_HANDLE) {
        throw std::runtime_error("The descriptor set layout is already created");
    }

    VkDescriptorSetLayoutBinding layoutBinding {};
    layoutBinding.binding = descriptorSetLayoutBindings.size();
    layoutBinding.descriptorType = type;
    //TODO: see if needed to made dynamic
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = flags;
    layoutBinding.pImmutableSamplers = sampler;

    descriptorSetLayoutBindings.push_back(layoutBinding);

}*/

void UniverseEngine::createDescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> lbs;

    //For unigorm buffers
        for (auto a : unifromBuffers) {
            lbs.push_back(a->getDescriptorSetLayoutBinding());
        }

    VkDescriptorSetLayoutCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = static_cast<uint32_t>(lbs.size());
    info.pBindings = lbs.data();

    if (vkCreateDescriptorSetLayout(device, &info, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor");
    }
}
//Descriptors must be set before this is run
void UniverseEngine::createDescriptorSets() {
    if (getDescriptorsSize() == 0) return;

    uint32_t size = static_cast<uint32_t>(swapChainImages.size());

    std::vector<VkDescriptorSetLayout> layouts(size, descriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(size);
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(size);

    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("faield to allocate descriptor sets");
    }

    for (size_t i = 0; i < size; i++) {
        /* TODO: do in the class
        VkDescriptorImageInfo imageInfo {};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureImage.getImageView();
        imageInfo.sampler = textureSampler.getSampler();
        */

        std::vector<VkWriteDescriptorSet> sets;
        sets.resize(getDescriptorsSize());

        /*
        for (uint32_t bi = 0; bi < getDescriptorsSize(); bi++) {
            VkWriteDescriptorSet set = descriptors[bi]->getWriterDescriptorSet(i);
            set.dstSet = descriptorSets[i];
            sets[i] = set;
        }
        */

        for (UniformBuffer * u : unifromBuffers) {
            VkWriteDescriptorSet setu = u->getWriterDescriptorSet();
            setu.dstSet = descriptorSets[i];
            VkDescriptorBufferInfo bufInfou {};
            bufInfou.buffer = u->getBuffer(i);
            bufInfou.offset = 0;
            bufInfou.range = u->getSize();
            setu.pBufferInfo = &bufInfou;
            sets[u->getId()] = setu;
        }

/*      TODO: class
        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo; */

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
    }
}

void UniverseEngine::createDepthResourses() {
    depthImage = MImage(swapChainExtent.width, swapChainExtent.height, findDepthFormat(), VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
    depthImage.create(device, phyDevice);
    depthImage.createImageView(device);
}

// ----

void UniverseEngine::createFramebuffers() {
    swapChainFramebuffers.resize(swapChainImages.size());

    VkImageView v = depthImage.getImageView();

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {

        std::array<VkImageView, 2> attachments = {
            swapChainImageViews[i],
            v
        };

        VkFramebufferCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.renderPass = renderPass;
        createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        createInfo.pAttachments = attachments.data();
        createInfo.width = swapChainExtent.width;
        createInfo.height = swapChainExtent.height;
        createInfo.layers = 1;

        if (vkCreateFramebuffer(device, &createInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Frame buffer failed to create");
        }
    }
}

void UniverseEngine::createCommandBuffers() {
    commandBuffers.resize(swapChainFramebuffers.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to created buffers");
    }

    for (size_t i = 0; i < commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer");
        }

        std::array<VkClearValue, 2> clearValues{};
        //TODO: possible change
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        //TODO: Possible change to add mutiple passes
        VkRenderPassBeginInfo renderPassInfo {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[i];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChainExtent;
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
        
        VkBuffer vertexBuffers[] = {vertexBuffer};
        VkDeviceSize offsets[] = {0};

        vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

        vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        if (getDescriptorsSize() != 0)
            vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);

        vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indicies.size()), 1, 0 , 0, 0);

        vkCmdEndRenderPass(commandBuffers[i]);

        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer");
        }
    }
}

void UniverseEngine::createSyncObjects() {
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo createSemaphoreInfo{};
    createSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo createFenceInfo {};
    createFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    createFenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if( vkCreateSemaphore(device, &createSemaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &createSemaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS || 
            vkCreateFence(device, &createFenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create semaphore");
        }
    }
}

void UniverseEngine::recreateModel() { positionChanged = true; }

/*

    Getters start

*/

VkPhysicalDevice UniverseEngine::getPhyDevice() { return phyDevice; };
VkDevice UniverseEngine::getDevice() { return device; }
VkInstance UniverseEngine::getInstance(void) { return instance; }
VkSurfaceKHR UniverseEngine::getSurface(void) { return surface; }
VkSurfaceKHR* UniverseEngine::getSurfaceP(void) { return &surface; }
std::vector<GameObject*> UniverseEngine::getGameObjs() { return gameObjs; }
uint32_t UniverseEngine::getLastId() { return lastId; }
std::vector<char *> UniverseEngine::getDeviceExtensions() { return deviceExtensions; }
VkExtent2D UniverseEngine::getExtent() { return swapChainExtent; }
std::vector<UniformBuffer *> UniverseEngine::getUniformBuffers() { return unifromBuffers; }
GLFWwindow* UniverseEngine::getWindow() {return window;}
size_t UniverseEngine::getDescriptorsSize() { return unifromBuffers.size(); }

/* Getters End */

/* UniverseEngine implemnetaions end */

/*

    PaneObject implementation

*/

PaneObject::PaneObject() {}

PaneObject::PaneObject(UniverseEngine * e, glm::vec3 posIn, glm::vec2 size, glm::vec3 color): GameObject(e) {

    std::vector<Vertex> v = {
        {{0, 0, 0}, color, {-1.0, -1.0}, 1},
        {{size.x, 0, 0}, color, {-1.0, -1.0}, 1},
        {{0, size.y, 0}, color, {-1.0, -1.0}, 1},
        {{size.x, size.y, 0}, color, {-1.0, -1.0}, 1},
    };
    std::vector<uint32_t> i = {
        0, 1, 2,
        1, 2, 3
    };
    this->vertecies = v;
    this->indicies = i;
    this->pos = glm::vec4(posIn, 1.0f);
    this->vec = glm::vec3(0.0f);
    this->acc = glm::vec3(0.0f);
};
PaneObject::PaneObject(UniverseEngine * e, glm::vec3 pos, glm::vec2 size, glm::vec3 color, glm::mat4 rot): GameObject(e) {
    glm::vec3 s1 = glm::vec3(rot * glm::vec4(size.x, 0, 0, 1));
    glm::vec3 s2 = glm::vec3(rot * glm::vec4(0, size.y, 0, 1));
    glm::vec3 s3 = glm::vec3(rot * glm::vec4(size.x, size.y, 0, 1));
    std::vector<Vertex> v = {
        {{0, 0, 0}, color, {-1.0, -1.0}},
        {s1, color, {-1.0, -1.0}},
        {s2, color, {-1.0, -1.0}},
        {s3, color, {-1.0, -1.0}},
    };
    std::vector<uint32_t> i = {
        0, 1, 2,
        1, 2, 3
    };
    this->vertecies = v;
    this->indicies = i;
    this->pos = glm::vec4(pos, 1.0f);
    this->vec = glm::vec3(0.0f);
    this->acc = glm::vec3(0.0f);
};

/* End PaneObject implementaions */
