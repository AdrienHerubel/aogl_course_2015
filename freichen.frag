#version 410 core

in block
{
	vec2 Texcoord;
} In; 

uniform sampler2D Texture;
uniform float Factor = 1.0;

uniform mat3 G[9] = mat3[](
	1.0/(2.0*sqrt(2.0)) * mat3( 1.0, sqrt(2.0), 1.0, 0.0, 0.0, 0.0, -1.0, -sqrt(2.0), -1.0 ),
	1.0/(2.0*sqrt(2.0)) * mat3( 1.0, 0.0, -1.0, sqrt(2.0), 0.0, -sqrt(2.0), 1.0, 0.0, -1.0 ),
	1.0/(2.0*sqrt(2.0)) * mat3( 0.0, -1.0, sqrt(2.0), 1.0, 0.0, -1.0, -sqrt(2.0), 1.0, 0.0 ),
	1.0/(2.0*sqrt(2.0)) * mat3( sqrt(2.0), -1.0, 0.0, -1.0, 0.0, 1.0, 0.0, 1.0, -sqrt(2.0) ),
	1.0/2.0 * mat3( 0.0, 1.0, 0.0, -1.0, 0.0, -1.0, 0.0, 1.0, 0.0 ),
	1.0/2.0 * mat3( -1.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, -1.0 ),
	1.0/6.0 * mat3( 1.0, -2.0, 1.0, -2.0, 4.0, -2.0, 1.0, -2.0, 1.0 ),
	1.0/6.0 * mat3( -2.0, 1.0, -2.0, 1.0, 4.0, 1.0, -2.0, 1.0, -2.0 ),
	1.0/3.0 * mat3( 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 )
);

layout(location = 0, index = 0) out vec4  Color;

void main(void)
{
	// vec3 color = texture(Texture, In.Texcoord).rgb;
	mat3 I;
	float cnv[9];
	vec3 s;
	
	/* fetch the 3x3 neighbourhood and use the RGB vector's length as intensity value */
	for (int i=0; i<3; i++)
	for (int j=0; j<3; j++) {
		s = texelFetch( Texture, ivec2(gl_FragCoord) + ivec2(i-1,j-1), 0 ).rgb;
		I[i][j] = length(s); 
	}
	
	/* calculate the convolution values for all the masks */
	for (int i=0; i<9; i++) {
		float dp3 = dot(G[i][0], I[0]) + dot(G[i][1], I[1]) + dot(G[i][2], I[2]);
		cnv[i] = dp3 * dp3; 
	}

    float M = (cnv[4] + cnv[5]) + (cnv[6] + cnv[7]); // Line detector
    float S = (cnv[0] + cnv[1]) + (cnv[2] + cnv[3]) + M + cnv[8]; 
	Color = vec4(texelFetch( Texture, ivec2(gl_FragCoord), 0 ).rgb - Factor * vec3(sqrt(M/S)), 1.0);
}