#version 410 core

layout(quads, equal_spacing, ccw) in;

in vec2 texCoordTS[];

out vec2 texCoord;
out vec3 worldPos;
out vec4 lightViewPos;

uniform mat4 m;
uniform mat4 matrixShadow;
uniform mat4 mvp;
uniform sampler2D dispMap;
uniform float dispSize;

vec2 interpolate(vec2 v0, vec2 v1, vec2 v2, vec2 v3) {
    vec2 a = mix(v0, v1, gl_TessCoord.x);
    vec2 b = mix(v3, v2, gl_TessCoord.x);
    return mix(a, b, gl_TessCoord.y);
}

vec4 interpolate(vec4 v0, vec4 v1, vec4 v2, vec4 v3) {
    vec4 a = mix(v0, v1, gl_TessCoord.x);
    vec4 b = mix(v3, v2, gl_TessCoord.x);
    return mix(a, b, gl_TessCoord.y);
}

void main() {
    vec4 p = interpolate(
            gl_in[0].gl_Position,
            gl_in[1].gl_Position,
            gl_in[2].gl_Position,
            gl_in[3].gl_Position
        );

    texCoord = interpolate(
            texCoordTS[0],
            texCoordTS[1],
            texCoordTS[2],
            texCoordTS[3]
        );

    p.z += texture(dispMap, texCoord).y * dispSize * 2.0f - dispSize / 2.0f;

    worldPos = vec3(m * p);
    lightViewPos = matrixShadow * p;
    gl_Position = mvp * p;
}
