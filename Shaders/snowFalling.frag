#version 450

uniform vec2 iResolution;           // viewport resolution (in pixels)
uniform float iTime;                 // shader playback time (in seconds)
uniform vec2 iMouse;                // mouse pixel coords. xy: current (if MLB down), zw: click
uniform int num_snow_flakes;
uniform float blizzard_amount;
uniform float turb_amount;

in highp vec3 vert;

out highp vec3 colour;

vec2 uv;
float rnd(float x)
{
    return fract(sin(dot(vec2(x+47.49,38.2467/(x+2.3)), vec2(12.9898, 78.233)))* (43758.5453));
}

float drawCircle(vec2 center, float radius)
{
    return 1.0 - smoothstep(0.0, radius, length(uv - center));
}

void main()
{
	uv = gl_FragCoord.xy / iResolution.x;
	vec2 scaledMouseCoords = iMouse / iResolution;
    
    float j;
    vec3 acc = vec3(0.0);
    for(int i=0; i<num_snow_flakes; i++)
    {
        j = float(i);
        float speed = 0.3+rnd(cos(j))*(0.7+0.5*cos(j/(float(num_snow_flakes)*0.25)));
        vec2 center = vec2((0.25-uv.y)*blizzard_amount+2.0*rnd(j)+turb_amount*cos(iTime+sin(j)) - 0.025*scaledMouseCoords.x, mod(sin(j)-speed*(iTime*1.5*(0.1+blizzard_amount)), 15.0) + 0.025*scaledMouseCoords.y);
        acc += vec3(0.39*drawCircle(center, 0.001+speed*0.008));
    }
	colour = acc;
}
