#version 330

#include <data/shaders/glsl330/grid_parameters.h>
#include <data/shaders/glsl330/grid_calculation.h>

in vec2 uv;
in vec2 camPos;

out vec4 finalColor;

void main()
{
	finalColor = gridColor(uv, camPos);
}