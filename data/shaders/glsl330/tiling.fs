#version 330 core

uniform sampler2D diffuseMap;
uniform vec2 tiling;
uniform vec2 offset;

in vec2 fragTexCoord;

out vec4 fragColor;

void main()
{
    vec2 texCoord = fragTexCoord * tiling;
    texCoord += offset;
    fragColor = texture(diffuseMap, texCoord);
}
