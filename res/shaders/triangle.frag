#version 330 core

in vec3 C;

void main() {
    gl_FragColor = vec4(C, 1.0f);
}
