#if defined(VERTEX)
uniform mat4 Projection;
uniform mat4 View;
uniform mat4 Object;

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
	gl_Position = Projection * View * Object * vec4(VertexPosition, 1.0);
}

#endif

#if defined(FRAGMENT)
uniform vec3 CameraPosition;
uniform float Time;

in vec2 uv;
in vec3 position;
in vec3 normal;

out vec4 Color;

void main(void)
{
	vec4 color = vec4(0.26, 0.37, 0.85, 1.0);
	float x = uv.x + sin(Time);
	float y = uv.y + cos(Time);

	x = x*50.0 - 20.0 ;
	y = y*50.0 - 20.0;
	y *= 3.14159;

	if (  ( int(float(abs(x - sin(y))))%10 < 4 ) )
	{	
		color = vec4(1.0*x, 0.25 * sin(Time), 0.1 * cos(Time), 1.0);
	}
	Color = color;
}

#endif
