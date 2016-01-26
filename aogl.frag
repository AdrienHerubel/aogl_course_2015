#version 410 core

#define POSITION	0
#define NORMAL		1
#define TEXCOORD	2
#define FRAG_COLOR	0

precision highp int;

uniform sampler2D Diffuse;
uniform vec3 DiffuseColor;

in block
{
	vec2 TexCoord;
	vec3 Normal;
} In;

layout(location = FRAG_COLOR, index = 0) out vec4 FragColor;

subroutine vec3 diffuseColor();


subroutine (diffuseColor) vec3 diffuseUniform()
{
	return DiffuseColor;
}

subroutine (diffuseColor) vec3 diffuseTexture()
{
	return texture(Diffuse, vec2(In.TexCoord.x, 1. - In.TexCoord.y) ).rgb;
}


subroutine uniform diffuseColor DiffuseColorSub;

void main()
{
	vec3 cdiff = DiffuseColorSub();
	vec3 n = In.Normal;
	vec3 l = vec3(1., 1., 1.);
	float ndotl = clamp(dot(n,l), 0., 1.);
	vec3 diffuse = cdiff * ndotl;
	FragColor = vec4(diffuse, 1.0);
}
