#version 410 core

in vec3 worldPos;
in vec3 worldNormal;
in vec4 lightViewPos;

out vec4 color;

uniform vec3 camPos;
uniform vec3 spotDir;
uniform vec3 lightPos;
uniform float lightFovRad;
uniform sampler2DShadow shadow;

void main() {

    color = vec4(0, 0, 0, 1);

    vec3 Kd = vec3(1, 0, 0);
    float Intensity_A = 0.2;
    vec3 C_Ambient = Intensity_A * Kd;

    vec3 lightDir = normalize(lightPos - worldPos);
    float angle = max(acos(dot(spotDir, - lightDir)),0);

    if(angle <= lightFovRad) {

        vec3 normal = normalize(worldNormal);
        vec3 viewDir = normalize(camPos - worldPos);
        vec3 halfDir = normalize(lightDir + viewDir);

        vec3 Ks = vec3(1, 1, 1);
        int alpha = 10;
        float Intensity = 1;

        vec3 C_Diffuse = Intensity * max(dot(normal, lightDir), 0.0) * Kd;
        vec3 C_Specular = Intensity * pow(max(dot(normal, halfDir), 0.0), alpha) * Ks;
        vec3 C_Blinn = C_Ambient + C_Diffuse + C_Specular;

        color += vec4(C_Blinn,0);
    }

    color *= textureProj(shadow, lightViewPos);
    color += vec4(C_Ambient, 0);
}