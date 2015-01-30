#version 430 core
#extension GL_ARB_shader_storage_buffer_object : require

#define POSITION	0
#define NORMAL		1
#define TEXCOORD	2
#define COLOR	    0

const float PI = 3.14159265359;
const float TWOPI = 6.28318530718;
const float PI_2 = 1.57079632679;
const float DEG2RAD = TWOPI / 360.0;

precision highp int;

uniform float Time;
uniform sampler2D Diffuse;
uniform sampler2D Specular;
uniform float SpecularPower;

layout(location = COLOR, index = 0) out vec4 Color;
layout(location = NORMAL, index = 0) out vec4 Normal;

in block
{
	vec2 Texcoord;
	vec3 CameraSpacePosition;
	vec3 CameraSpaceNormal;
} In; 

void main()
{
	vec3 n = normalize(In.CameraSpaceNormal);
	if (!gl_FrontFacing)
		n = -n;
	vec3  diffuseColor = texture(Diffuse, In.Texcoord).rgb;
	float specularColor = texture(Specular, In.Texcoord).rrr;
	Color = vec4(diffuseColor, specularColor);
	Normal = vec4(n, SpecularPower);
}