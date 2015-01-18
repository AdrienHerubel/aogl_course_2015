#version 410 core

#define POSITION	0
#define NORMAL		1
#define TEXCOORD	2
#define FRAG_COLOR	0

precision highp float;
precision highp int;

uniform mat4 MVP;
uniform mat4 MV;
uniform float Time;

const float pi = 3.14159;

//layout(std140, column_major) uniform;

layout(location = POSITION) in vec3 Position;
layout(location = NORMAL) in vec3 Normal;
layout(location = TEXCOORD) in vec2 Texcoord;

out block
{
	vec2 Texcoord;
	vec3 CameraSpacePosition;
	vec3 CameraSpaceNormal;
} Out;

void main()
{	
	Out.Texcoord = Texcoord;
	vec3 p;
	vec3 n;
	float t = Time * (gl_InstanceID+1);
	float ct = cos(t);
	float st = sin(t); 
	p.x = Position.x * ct + Position.z * st;// + p.x;
	p.z = -Position.x * st + Position.z * ct;// + p.z;
	p.y = Position.y + gl_InstanceID * 1.5;
	n.x = Normal.x * ct + Normal.z * st;// + p.x;
	n.z = -Normal.x * st + Normal.z * ct;
	n.y = Normal.y;
	Out.CameraSpacePosition = p; //vec3(MV * vec4(p, 1.0));
	Out.CameraSpaceNormal = n; //vec3(MV * vec4(n, 0.0));
	gl_Position = vec4(p, 1.0);
}