#version 410 core

layout (location = 0) in vec3 iPos;
layout (location = 1) in vec3 iNormal;

uniform mat4 mvp;

void main() {
	
    gl_Position = mvp * vec4(iPos, 1);
}