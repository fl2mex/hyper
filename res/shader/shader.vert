#version 450
#extension GL_EXT_buffer_reference : require

struct Vertex {
	vec3 position;
	vec3 normal;
	vec4 color;
	vec2 uv;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer { 
	Vertex vertices[];
};

layout(binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 proj;
} ubo;

layout(push_constant) uniform PushConstants {
	VertexBuffer vertexBuffer;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
	Vertex v = pc.vertexBuffer.vertices[gl_VertexIndex];

	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(v.position, 1.0);
	fragColor = v.color.rgb;
	fragTexCoord = v.uv;
}