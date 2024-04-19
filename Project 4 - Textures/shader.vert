#version 400 core

layout(location=0) in vec3 pos;
layout(location=1) in vec3 norm;
layout(location=2) in vec3 txc;

uniform mat4 mvp;
uniform mat4 mvn;

out vec4 frag_pos;
out vec3 frag_norm;
out vec2 frag_txc;

void main()
{
	frag_pos = mvn * vec4(pos, 1);
	frag_norm = (transpose(inverse(mvn)) * vec4(norm,0)).xyz;
	frag_txc = vec2(txc.x, 1.0 - txc.y);

	gl_Position = mvp * vec4(pos, 1);
}