#version 330 core

in vec3 frag_norm;
in vec4 frag_pos;

uniform samplerCube cubemap;
uniform mat4 mv;

out vec4 color;

void main()
{
	vec3 light_dir = normalize(vec3(1,1,5));
	float alpha = 18.0;

	vec3 v = normalize(vec3(normalize(frag_pos)) * vec3(-1,-1,-1));
	vec3 h = normalize(light_dir + v);

	float cos_theta = max(0, dot(normalize(frag_norm), light_dir));
	float cos_phi = max(0, dot(normalize(frag_norm), h));

	vec3 ref = vec3(transpose(mv) * vec4(reflect(v, normalize(frag_norm)),1));
	

	vec4 I = vec4(0.6,0.6,0.6,1);
	vec4 K_d = texture(cubemap, ref); // make sure reflect vector is in world instead of view
	vec4 r = I * ( (cos_theta * K_d) + ( pow(cos_phi, alpha)) );

	color = vec4(r.x,r.y,r.z,1);
}