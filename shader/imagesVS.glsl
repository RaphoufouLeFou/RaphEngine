#version 330 core

layout(location = 0) in vec4 aPos;

uniform mat4 projection;

out vec2 UV;


void main()
{
    gl_Position = projection * vec4(aPos.xyz, 1.0);
    int UVcompact = int(aPos.w);
    float x = UVcompact / 10;
    float y = UVcompact - x * 10;
    UV = vec2(x, y);
}
