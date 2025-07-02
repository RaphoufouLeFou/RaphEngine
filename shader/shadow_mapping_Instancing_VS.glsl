#version 410 core
layout (location = 0) in vec3 aPos;

uniform mat4 ModelOffsets[128];
uniform mat4 model;

void main()
{
    gl_Position = model * ModelOffsets[gl_InstanceID] * vec4(aPos, 1.0);
}
