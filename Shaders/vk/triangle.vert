#version 450 core

layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec4 Color;


void main ()
{
	gl_Position = vec4(Position.xyz, 1.0f);
    Color = vec4(inColor, 1.0f);
}
