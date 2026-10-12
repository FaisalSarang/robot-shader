#version 330 core

// Claude AI: skeleton provided by AI (inputs, uniforms, structure and the diffuse line).
// I filled in the L, V and H vectors, the specular dot product and the specular term of the formula.

in vec3 vWorldPos;
in vec3 vNormal;

// light and camera
uniform vec3 uLightPos;    // where the light is
uniform vec3 uViewPos;     // where the camera is
uniform vec3 uLightColor;  // Il
uniform vec3 uAmbient;     // Ia

// material
uniform float uKa;
uniform vec3  uKd;         // the model's colour
uniform float uKs;
uniform float uShininess;  // alpha

out vec4 fragColor;

void main() {
    // 1. the four vectors
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightPos - vWorldPos);   // pixel -> light: (light position) minus (pixel position)
    vec3 V = normalize(uViewPos - vWorldPos);   // pixel -> camera: same idea
    vec3 H = normalize(L+V);            // halfway between L and V

    // 2. the two max terms
    float diffuse  = max(dot(N, L), 0.0);
    float specular = pow(max(dot(N, H), 0.0), uShininess);

    // 3. put the formula together
    vec3 color = (uKa * uAmbient + uKd * diffuse) * uLightColor + (uKs * uLightColor * specular);

    fragColor = vec4(color, 1.0);   // 1.0 = fully opaque
}