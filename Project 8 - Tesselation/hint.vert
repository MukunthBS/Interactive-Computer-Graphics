#version 410 core
layout(location = 0) in vec3 iPos;

uniform mat4 vp;
uniform mat4 m;

void main()
{
    gl_Position = vp * m * vec4(iPos, 1);
}
