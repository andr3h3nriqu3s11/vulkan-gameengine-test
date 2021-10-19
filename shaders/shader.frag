#version 450

#extension GL_ARB_separate_shader_objects : enable

//layout (binding = 1) uniform sampler2D texSampler;

layout (location = 0) out vec4 outColor;

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 texPos;
layout(location = 2) in float c;



void main() {
//    outColor = vec4(texPos, 0.0, 1.0);
    if (c >= 0.0) {
        outColor = vec4(inColor, 1.0);
        //outColor = vec4(1.0, 0.0, 0.0, 1.0);
    } else {
        //outColor = vec4(1.0, 0.0, 0.0, 1.0);
        //outColor = texture(texSampler, texPos);
    }
    //outColor = vec4(c, 0.0, 1.0, 1.0);
}