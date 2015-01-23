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

void main()
{	
	for(int i = 0; i < gl_in.length(); ++i)
	{
		vec4 p = gl_in[i].gl_Position;
		//p.y += sin(Time-gl_PrimitiveIDIn);
		gl_Position = MVP * p;
		Out.Texcoord = In[i].Texcoord;
		Out.CameraSpacePosition = vec3(MV * p);
		Out.CameraSpaceNormal = vec3(MV * vec4(In[i].CameraSpaceNormal, 0.0));
		EmitVertex();
	}
	EndPrimitive();
}
