#version 330 core

#define PI     3.14159265358979323846
#define INV_PI 0.31830988618379067154

in vec3 P;   // Surface position
in vec3 N;   // Surface normal
in vec2 UV;  // Surface UV coordinate

// Surface material attributes
uniform struct Material {
    vec3 color;
    float exponent;
} material;

// Light attributes
uniform struct Light {
    vec3 position;
    vec3 color;
} light;

// Camera attributes
uniform struct Camera {
    vec3 position;
} camera;

uniform sampler2D image;

// Lambert material implementation (diffuse)
void main() {
    vec2 uv = vec2(UV.x, -UV.y);
    
    vec3 L = light.position - P;
    float invD2 = 1.0f / dot(L, L);
    
    L *= sqrt(invD2);
    
    vec3 li = light.color * invD2;
    vec3 brdf = material.color * texture(image, uv).rgb * INV_PI;
    vec3 diffuse = brdf * li * max(dot(N, L), 0.0f);
    
    gl_FragColor = vec4(diffuse, 1.0f); // Output color
}
