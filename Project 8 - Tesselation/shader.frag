#version 410 core

in vec3 worldPos;
in vec4 lightViewPos;
in vec2 texCoord;

out vec4 color;

uniform vec3 camPos;
uniform vec3 spotDir;
uniform vec3 lightPos;
uniform float lightFovRad;
uniform bool hasDisp;
uniform sampler2DShadow shadowMap;
uniform sampler2D normalMap;

void main() {
    color = vec4(0, 0, 0, 1);

    //Compute Ambient
    vec3 Kd = vec3(1, 0, 0);
    float Intensity_A = 0.2;
    vec3 C_Ambient = Intensity_A * Kd;

    //Determine if light is on this fragment;
    vec3 lightDir = normalize(lightPos - worldPos);
    float angle = max(acos(dot(spotDir, -lightDir)), 0);
    if (angle <= lightFovRad) {
        //Compute Diffuse, Specular and Blinn
        vec3 normal = normalize(texture(normalMap, texCoord).rgb * 2.0f - 1.0f);
        vec3 viewDir = normalize(camPos - worldPos);
        vec3 halfDir = normalize(lightDir + viewDir);

        vec3 Ks = vec3(1, 1, 1);
        int alpha = 10;
        float Intensity = 1;

        vec3 C_Diffuse = Intensity * max(dot(normal, lightDir), 0.0) * Kd;
        vec3 C_Specular = Intensity * pow(max(dot(normal, halfDir), 0.0), alpha) * Ks;
        vec3 C_Blinn = C_Ambient + C_Diffuse + C_Specular;

        color += vec4(C_Blinn, 0);
    }

    //Compute Shadow
    if (hasDisp) color *= textureProj(shadowMap, lightViewPos);
    color += vec4(C_Ambient, 0);
}
