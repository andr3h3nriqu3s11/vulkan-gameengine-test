#define STB_IMAGE_IMPLEMENTATION
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <set>
#include <cstdint>
#include <thread>
#include <array>
#include <chrono>
#include "stb_image.h"

#include "UEngine.hpp"


const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers = {
#ifdef NDEBUG
#else
    "VK_LAYER_KHRONOS_validation",
#endif
};

const std::vector<const char*> deviceExtensions = {
};

class UniverseApp {
    public:
        UniverseApp() {
            p = PaneObject(&this->uniEngine, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec2(10.f, 10.0f), glm::vec3(0.5f, 0.5f, 0.5f));
            p1 = PaneObject(&this->uniEngine, glm::vec3(0.0f, 0.0f, 2.0f), glm::vec2(10.f, 10.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            p1.updateRot(glm::rotate(glm::mat4(1.0f), glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
        }
        void run() {
            #ifdef NDEBUG
            #else
            std::cout << "Starting up in debug mode\n";
            #endif

            initWindow();
            initVulkan();
            mainLoop();
            cleanup();
        }

    private:
        GLFWwindow* window;

        UniverseEngine uniEngine;

        MImage textureImage;
        MSampler textureSampler;

        UniformBuffer ub;

        Shader vertShader;
        Shader fragShader;

        PaneObject p = PaneObject();
        PaneObject p1 = PaneObject();

        void initWindow() {
            glfwInit();

            //So glfw does not load opengl
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

            window = glfwCreateWindow(WIDTH, HEIGHT, "Universe", nullptr, nullptr);

            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            glfwSetWindowUserPointer(window, this);
            glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
            glfwSetCursorPosCallback(window, cursorPositionCallback);
        }

        static void framebufferResizeCallback(GLFWwindow * window, int width, int height) {
            auto app = reinterpret_cast<UniverseApp*>(glfwGetWindowUserPointer(window));
            app->uniEngine.framBufferResize();
        }

        void initVulkan() {
            uniEngine = UniverseEngine(window, validationLayers);

            //Start scene

            if (glfwCreateWindowSurface(uniEngine.getInstance(), window, nullptr, uniEngine.getSurfaceP()) != VK_SUCCESS) throw std::runtime_error("Failed to create window");

            uniEngine.addDeviceExtencions(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

            uniEngine.getDevices();
            

            //Stuff to send to the buffers

            ub = UniformBuffer(&uniEngine ,VK_SHADER_STAGE_VERTEX_BIT);
            //uniEngine.addDescriptor(&ub);
            uniEngine.addUniformBuffer(&ub);

            std::cout << "added buffer" << std::endl;

//            createTextureImage();
//            textureImage.createImageView(uniEngine.getDevice());
//            textureSampler.create(uniEngine.getPhyDevice(), uniEngine.getDevice());

//          uniEngine.addDescriptorSetLayoutBindings(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);

            vertShader = Shader(&uniEngine, "shaders/vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
            uniEngine.addShader(&vertShader);
            fragShader = Shader(&uniEngine, "shaders/frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
            uniEngine.addShader(&fragShader);

            uniEngine.lockPipelineData();

            uniEngine.addGameobject(&p);
            uniEngine.addGameobject(&p1);

            uniEngine.createPipeline();

            std::cout << "pipe line created\n";

            uniEngine.createSyncObjects();
            
            std::cout << "got sync objs\n";
        }

/*
        void createTextureImage() {
            int texWidth;
            int texHeight;
            int texChannels;
            stbi_uc* pixels = stbi_load("textures/texture.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
            VkDeviceSize imageSize = texWidth * texHeight * 4;
            if (!pixels) {
                throw std::runtime_error("failed to load texture");
            }
            VkBuffer stagingBuffer;
            VkDeviceMemory stagingBufferMemory;
            createBuffer(device, physicalDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
            void* data;
            vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
                memcpy(data, pixels, static_cast<size_t>(imageSize));
            vkUnmapMemory(device, stagingBufferMemory);

            stbi_image_free(pixels);

            MImage textureImageTemp( static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

            textureImage = textureImageTemp;

            textureImage.create(device, physicalDevice);

            textureImage.changeLayout(device, graphicsQueue, commandPool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            copyBufferToImage(device, commandPool, graphicsQueue, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), stagingBuffer, textureImage.getImage());

            textureImage.changeLayout(device, graphicsQueue, commandPool, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            //Clean up
            vkDestroyBuffer(device, stagingBuffer, nullptr);
            vkFreeMemory(device, stagingBufferMemory, nullptr);
        }

*/
        
        //MAIN EVENT LOOP
        float angle = 0.0f;
        float cameraAngle = 0.0f;

        glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f));

        glm::vec3 pos = {0.0f, 0.0f, 0.0f};
        glm::vec3 v   = {0.0f, 0.0f, 0.0f};
        glm::vec3 a   = {0.0f, 0.0f, 0.0f};

        glm::vec2 mousepos = {0.0f, 0.0f};

        float maxPlayerWalkSpeed = 0.5f;

        bool lastLoopPressedESC = false;

        void mainLoop() {
            static auto startTime = std::chrono::high_resolution_clock::now();

            while (!glfwWindowShouldClose(window)) {

                rot = glm::rotate(glm::mat4(1.0f), glm::radians(-angle), glm::vec3(0.0f, 0.0f, 1.0f));

                drawFrame();

                if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                    if (!lastLoopPressedESC) {
                        recordingMouse = !recordingMouse;
                        std::cout << "switch\n";
                        if (recordingMouse) {
                            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                        } else {
                            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                        }
                    }
                    lastLoopPressedESC = true;
                } else {
                    lastLoopPressedESC = false;
                }

                glm::vec3 am = {0.0, 0.0, 0.0};

                if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                    am.x = 0.1f;
                }
                if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                    am.x = -0.1f;
                }

                if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
                    am.y = -0.05f;
                }
                if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
                    am.y = 0.05f;
                }

                glm::vec3 f = {-0.3, -0.3, 0.0};

                f = f * v;

                a = glm::vec3(rot * glm::vec4(am, 1.0f)) + f;

                v = v + a;
                pos = pos + v;

                auto currentTime = std::chrono::high_resolution_clock::now();
                float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

                //p1.updatePos(glm::vec3(0.0, 0.0, std::sin(time) * 5));
                //uniEngine.tick();

                //Get events like the close event or mouse / keyboard events
                glfwPollEvents();
            }

            vkDeviceWaitIdle(uniEngine.getDevice());
        }

        void drawFrame() {
            uint32_t imageIndex = uniEngine.getCurrentImage();

            if (imageIndex == 0) return;

            imageIndex = imageIndex - 1;

            updateUniformBuffer(imageIndex);

            uniEngine.draw();
        }
        
        bool recordingMouse = true;

        static void cursorPositionCallback(GLFWwindow * window, double xPos, double yPos) {
            auto app = reinterpret_cast<UniverseApp*>(glfwGetWindowUserPointer(window));
            if (app->recordingMouse) {
                //std::cout << "xpos: " << xPos << ", ypos:" << yPos << "\n";
                glm::vec2 mousenewpos = {xPos, yPos};

                glm::vec2 diff = app->mousepos - mousenewpos;

                app->angle += diff.x * 160 / app->uniEngine.getExtent().width;
                app->cameraAngle += diff.y * 160 / app->uniEngine.getExtent().height;

                if (app->cameraAngle > 80.0) {
                    app->cameraAngle = 80.0;
                } else if (app->cameraAngle < -89.0) {
                    app->cameraAngle = -89.0;
                }

                app->mousepos = mousenewpos;
            }
        }

        void updateUniformBuffer(uint32_t imageIndex) {
            UniformBufferObject ubo {};

            for (GameObject* o : uniEngine.getGameObjs()) {
	            //ubo.model/*[o->id]*/ = glm::translate(glm::mat4(1.0f), glm::vec3(o->pos));
            }

            glm::vec3 eyeHight = {0.0f, 0.0f, 1.8f};
            glm::vec4 front = {10.0f, 0.0f, 0.0f, 0.0f};
            front = glm::rotate(rot, glm::radians(-cameraAngle), glm::vec3(0.0f, 1.0f, 0.0f)) * front;

	        ubo.view = glm::lookAt(pos + eyeHight, (pos + eyeHight) + glm::vec3(front), glm::vec3(0.0f, 0.0f, -1.0f));

            ubo.proj = glm::perspective(glm::radians(100.0f), uniEngine.getExtent().width / (float) uniEngine.getExtent().width, 0.1f, 50.0f);

            ub.updateUniformBuffer(imageIndex, ubo);
        }

        void cleanup() {

//            textureSampler.clean(device);
//            textureImage.clean(device);
            
            uniEngine.cleanup();
            //WINDOW
            glfwDestroyWindow(window);
            glfwTerminate();
        }
};

int main() {
    UniverseApp app;
    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
