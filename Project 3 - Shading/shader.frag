#version 330 core

in vec4 frag_pos;
in vec3 frag_norm;

out vec4 color;

void main()
{
	vec3 light_dir = normalize(vec3(2, 2, 5));
	float alpha = 50.0;

	vec3 v = vec3(normalize(frag_pos)) * vec3(-1, -1, -1);
	vec3 h = normalize(light_dir + v);

	float cos_theta = max(0.0, dot(normalize(frag_norm), light_dir));
	float cos_phi = max(0.0, dot(normalize(frag_norm), h));

	vec3 I = vec3(0.8, 0.8, 0.8);
	vec3 K_s = vec3(1, 1, 1);
	vec3 K_d = vec3(0.8, 0, 0);
	vec3 K_a = vec3(0.2, 0, 0);

	// color = vec4(frag_norm, 1);		                 			// surface normal
	// color = vec4(I * (K_a + (cos_theta * K_d)), 1); 		      	// diffuse + ambient
	// color = vec4(I * (K_s * pow(cos_phi, alpha)), 1);  			// spectral
	color = vec4(I * (K_a + (cos_theta * K_d) + (K_s * pow(cos_phi, alpha))), 1);
}