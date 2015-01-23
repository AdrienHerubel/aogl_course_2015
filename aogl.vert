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
uniform int InstanceCount;

const float PI = 3.14159265359;
const float TWOPI = 6.28318530718;
const float PI_2 = 1.57079632679;

const float SPHERE_RADIUS = 10.0;

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
	vec3 p = Position;
	vec3 n = Normal;
	float t = Time + gl_InstanceID;
	float ct = cos(t);
	float st = sin(t);
	p.x = Position.x * ct + Position.z * st;// + p.x;
	p.z = -Position.x * st + Position.z * ct;// + p.z;
	// p.y = Position.y + gl_InstanceID * 1.5;
	n.x = Normal.x * ct + Normal.z * st;// + p.x;
	n.z = -Normal.x * st + Normal.z * ct;
	n.y = Normal.y;

	float theta_sub;
	float phi_sub;
	theta_sub = phi_sub = sqrt(InstanceCount);
	float theta = floor(gl_InstanceID/theta_sub)  * (PI/theta_sub) - PI_2;
	float phi = (gl_InstanceID - (phi_sub * floor(gl_InstanceID/phi_sub)))   * (TWOPI/phi_sub) - PI; 
	float ctheta = cos(theta);
	float stheta = sin(theta);
	float cphi = cos(phi);
	float sphi = sin(phi);

	p.x +=  SPHERE_RADIUS * ctheta * cphi;
	p.z +=  SPHERE_RADIUS * ctheta * sphi;
	p.y +=  SPHERE_RADIUS * stheta;
	Out.CameraSpacePosition = p; 
	Out.CameraSpaceNormal = n; 
	gl_Position = vec4(p, 1.0);
}