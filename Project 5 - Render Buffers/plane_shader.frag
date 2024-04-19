#version 330 core

uniform sampler2D render_tex;

in vec2 frag_txc;

out vec4 color;

void main()
{
	color = vec4(texture2D(render_tex, frag_txc).rgb, 1.0f) + vec4(0.1, 0.1, 0.1, 1.0f);
}