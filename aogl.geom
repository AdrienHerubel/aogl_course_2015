#version 410 core

#define POSITION	0
#define NORMAL		1
#define TEXCOORD	2
#define FRAG_COLOR	0

precision highp float;
precision highp int;
layout(std140, column_major) uniform;
layout(triangles) in;
layout(triangle_strip, max_vertices = 4) out;

in block
{
	int InstanceId;
	vec2 Texcoord;
	vec3 CameraSpacePosition;
	vec3 CameraSpaceNormal;
} In[]; 

out block
{
	vec2 Texcoord;
	vec3 CameraSpacePosition;
	vec3 CameraSpaceNormal;
}Out;

uniform mat4 MVP;
uniform mat4 MV;
uniform float Time;

const float tstart = 100.0;

void main()
{	
	vec3 center = vec3(0.0, 0.0, 0.0);
	for(int i = 0; i < gl_in.length(); ++i)
	{
		center += gl_in[i].gl_Position.xyz;
	}
	center /= gl_in.length();
	for(int i = 0; i < gl_in.length(); ++i)
	{
		vec4 p = gl_in[i].gl_Position;
		if (Time > tstart)
		{
			p += (Time - tstart) * 0.5 * (mod(gl_PrimitiveIDIn+1,20)) * vec4(center, 0.0);
		}
		gl_Position = MVP * p;
		Out.Texcoord = In[i].Texcoord;
		Out.CameraSpacePosition = vec3(MV * p);
		Out.CameraSpaceNormal = vec3(MV * vec4(In[i].CameraSpaceNormal, 0.0));
		EmitVertex();
	}
	EndPrimitive();
}
