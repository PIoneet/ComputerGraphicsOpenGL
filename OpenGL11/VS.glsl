#version 330 core
layout (location = 0) in vec2 vPos;

uniform vec2 uOffset;   // 칸 중심 좌표 (NDC)
uniform float uSize;    // 도형 반폭

void main()
{
    gl_Position = vec4(vPos * uSize + uOffset, 0.0, 1.0);
}