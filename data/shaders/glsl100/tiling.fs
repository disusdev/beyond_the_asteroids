#version 100

precision mediump float;

// Input vertex attributes (from vertex shader)
varying vec2 fragTexCoord;
varying vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec2 tiling;
uniform vec2 offset;

// NOTE: Add here your custom variables

void main()
{
    vec2 texCoord = fragTexCoord * tiling;
    texCoord += offset;    
    gl_FragColor = texture2D(texture0, texCoord);
}