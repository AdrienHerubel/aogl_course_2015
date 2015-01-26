#version 410 core

#define POSITION	0
#define NORMAL		1
#define TEXCOORD	2
#define FRAG_COLOR	0

precision highp int;

uniform float Time;
uniform sampler2D Diffuse;
uniform sampler2D Specular;
uniform vec3 Light;
uniform float SpecularPower;

layout(location = FRAG_COLOR, index = 0) out vec4 FragColor;

in block
{
	vec2 Texcoord;
	vec3 CameraSpacePosition;
	vec3 CameraSpaceNormal;
} In; 

vec3 pointLight( in vec3 n, in vec3 v, in vec3 diffuseColor, in vec3 specularColor, in float specularPower)
{
	vec3 l = normalize(Light - In.CameraSpacePosition);
	float ndotl =  max(dot(n, l), 0.0);
	vec3 h = normalize(l+v);
	float ndoth = max(dot(n, h), 0.0);
	return diffuseColor * ndotl + specularColor * pow(ndoth, SpecularPower);
}

void main()
{
	vec3 n = normalize(In.CameraSpaceNormal);
	vec3 v = normalize(-In.CameraSpacePosition);
	vec3 diffuseColor = texture(Diffuse, In.Texcoord).rgb;
	vec3 specularColor = texture(Specular, In.Texcoord).rrr;

	FragColor = vec4(pointLight(n, v, diffuseColor, specularColor, SpecularPower), 1.0);

	// FragColor = vec4(vec3(ndotl), 1.0);
	// FragColor = vec4(vec3(n), 1.0);
	// FragColor = vec4(vec3(l), 1.0);
}
