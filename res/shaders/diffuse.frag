#version 330 core

#define INV_PI 0.318309886183790671538

in vec3 P;
in vec3 N;
in vec2 UV;

uniform struct Light {
    vec3 position;
    vec3 color;
} light;

uniform struct Material {
    vec3 color;
} material;

void main() {
    vec3 L = light.position - P;
    float invD2 = 1.0f / dot(L, L);
    
    L *= sqrt(invD2);
    
    vec3 li = light.color * invD2;
    vec3 brdf = material.color * INV_PI;
    vec3 diffuse = brdf * li * max(dot(N, L), 0.0f);
    
    gl_FragColor = vec4(diffuse, 1.0f);
}
