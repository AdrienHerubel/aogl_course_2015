#version 410 core

#define POSITION	0
#define NORMAL		1
#define TEXCOORD	2
#define FRAG_COLOR	0

precision highp float;
precision highp int;

uniform mat4 MVP;

//layout(std140, column_major) uniform;

layout(location = POSITION) in vec3 Position;
layout(location = NORMAL) in vec4 Normal;
layout(location = TEXCOORD) in vec2 Texcoord;
layout(location = TEXCOORD) out vec2 VertTexcoord;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{	
	VertTexcoord = Texcoord;
	gl_Position = MVP * vec4(Position, 1.0);
}