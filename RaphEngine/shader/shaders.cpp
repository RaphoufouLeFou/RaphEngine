#include "shaders.h"
#include "pch.h"

const char* DEBUG_FS_shader = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture_diffuse1;

void main()
{
    FragColor = texture(texture_diffuse1, TexCoords);
}
)";

const char* debug_quad_depth_FS_shader = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D depthMap;
uniform float near_plane;
uniform float far_plane;

// required when using a perspective projection matrix
float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // Back to NDC 
    return (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));
}

void main()
{             
    float depthValue = texture(depthMap, TexCoords).r;
    //FragColor = vec4(vec3(LinearizeDepth(depthValue) / far_plane), 1.0); // perspective
    FragColor = vec4(vec3(depthValue), 1.0); // orthographic
}
)";

const char* debug_quad_VS_shader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos, 1.0);
}
)";

const char* DEBUG_VS_shader = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;

out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* fragmentShader_shader = R"(
#version 330 core

// Interpolated values from the vertex shaders
in vec2 UV;
in vec3 FragPos;
in vec3 Normal;
in vec4 FragPosLightSpace;
in float distToCamera;

// Ouput data
out vec3 FragColor;

// Values that stay constant for the whole mesh.
uniform sampler2D myTextureSampler;
uniform sampler2D NormalMap;
uniform int RenderDistance;
uniform vec3 lightPos[256];
uniform vec3 lightColor[256];
uniform vec3 lightSettings[256];
uniform vec3 ObjectPosition;
uniform int lightCount;
uniform vec3 fogColor;
uniform bool isTerrain;

void main() {

	vec3 norm = normalize(Normal);

	vec3 diffuse = vec3(0, 0, 0);
	for (int i = 0; i < lightCount; i++) {
		if (lightSettings[i].x <= 1.5) {
			vec3 lightDir = normalize(lightPos[i] - FragPos);
			float diff = max(dot(norm, lightDir), 0.0);
			float distToLight = distance(ObjectPosition, lightPos[i]);
			float attenuation = lightSettings[i].y / (distToLight * distToLight);
			if (attenuation > 1) attenuation = 1;
			diffuse += diff * lightColor[i] * attenuation;
		}
		else if (lightSettings[i].x <= 2.5) {
			// directional light
			// here the light rotation is stored in the lightPos
			// and the color is white
			vec3 lightDir = normalize(lightPos[i]);
			float diff = max(dot(norm, lightDir), 0.0);
			diffuse += diff * lightColor[i] * lightSettings[i].y;
		}
	}

	float ambientStrength = 0.1;
	vec3 ambient = ambientStrength * vec3(1, 1, 1);

	vec3 objectColor = texture(myTextureSampler, UV).rgb;
	FragColor = (ambient + diffuse) * objectColor;
	
}
)";

const char* imagesFS_shader = R"(

#version 330 core

in vec2 UV;

out vec4 FragColor;

uniform sampler2D TextureSampler;


void main()
{
	FragColor = texture(TextureSampler, UV).rgba;
}
)";

const char* imagesVS_shader = R"(
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

)";

const char* shadow_mappingFS_shader = R"(
#version 330 core
out vec4 FragColor;

in VS_OUT{
    vec3 FragPos;
    vec3 NormalPos;
    vec2 TexCoords;
    vec3 TangentLightPos;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
    vec4 FragPosLightSpace;
} fs_in;


uniform sampler2D diffuseTexture;
uniform sampler2D shadowMap;
uniform sampler2D normalMap;

uniform vec4 lightPos;
uniform vec3 viewPos;
uniform vec2 textureScale;
uniform vec3 lightDir;

uniform bool HaveNormalMap;

float ShadowCalculation(vec4 fragPosLightSpace, vec3 Normal)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // calculate bias (based on depth map resolution and slope)
    vec3 normal = normalize(Normal);
    float bias = max(0.001 * (1.0 - dot(normal, lightDir)), 0.000005);
    // check whether current frag pos is in shadow
    // float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias >= pcfDepth  ? 1.0 : 0.0;
        }    
    }
    shadow /= 9.0;
    
    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)
        shadow = 0.0;
    if(shadow < 0.5)
		shadow = 0.0;
        
    return shadow;
}

void main()
{           
    vec3 color = texture(diffuseTexture, fs_in.TexCoords * textureScale).rgb;
    vec3 normal = normalize(fs_in.NormalPos);
    if (HaveNormalMap)
    {
        normal = texture(normalMap, fs_in.TexCoords * textureScale).rgb;
        normal = normalize(normal * 2.0 - 1.0);
    }
    vec3 lightColor = vec3(1.0);
    // ambient
    vec3 ambient = 0.1 * lightColor;
    // diffuse
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;
    // specular

    vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    vec3 specular = vec3(0.2) * spec;

    // calculate shadow
    float shadow = ShadowCalculation(fs_in.FragPosLightSpace, normal);
    vec3 lighting = (ambient + vec3(1 - shadow) * (diffuse + specular)) * color;
    
    //FragColor = vec4(lighting, 1.0);
    FragColor = vec4(1, 0, 0, 1);
}
)";

const char* shadow_mappingVS_shader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec3 aNormal;
layout (location = 1) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out VS_OUT {
    vec3 FragPos;
    vec3 NormalPos;
    vec2 TexCoords;
    vec3 TangentLightPos;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
    vec4 FragPosLightSpace;
} vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform mat4 lightSpaceMatrix;

void main()
{
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.NormalPos = transpose(inverse(mat3(model))) * aNormal;
    vs_out.TexCoords = aTexCoords;

    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 N = normalize(normalMatrix * aNormal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    mat3 TBN = transpose(mat3(T, B, N));
    vs_out.TangentLightPos = TBN * lightPos;
    vs_out.TangentViewPos = TBN * viewPos;
    vs_out.TangentFragPos = TBN * vs_out.FragPos;

    vs_out.FragPosLightSpace = lightSpaceMatrix * vec4(vs_out.FragPos, 1.0);
    //gl_Position = projection * view * model * vec4(aPos, 1.0);
    gl_Position = vec4(10, 10, 10, 1.0);
}
)";

const char* shadow_mapping_depthFS_shader = R"(
#version 330 core

void main()
{             
    gl_FragDepth = gl_FragCoord.z;
}
)";

const char* shadow_mapping_depthVS_shader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 lightSpaceMatrix;
uniform mat4 model;

void main()
{
    gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
}
)";

const char* textFS_shader = R"(
#version 330 core

in vec2 TexCoords;
out vec4 color;

uniform sampler2D text;
uniform vec3 textColor;

void main()
{ 
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
    color = vec4(textColor, 1.0) * sampled;                        
} 

)";

const char* textVS_shader = R"(
#version 330 core
layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>
out vec2 TexCoords;

uniform mat4 projection;

void main()
{
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
    TexCoords = vertex.zw;
}
)";

const char* vertexShader_shader = R"(
#version 330 core


layout(location = 0) in vec3 vertexRelativePosition;
layout(location = 1) in vec2 vertexUV;
layout(location = 2) in vec3 vertexNormal;

out vec3 Normal;
out vec3 FragPos;
out vec4 FragPosLightSpace;
out vec2 UV;
out float distToCamera;

uniform mat4 MVP;
uniform vec3 PlayerPosition;
uniform vec3 ObjectPosition;
uniform mat3 ObjectRotation;
uniform vec3 ObjectScale;
uniform bool isTerrain;
uniform mat4 lightSpaceMatrix;

vec3 Rotate(vec3 v) {
	vec3 res = ObjectRotation * v;
	return res;
}

vec3 scale(vec3 v, vec3 s) {
	return vec3(v.x * s.x, v.y * s.y, v.z * s.z);
}

void main() {

	vec3 vertexPosition = Rotate(vertexRelativePosition);
	vertexPosition = scale(vertexPosition, ObjectScale);
	vertexPosition += ObjectPosition;


	gl_Position = MVP * vec4(vertexPosition, 1);
	vec3 vposition = vertexPosition.xyz;
	distToCamera = distance(vposition, PlayerPosition);
	mat4 modelView = mat4(1.0);
	FragPos = vec3(modelView * vec4(vertexPosition, 1.0));
	FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
	UV = vertexUV;
	Normal = Rotate(vertexNormal);
	
}

)";

