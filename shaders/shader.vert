#version 450

#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;

layout(location = 1) in vec3 inColor;

layout (location = 2) in vec2 inFragPos;

layout (location = 3) in float c;

layout (location = 4) in uint gameobj;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragPos;
layout(location = 2) out float cout;

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    mat4 model[1];
} ubo; 

void main() {
    //vec3 a = inPosition;
    //a.z = a.z + gameobj;/*a.z + gameobj*/;
    //a.z = a.z + 1;
    gl_Position =  ubo.proj * ubo.view * vec4(inPosition, 1.0);
    //gl_Position = vec4(inPosition, 1.0);
    fragColor = inColor;
    fragPos = inFragPos;
    cout = c;
}
