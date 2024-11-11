
#version 330 core

in vec2 UV;

out vec4 FragColor;

uniform sampler2D TextureSampler;


void main()
{
	FragColor = texture(TextureSampler, UV).rgba;
}