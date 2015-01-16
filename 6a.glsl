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
uniform mat4 View;

in vec2 uv;
in vec3 position;
in vec3 normal;
out vec4 Color;

void main(void)
{
	vec3 diffuse = vec3(0.15, 0.27, 0.56);

	vec3 n = normalize(normal);
	//vec3 l = vec3(0.0, 1.0, 10.0) - position;
	vec3 l =  vec3(sin(Time) *  10.0, 1.0, cos(Time) * 10.0) - position;

	vec3 v = position - CameraPosition;
	vec3 h = normalize(l-v);
	float n_dot_l = clamp(dot(n, l), 0, 1.0);
	float n_dot_h = clamp(dot(n, h), 0, 1.0);

	vec3 color = diffuse * n_dot_l + vec3(1.0, 1.0, 1.0) *  pow(n_dot_h, 60.0);

	Color = vec4(color, 1.0);
}

#endif
