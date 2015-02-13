#version 410 core

in block
{
	vec2 Texcoord;
} In; 

uniform sampler2D Color;
uniform sampler2D Blur;
uniform sampler2D CoC;

layout(location = 0, index = 0) out vec4 OutColor;

void main(void)
{
	float blurCoef = texture(CoC, In.Texcoord).r;
	OutColor = vec4(mix(texture(Color, In.Texcoord).rgb, texture(Blur, In.Texcoord).rgb, blurCoef), 1.0);
}