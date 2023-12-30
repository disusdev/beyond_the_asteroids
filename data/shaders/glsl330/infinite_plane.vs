#version 330

#include <data/shaders/glsl330/grid_parameters.h>

in vec3 vertexPosition;

out vec2 uv;
out vec2 camPos;

uniform mat4 mvp;
uniform mat4 matView;

void main()
{
	mat4 iview = inverse(matView);
	camPos = vec2(iview[3][0], iview[3][2]);

	vec3 position = vertexPosition * gridSize * iview[3][1];
	position.xz += camPos.xy;

	gl_Position = mvp * vec4(position, 1.0);
	uv = position.xz;
}