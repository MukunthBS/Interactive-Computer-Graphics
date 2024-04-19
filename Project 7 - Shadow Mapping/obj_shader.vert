#version 410 core

layout (location = 0) in vec3 iPos;
layout (location = 1) in vec3 iNormal;

out vec3 worldPos;
out vec3 worldNormal;
out vec4 lightViewPos;

uniform mat4 m;
uniform mat3 mN;
uniform mat4 mvp;
uniform mat4 matrixShadow;

void main() {
	
    worldPos = vec3(m * vec4(iPos, 1));
    worldNormal = mN * iNormal;
    lightViewPos = matrixShadow * vec4(iPos, 1);
    gl_Position = mvp * vec4(iPos, 1);
}