#version 410 core

in block
{
	vec2 Texcoord;
} In; 

uniform sampler2D Texture1;

layout(location = 0, index = 0) out vec4  Color;

void main(void)
{
	vec3 color = texture(Texture1, Texcoord).rgb;
	Color = vec4(color, 1.0);
}