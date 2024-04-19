#version 330 core

layout (location = 0) in vec3 pos;

uniform mat4 mv;
uniform mat4 mvp;

out vec4 frag_pos;
out vec3 frag_norm;
out vec2 frag_txc;

void main()
{	
	frag_pos = mv * vec4(pos, 1);
	frag_norm = (transpose(inverse(mv)) * vec4(0.0f, 0.0f, 1.0f, 0.0f)).xyz;
	frag_txc = vec2(pos.xy + vec2(1,1))/2.0;

	gl_Position = mvp * vec4(pos, 1);
}
