// #version 330 core

// in vec3 frag_pos;
// in vec3 frag_norm;

// uniform samplerCube cubemap;
// uniform sampler2DRect renderTex;

// out vec4 color;

// void main()
// {

// 	vec3 camera = vec3(0, 0, -10);
// 	vec2 tex = vec2(gl_FragCoord.x, gl_FragCoord.y);
// 	vec3 I = normalize(frag_pos - camera);
//     vec3 R = reflect(I, normalize(frag_norm));
// 	vec4 cubemap_tex = texture(cubemap, R).rgba;
// 	vec4 render_tex = texture(renderTex, tex).rgba;

// 	color = cubemap_tex + render_tex;
// }

#version 330 core

in vec3 frag_norm;
in vec4 frag_pos;
in vec2 frag_txc;

uniform samplerCube cubemap;
uniform mat4 mv;
uniform sampler2D renderTex;

out vec4 color;

void main()
{
	// vec3 camera = vec3(0, 0, -10);
	vec3 light_dir = normalize(vec3(1,1,5));
	float alpha = 18.0;

	vec3 v = vec3(normalize(frag_pos)) * vec3(-1,-1,-1);	
	vec3 h = normalize(light_dir + v);

	float cos_theta = max(0, dot(normalize(frag_norm), light_dir));
	float cos_phi = max(0, dot(normalize(frag_norm), h));

	vec3 ref = vec3(transpose(mv) * vec4(reflect(-v, normalize(frag_norm)),1));

	vec4 I = vec4(0.5,0.5,0.5,1);
	vec4 K_d =  vec4(texture(cubemap, ref).rgb, 1.0f) + vec4(texture(renderTex, frag_txc).rgb, 1.0f);
	vec4 r = I * ( (cos_theta * K_d) + (pow(cos_phi, alpha)) );

	color = vec4(r.x,r.y,r.z,1);
	
	// color = K_d;
}
