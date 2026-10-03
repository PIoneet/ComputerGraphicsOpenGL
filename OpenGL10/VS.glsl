#version 330 core


layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vColor;

uniform vec2 uOffset;
out vec3 outColor;

void main()
{
    gl_Position = vec4(vPos.xy + uOffset, vPos.z, 1.0);
    outColor = vColor;
}