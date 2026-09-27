#version 330 core


in vec3 outColor;

out vec4 FragColor;

uniform bool useOverrideColor;
uniform vec3 overrideColor;

void main()
{ 
	vec3 color = useOverrideColor ? overrideColor : outColor;
	FragColor = vec4(color, 1.0); 
}