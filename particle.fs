#version 400 core

in float d;

out vec4 Color;

uniform vec3 hsv;

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main()
{
	Color = vec4(hsv2rgb(vec3(hsv.x - d / 7, hsv.y, hsv.z)), 1.0);
}
