#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

// Vertex attributes
layout(location = 0) in vec3 inVertexPosition;
layout(location = 1) in vec3 inVertexColor;
layout(location = 2) in vec2 inVertexTexCoord;

// Instance attributes
layout(location = 3) in vec3 inInstancePosition;
layout(location = 4) in float inInstanceRotation;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {    
	float s = sin(inInstanceRotation);
	float c = cos(inInstanceRotation);	
	
	mat3 rotMat;
	rotMat[0] = vec3(c, s, 0.0);
	rotMat[1] = vec3(-s, c, 0.0);
	rotMat[2] = vec3(0.0, 0.0, 1.0);
	
	vec3 locPos = inVertexPosition * rotMat;
	vec4 pos = vec4(locPos + inInstancePosition, 1.0);

    gl_Position = ubo.proj * ubo.view * ubo.model * pos;
    fragColor = inVertexColor;
    fragTexCoord = inVertexTexCoord;
}
