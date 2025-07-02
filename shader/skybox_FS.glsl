#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;

void main()
{    
    vec3 NewTexCoords;
    NewTexCoords.x = TexCoords.x;
    NewTexCoords.y = TexCoords.z;
    NewTexCoords.z = TexCoords.y;
    FragColor = texture(skybox, NewTexCoords);
}