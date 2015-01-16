#if defined(VERTEX)
uniform mat4 Projection;
uniform mat4 View;
uniform mat4 Object;
uniform float Time;

in vec3 VertexPosition;
in vec3 VertexNormal;
in vec2 VertexTexCoord;

out vec2 uv;
out vec3 normal;
out vec3 position;

void main(void)
{	
	uv = VertexTexCoord;
	normal = vec3(Object * vec4(VertexNormal, 1.0));; 
	position = vec3(Object * vec4(VertexPosition, 1.0));
	if (gl_VertexID % 2 != 0)
		gl_Position = Projection * View * Object * vec4(VertexPosition.x + sin(Time) * 2.f, VertexPosition.y * sin(Time), VertexPosition.z + cos(Time) * 2.f, 1.0);
	else
		gl_Position = Projection * View * Object * vec4(VertexPosition, 1.0);
}

#endif

#if defined(FRAGMENT)
uniform vec3 CameraPosition;

in vec2 uv;
in vec3 position;
in vec3 normal;

out vec4 Color;

void main(void)
{
	Color = vec4(uv.s, uv.t, 0.0, 1.0);
}

#endif
