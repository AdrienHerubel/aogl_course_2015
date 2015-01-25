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

void main()
{
#define EX4
#if defined(EX1)
	//FragColor = vec4(1.0, 1.0, 0.0, 1.0);
	//FragColor = vec4(GeomTexcoord.s, GeomTexcoord.t, 0.0, 1.0);
	FragColor = vec4(In.CameraSpacePosition.s, In.CameraSpacePosition.t, 0.0, 1.0);
	FragColor = vec4(In.CameraSpaceNormal.s, In.CameraSpaceNormal.t, 0.0, 1.0);
	FragColor = vec4(In.Texcoord.s, In.Texcoord.t, 0.0, 1.0);
#endif
#if defined(EX2)
	vec3 color = vec3(0.26, 0.37, 0.85);
	float x = In.Texcoord.x * cos(Time) + In.Texcoord.y * sin(Time);
	float y = In.Texcoord.x * sin(Time) - In.Texcoord.y + cos(Time);

	x = x*50.0 - 20.0 ;
	y = y*50.0 - 20.0;
	y *= 3.14159;

	if (  ( int(float(abs(x - sin(y))))%6 < 5 ) )
	{	
		color = vec3(0.25 * sin(Time*2.0), 0.25 * sin(Time), 0.1 * cos(Time));
	}
	FragColor = vec4(color, 1.0);
#endif
#if defined(EX3)
	vec3 color = mix(texture(Diffuse, In.Texcoord).rgb, texture(Specular, In.Texcoord).rrr, 0.7); ;
	FragColor = vec4(color, 1.0);
#endif
#if defined(EX4)
	vec3 n = normalize(In.CameraSpaceNormal);
	float ndotl =  dot(n, Light);
	vec3 h = normalize(Light-normalize(In.CameraSpacePosition));
	float ndoth = dot(n, h);
	vec3 color = texture(Diffuse, In.Texcoord).rgb * ndotl + texture(Specular, In.Texcoord).rrr * pow(ndoth, SpecularPower);
	FragColor = vec4(color, 1.0);
#endif
}
