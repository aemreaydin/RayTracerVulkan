#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding=0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 projection;
} ubo;

layout(location=0) in vec4 position;
layout(location=1) in vec4 color;
layout(location=2) in vec2 texCoords;

layout(location=0) out vec4 fragColor;
layout(location=1) out vec2 fragTexCoords;

void main() {
	gl_Position = ubo.projection * ubo.view * ubo.model * position;
	fragTexCoords = texCoords;
	fragColor = color;
}