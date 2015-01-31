#version 410 core

const float PI = 3.14159265359;
const float TWOPI = 6.28318530718;
const float PI_2 = 1.57079632679;
const float DEG2RAD = TWOPI / 360.0;

in block
{
	vec2 Texcoord;
} In; 

uniform sampler2D ColorBuffer;
uniform sampler2D NormalBuffer;
uniform sampler2D DepthBuffer;

layout(location = 0, index = 0) out vec4 Color;

uniform mat4 InverseProjection;

uniform light
{
	vec3 Position;
	float Angle;
	vec3 Direction;
	float PenumbraAngle;
	vec3 Color;
	float Intensity;
} SpotLight;

vec3 spotLight( in vec3 p, in vec3 n, in vec3 v, in vec3 diffuseColor, in vec3 specularColor, in float specularPower)
{
	vec3 l = normalize(SpotLight.Position - p);
	float a = cos(SpotLight.Angle * DEG2RAD);
	float pa = cos(SpotLight.PenumbraAngle * DEG2RAD);
	float ndotl = max(dot(n, l), 0.0);
	float ldotd = dot(-l, normalize(SpotLight.Direction));
	vec3 h = normalize(l+v);
	float ndoth = max(dot(n, h), 0.0);
	float fallof = clamp(pow( (ldotd - a) / (a-pa), 4), 0.0, 1.0);
	float d = distance(SpotLight.Position, p);
	float att = 1.f / (d*d);
	return att * fallof * SpotLight.Color * SpotLight.Intensity * (diffuseColor * ndotl + specularColor * pow(ndoth, specularPower));
}

void main(void)
{
	vec4 colorBuffer = texture(ColorBuffer, In.Texcoord).rgba;
	vec4 normalBuffer = texture(NormalBuffer, In.Texcoord).rgba;
	float depth = texture(DepthBuffer, In.Texcoord).r;

	vec3 n = normalBuffer.rgb;
	vec3 diffuseColor = colorBuffer.rgb;
	vec3 specularColor = colorBuffer.aaa;
	float specularPower = normalBuffer.a;

	vec2 xy = In.Texcoord * 2.0 -1.0;
	vec4 wP = InverseProjection * vec4(xy, depth * 2.0 -1.0, 1.0);
	vec3 p = vec3(wP.xyz / wP.w);
	vec3 v = normalize(-p);

	Color = vec4(spotLight(p, n, v, diffuseColor, specularColor, specularPower), 1.0);
}