#version 330 core

layout(location = 0) in vec4 aPos;
out vec2 UV;
void main()
{
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
    int UVcompact = int(aPos.w);
    float x = UVcompact / 10;
    float y = UVcompact % 10;
    UV = vec2(x, y);
}
