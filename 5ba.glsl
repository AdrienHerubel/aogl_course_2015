#if defined(VERTEX)
uniform mat4 Projection;
uniform mat4 View;
uniform mat4 Object;

in vec3 VertexPosition;
in vec3 VertexNormal;
in vec2 VertexTexCoord;

out vData
{
	vec2 uv;
    vec3 normal;
    vec3 position;
}vertex;


void main(void)
{	
	vertex.uv = VertexTexCoord;
	vertex.normal = vec3(Object * vec4(VertexNormal, 1.0));; 
	vertex.position = vec3(Object * vec4(VertexPosition, 1.0));
	vertex.position.x += (gl_InstanceID % 8) * 1.5; 
	vertex.position.y += (gl_InstanceID % 4) * 1.5; 
	vertex.position.z += (int(gl_InstanceID / 8) * 1.5); 

	gl_Position = Projection * View * vec4(vertex.position, 1.0);
}

#endif

#if defined(GEOMETRY)

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in vData
{
	vec2 uv;
    vec3 normal;
    vec3 position;

}vertices[];

out fData
{
	vec2 uv;
    vec3 normal;
    vec3 position;
}frag;    

void main()
{

	int n;
	if (gl_PrimitiveIDIn % 2 == 0)
	{
		for (n = 0; n < gl_in.length(); n++)
		{
			gl_Position = gl_in[n].gl_Position;
			frag.normal = vertices[n].normal;
			frag.position = vertices[n].position;
			frag.uv = vertices[n].uv;
			EmitVertex();
		}
		EndPrimitive();
	}
	else
	{
		for (n = 0; n < gl_in.length(); n++)
		{
			gl_Position = gl_in[n].gl_Position;
			frag.normal = vertices[n].normal;
			frag.position = vertices[n].position;
			frag.uv = vertices[n].uv * 2;
			gl_Position.y += 1.f;
			EmitVertex();
		}
		EndPrimitive();
	}
}

#endif

#if defined(FRAGMENT)
uniform vec3 CameraPosition;

in fData
{
	vec2 uv;
    vec3 normal;
    vec3 position;
};

out vec4 Color;

void main(void)
{
	Color = vec4(uv.s, uv.t, 0.0, 1.0);
	//Color = vec4(normal, 1.0);
	//Color = vec4(position, 1.0);
}

#endif