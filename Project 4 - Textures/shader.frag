#version 400 core

uniform sampler2D tex_a;
uniform sampler2D tex_d;
uniform sampler2D tex_s;
uniform vec3 K_a;
uniform vec3 K_d;
uniform vec3 K_s;

in vec4 frag_pos;
in vec3 frag_norm;
in vec2 frag_txc;

out vec4 color;

void main()
{
	
	vec3 light_dir = normalize(vec3(2, 2, 5));
	float alpha = 25.0;

	vec3 v = vec3(normalize(frag_pos)) * vec3(-1, -1, -1);
	vec3 h = normalize(light_dir + v);

	float cos_theta = max(0.0, dot(normalize(frag_norm), light_dir));
	float cos_phi = max(0.0, dot(normalize(frag_norm), h));

	vec3 I = vec3(1, 1, 1);

	vec3 ambient_tex = texture2D(tex_a, frag_txc).rgb;
	vec3 diffuse_tex =  texture2D(tex_d, frag_txc).rgb;
	vec3 specular_tex =  texture2D(tex_s, frag_txc).rgb;

	color = vec4(I * (K_a * ambient_tex + (cos_theta * K_d * diffuse_tex) + (K_s * pow(cos_phi, alpha) * specular_tex)), 1);
}