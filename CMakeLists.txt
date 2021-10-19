cmake_minimum_required(VERSION 3.21.0)

set(CMAKE_CXX_STANDARD 17)

project(universeengine)

add_custom_target(vert.spv glslc shader.vert -o vert.spv 
    WORKING_DIRECTORY shaders 
    SOURCES shaders/shader.vert
    COMMENT "Compiliing vertShader"
)

add_custom_target(frag.spv glslc shader.frag -o frag.spv
    WORKING_DIRECTORY shaders 
    SOURCES shaders/shader.frag
    COMMENT "Compiliing fragShader"
)

add_library(UEngine UEngine.cpp)
add_library(GameObject ./lib/GameObject.cpp)
    
add_executable(main main.cpp)

add_dependencies(main frag.spv vert.spv)

target_link_libraries(main PRIVATE UEngine)
#target_link_libraries(main PRIVATE UniformBuffer)
target_link_libraries(main PRIVATE GameObject)

target_link_directories(main PRIVATE .)
target_link_directories(main PRIVATE ./lib)

target_link_libraries(main PUBLIC glfw vulkan)
