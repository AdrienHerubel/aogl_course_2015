#version 410 core

in block
{
	vec2 Texcoord;
} In; 

uniform sampler2D Texture;

uniform mat3 G[2] = mat3[](
	mat3( 1.0, 2.0, 1.0, 0.0, 0.0, 0.0, -1.0, -2.0, -1.0 ),
	mat3( 1.0, 0.0, -1.0, 2.0, 0.0, -2.0, 1.0, 0.0, -1.0 )
);

layout(location = 0, index = 0) out vec4  Color;

void main(void)
{
	mat3 I;
	float cnv[2];
	vec3 s;
	
	/* fetch the 3x3 neighbourhood and use the RGB vector's length as intensity value */
	for (int i=0; i<3; i++)
	for (int j=0; j<3; j++) {
		s = texelFetch( Texture, ivec2(gl_FragCoord) + ivec2(i-1,j-1), 0 ).rgb;
		I[i][j] = length(s); 
	}
	
	/* calculate the convolution values for all the masks */
	for (int i=0; i<2; i++) {
		float dp3 = dot(G[i][0], I[0]) + dot(G[i][1], I[1]) + dot(G[i][2], I[2]);
		cnv[i] = dp3 * dp3; 
	}

	Color = vec4(texelFetch( Texture, ivec2(gl_FragCoord), 0 ).rgba - 2.0 * sqrt(cnv[0]*cnv[0]+cnv[1]*cnv[1]));
}