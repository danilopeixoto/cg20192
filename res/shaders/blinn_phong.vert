#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 textureCoordinate;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 P;
out vec3 N;
out vec2 UV;

void main() {
    P = (model * vec4(position, 1.0f)).xyz;
    N = normalize((transpose(inverse(model)) * vec4(normal, 0.0f)).xyz);
    UV = textureCoordinate;
    
    gl_Position = projection * view * vec4(P, 1.0f);
}
