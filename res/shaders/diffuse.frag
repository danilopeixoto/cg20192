#version 330 core

in vec3 P;
in vec3 N;

void main() {
    gl_FragColor = vec4(N, 1.0f);
}
