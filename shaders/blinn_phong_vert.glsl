#version 330 core

// Vertex shader written by me, with explanations, a skeleton for main() and corrections from Claude AI

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;

out vec3 vWorldPos;
out vec3 vNormal;

void main() {
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal = uNormalMatrix * aNormal;
    gl_Position = uProjection*uView*worldPos;
}