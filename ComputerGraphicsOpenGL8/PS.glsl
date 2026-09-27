#version 330 core


in vec3 outColor;

out vec4 FragColor;

void main()
{ 
	vec3 color = outColor;
	FragColor = vec4(color, 1.0); 
}