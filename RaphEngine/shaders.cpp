#include "shaders.h"
#include "pch.h"

const char* a10_debug_cascade_VS_shader = R"(
#version 410 core
layout (location = 0) in vec3 aPos;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * vec4(aPos, 1.0);
}

)";

const char* Map_VS_shader = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

out VS_OUT{
    vec3 FragPos;
    vec2 TexCoords;
    vec3 FragNormal;
    vec3 TangentLightDir;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
    vec4 FragPosLightSpace;
    mat3 TBN;
} vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform vec3 lightPos;
uniform vec3 lightDir;
uniform vec3 viewPos;
uniform mat4 lightSpaceMatrix;

void main()
{
    /*
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
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

    gl_Position = projection * view * model * vec4(aPos, 1.0);
    */

    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.TexCoords = aTexCoords;
    vs_out.FragNormal = aNormal;

    vec3 T = normalize(vec3(model * vec4(aTangent,   0.0)));
    vec3 B = normalize(vec3(model * vec4(aBitangent, 0.0)));
    vec3 N = normalize(vec3(model * vec4(aNormal,    0.0)));
    mat3 TBN = mat3(T, B, N);

    vs_out.TangentLightDir = TBN * lightDir;
    vs_out.TangentViewPos = TBN * viewPos;
    vs_out.TangentFragPos = TBN * vs_out.FragPos;

    vs_out.FragPosLightSpace = lightSpaceMatrix * vec4(vs_out.FragPos, 1.0);

    vs_out.TBN = TBN;

    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* a10_shadow_mapping_VS_shader = R"(
#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.Normal = transpose(inverse(mat3(model))) * aNormal;
    vs_out.TexCoords = aTexCoords;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}

)";

const char* DEBUG_FS_shader = R"(
#version 330 core

#define SAMPLES_COUNT 2
#define INV_SAMPLES_COUNT (1.0f / SAMPLES_COUNT)
#define PIXELS_COUNT ((SAMPLES_COUNT * 2 + 1) * (SAMPLES_COUNT * 2 + 1))
out vec4 FragColor;

in VS_OUT{
    vec3 FragPos;
    vec2 TexCoords;
    vec3 TangentLightPos;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
    vec4 FragPosLightSpace;
} fs_in;

uniform sampler2D texture_diffuse;
uniform sampler2D texture_specular;
uniform sampler2D texture_normal;
uniform sampler2D texture_height;
uniform sampler2D shadowMap;
uniform sampler3D gShadowMapOffsetTexture;

uniform int gShadowMapFilterSize = 0;
uniform float gShadowMapOffsetTextureSize;
uniform float gShadowMapOffsetFilterSize;
uniform float gShadowMapRandomRadius = 0.0;

uniform vec3 lightPos;
uniform vec3 lightDir;
uniform vec3 viewPos;
uniform float heightScale;
uniform bool HaveNormalMap;
uniform bool HaveSpecularMap;
uniform bool HaveHeightMap;
uniform vec2 offsets[PIXELS_COUNT];



vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{
    if (!HaveHeightMap)
        return texCoords;
    // number of depth layers
    const float minLayers = 8;
    const float maxLayers = 32;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    // depth of current layer
    float currentLayerDepth = 0.0;
    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy / viewDir.z * heightScale;
    vec2 deltaTexCoords = P / numLayers;

    // get initial values
    vec2  currentTexCoords = texCoords;
    float currentDepthMapValue = texture(texture_height, currentTexCoords).r;

    while (currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get depthmap value at current texture coordinates
        currentDepthMapValue = texture(texture_height, currentTexCoords).r;
        // get depth of next layer
        currentLayerDepth += layerDepth;
    }

    // get texture coordinates before collision (reverse operations)
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // get depth after and before collision for linear interpolation
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(texture_height, prevTexCoords).r - currentLayerDepth + layerDepth;

    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;
}

float ShadowCalculation(vec4 fragPosLightSpace, vec3 Normal)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    //projCoords.xy += offsets[0];
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // calculate bias (based on depth map resolution and slope)
    vec3 normal = normalize(Normal);
    float bias = max(0.01 * (1.0 - dot(normal, lightDir)), 0.005);
    // check whether current frag pos is in shadow
    // float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    int i = 0;
    for(int x = -SAMPLES_COUNT; x <=SAMPLES_COUNT; x++)
    {
		for(int y = -SAMPLES_COUNT; y <=SAMPLES_COUNT; y++)
		{
			float pcfDepth = texture(shadowMap, projCoords.xy + (vec2(x, y)) * texelSize + offsets[i++] * texelSize * projCoords.xy).r;
			shadow += currentDepth - bias >= pcfDepth ? 1.0 : 0.0;
		}
	}
    shadow /= PIXELS_COUNT;

    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if (projCoords.z > 1.0)
        shadow = 0.0;
    if (shadow < 0.5)
        shadow = 0.0;

    return shadow;
}

vec3 CalcShadowCoords()
{
    vec3 ProjCoords = fs_in.FragPosLightSpace.xyz / fs_in.FragPosLightSpace.w;
    vec3 ShadowCoords = ProjCoords * 0.5 + vec3(0.5);
    return ShadowCoords;
}

float CalcShadowFactorWithRandomSampling(vec3 LightDirection, vec3 Normal)
{
    ivec3 OffsetCoord;
    vec2 f = mod(gl_FragCoord.xy, vec2(gShadowMapOffsetTextureSize));
    OffsetCoord.yz = ivec2(f);
    float Sum = 0.0;
    int SamplesDiv2 = int(gShadowMapOffsetFilterSize * gShadowMapOffsetFilterSize / 2.0);
    vec3 ShadowCoords = CalcShadowCoords();
    vec4 sc = vec4(ShadowCoords, 1.0);

    vec2 TexSize = textureSize(shadowMap, 0);
    float TexelWidth = 1.0 / TexSize.x;
    float TexelHeight = 1.0 / TexSize.y;

    vec2 TexelSize = vec2(TexelWidth, TexelHeight);

    float DiffuseFactor = dot(Normal, -LightDirection);
    float bias = mix(0.001, 0.0, DiffuseFactor);
    float Depth = 0.0;

    for (int i = 0; i < 4; i++) {
        OffsetCoord.x = i;
        vec4 Offsets = texelFetch(gShadowMapOffsetTexture, OffsetCoord, 0) * gShadowMapRandomRadius;
        sc.xy = ShadowCoords.xy + Offsets.rg * TexelSize;
        Depth = texture(shadowMap, sc.xy).x;
        if (Depth + bias < ShadowCoords.z) {
            Sum += 0.0;
        }
        else {
            Sum += 1.0;
        }

        sc.xy = ShadowCoords.xy + Offsets.ba * TexelSize;
        Depth = texture(shadowMap, sc.xy).x;
        if (Depth + bias < ShadowCoords.z) {
            Sum += 0.0;
        }
        else {
            Sum += 1.0;
        }
    }

    float Shadow = Sum / 8.0;

    if (Shadow != 0.0 && Shadow != 1.0) {
        for (int i = 4; i < SamplesDiv2; i++) {
            OffsetCoord.x = i;
            vec4 Offsets = texelFetch(gShadowMapOffsetTexture, OffsetCoord, 0) * gShadowMapRandomRadius;
            sc.xy = ShadowCoords.xy + Offsets.rg * TexelSize;
            Depth = texture(shadowMap, sc.xy).x;
            if (Depth + bias < ShadowCoords.z) {
                Sum += 0.0;
            }
            else {
                Sum += 1.0;
            }

            sc.xy = ShadowCoords.xy + Offsets.ba * TexelSize;
            Depth = texture(shadowMap, sc.xy).x;
            if (Depth + bias < ShadowCoords.z) {
                Sum += 0.0;
            }
            else {
                Sum += 1.0;
            }
        }

        Shadow = Sum / float(SamplesDiv2 * 2.0);
    }

    return Shadow;
}


void main()
{
    /*
    // offset texture coordinates with Parallax Mapping
   
    vec2 texCoords = fs_in.TexCoords;

    texCoords = ParallaxMapping(fs_in.TexCoords, viewDir);
    if (texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0)
        discard;
    */
    vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
    vec3 normal = vec3(0, 0, 1.0);
    if (HaveNormalMap)
	{
        normal = texture(texture_normal, fs_in.TexCoords).rgb;
        normal = normalize(normal * 2.0 - 1.0);  // this normal is in tangent space
	}
    
    vec3 color = texture(texture_diffuse, fs_in.TexCoords).rgb;
    vec3 lightColor = vec3(1.0);

    // ambient
    vec3 ambient = 0.1 * lightColor;
    // diffuse
    vec3 lightDir2 = normalize(fs_in.TangentLightPos - fs_in.TangentFragPos);
    float diff = max(dot(lightDir2, normal), 0.0);
    vec3 diffuse = diff * lightColor;
    // specular
    vec3 reflectDir = reflect(-lightDir2, normal);
    vec3 halfwayDir = normalize(lightDir2 + viewDir);
    float specFact = 0.2; 
    if(HaveSpecularMap)
        specFact = texture(texture_specular, fs_in.TexCoords).r;
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0) * specFact;
    vec3 specular = vec3(spec);

    // calculate shadow
    float shadow = ShadowCalculation(fs_in.FragPosLightSpace, normal);
    //float shadow = CalcShadowFactorWithRandomSampling(lightDir2, normal);
    vec3 lighting = (ambient + vec3(1 - shadow) * (diffuse + specular)) * color;


    FragColor = vec4(lighting, 1.0);
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

const char* textVS_shader = R"(
#version 330 core
layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>
out vec2 TexCoords;

uniform mat4 projection;

void main()
{
    gl_Position = projection * vec4(vertex.xy, 1.0, 1.0);
    TexCoords = vertex.zw;
}
)";

const char* a10_debug_cascade_FS_shader = R"(
#version 410 core
out vec4 FragColor;

uniform vec4 color;

void main()
{             
    FragColor = color;
}

)";

const char* Map_FS_shader = R"(
#version 330 core

#define SAMPLES_COUNT 1
#define INV_SAMPLES_COUNT (1.0f / SAMPLES_COUNT)
#define PIXELS_COUNT ((SAMPLES_COUNT * 2 + 1) * (SAMPLES_COUNT * 2 + 1))
out vec4 FragColor;

in VS_OUT{
    vec3 FragPos;
    vec2 TexCoords;
    vec3 FragNormal;
    vec3 TangentLightDir;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
    vec4 FragPosLightSpace;
    mat3 TBN;
} fs_in;

uniform sampler2D texture_diffuse;
uniform sampler2D texture_specular;
uniform sampler2D texture_normal;
uniform sampler2D texture_height;
uniform sampler2DArray shadowMap;
uniform sampler3D gShadowMapOffsetTexture;

uniform vec3 CircleCenter;

uniform int gShadowMapFilterSize = 0;
uniform float gShadowMapOffsetTextureSize;
uniform float gShadowMapOffsetFilterSize;
uniform float gShadowMapRandomRadius = 0.0;

uniform vec3 lightPos;
uniform vec3 lightDir;
uniform vec3 viewPos;
uniform float heightScale;
uniform float farPlane;
uniform bool HaveNormalMap;
uniform bool HaveSpecularMap;
uniform bool HaveHeightMap;
uniform vec2 offsets[PIXELS_COUNT];
uniform mat4 view;

layout (std140) uniform LightSpaceMatrices
{
    mat4 lightSpaceMatrices[8];
};
uniform float cascadePlaneDistances[8];
uniform int cascadeCount;   // number of frusta - 1

vec3 CircleColor()
{
    vec3 color = vec3(0.0);
    float dist = distance(fs_in.FragPos, CircleCenter);
    if (dist < 8)
        color = vec3(1.0, 0.0, 0.0);
    else if (dist < 10)
        color = vec3(0.0, 1.0, 0.0);
    return color;
}

int GetTextureIndex(vec3 normal)
{
    vec3 upVec = vec3(0, 0, 1);
    float dotProd = dot(normal, upVec);
    if(dotProd > -0.9)
        return 0;
    else return 1;
}



float ShadowCalculation(vec3 fragPosWorldSpace, vec3 normal)
{
    // select cascade layer
    vec4 fragPosViewSpace = view * vec4(fragPosWorldSpace, 1.0);
    float depthValue = abs(fragPosViewSpace.z);

    int layer = -1;
    for (int i = 0; i < cascadeCount; ++i)
    {
        if (depthValue < farPlane / cascadePlaneDistances[i])
        {
            layer = i;
            break;
        }
    }
    //layer = 4;
    if (layer == -1)
    {
        layer = cascadeCount;
    }

    vec4 fragPosLightSpace = lightSpaceMatrices[layer] * vec4(fragPosWorldSpace, 1.0);
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if (currentDepth > 1.0)
    {
        return 0.0;
    }
    // calculate bias (based on depth map resolution and slope)
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);  
    if (layer < 2)
    {
        bias *= 0.012;
    }
    else
    {
        bias *= 0.005; //1 / ((farPlane / cascadePlaneDistances[layer]) * biasModifier);
    }

    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    for(int x = -1; x <= 1; x++)
    {
        for(int y = -1; y <= 1; y++)
        {
            float pcfDepth = texture(shadowMap, vec3(projCoords.xy + vec2(x, y) * texelSize, layer)).r;
            shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
        
    return shadow;
}


/*
vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{
    if (!HaveHeightMap)
        return texCoords;
    // number of depth layers
    const float minLayers = 8;
    const float maxLayers = 32;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    // depth of current layer
    float currentLayerDepth = 0.0;
    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy / viewDir.z * heightScale;
    vec2 deltaTexCoords = P / numLayers;

    // get initial values
    vec2  currentTexCoords = texCoords;
    float currentDepthMapValue = texture(texture_height, currentTexCoords).r;

    while (currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get depthmap value at current texture coordinates
        currentDepthMapValue = texture(texture_height, currentTexCoords).r;
        // get depth of next layer
        currentLayerDepth += layerDepth;
    }

    // get texture coordinates before collision (reverse operations)
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // get depth after and before collision for linear interpolation
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(texture_height, prevTexCoords).r - currentLayerDepth + layerDepth;

    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;
}*/


/*
float ShadowCalculation(vec4 fragPosLightSpace, vec3 Normal)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    //projCoords.xy += offsets[0];
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // calculate bias (based on depth map resolution and slope)
    vec3 normal = normalize(Normal);
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);  
    // float bias = max(0.01 * (1.0 - dot(normal, lightDir)), 0.005);
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
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
*/
/* 
    int i = 0;
    for(int x = -SAMPLES_COUNT; x <=SAMPLES_COUNT; x++)
    {
		for(int y = -SAMPLES_COUNT; y <=SAMPLES_COUNT; y++)
		{
			float pcfDepth = texture(shadowMap, projCoords.xy + (vec2(x, y)) * texelSize + offsets[i++] * texelSize * projCoords.xy).r;
			shadow += currentDepth - bias >= pcfDepth ? 1.0 : 0.0;
		}
	}
    shadow /= PIXELS_COUNT;

    float pcfDepth = texture(shadowMap, projCoords.xy * texelSize).r;
	shadow = currentDepth - bias >= pcfDepth ? 1.0 : 0.0;
*//*
    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if (projCoords.z > 1.0)
        shadow = 0.0;
    if (shadow < 0.5)
        shadow = 0.0;

    return shadow;
}
*/

/*
vec3 CalcShadowCoords()
{
    vec3 ProjCoords = fs_in.FragPosLightSpace.xyz / fs_in.FragPosLightSpace.w;
    vec3 ShadowCoords = ProjCoords * 0.5 + vec3(0.5);
    return ShadowCoords;
}

float CalcShadowFactorWithRandomSampling(vec3 LightDirection, vec3 Normal)
{
    ivec3 OffsetCoord;
    vec2 f = mod(gl_FragCoord.xy, vec2(gShadowMapOffsetTextureSize));
    OffsetCoord.yz = ivec2(f);
    float Sum = 0.0;
    int SamplesDiv2 = int(gShadowMapOffsetFilterSize * gShadowMapOffsetFilterSize / 2.0);
    vec3 ShadowCoords = CalcShadowCoords();
    vec4 sc = vec4(ShadowCoords, 1.0);

    vec2 TexSize = textureSize(shadowMap, 0);
    float TexelWidth = 1.0 / TexSize.x;
    float TexelHeight = 1.0 / TexSize.y;

    vec2 TexelSize = vec2(TexelWidth, TexelHeight);

    float DiffuseFactor = dot(Normal, -LightDirection);
    float bias = mix(0.001, 0.0, DiffuseFactor);
    float Depth = 0.0;

    for (int i = 0; i < 4; i++) {
        OffsetCoord.x = i;
        vec4 Offsets = texelFetch(gShadowMapOffsetTexture, OffsetCoord, 0) * gShadowMapRandomRadius;
        sc.xy = ShadowCoords.xy + Offsets.rg * TexelSize;
        Depth = texture(shadowMap, sc.xy).x;
        if (Depth + bias < ShadowCoords.z) {
            Sum += 0.0;
        }
        else {
            Sum += 1.0;
        }

        sc.xy = ShadowCoords.xy + Offsets.ba * TexelSize;
        Depth = texture(shadowMap, sc.xy).x;
        if (Depth + bias < ShadowCoords.z) {
            Sum += 0.0;
        }
        else {
            Sum += 1.0;
        }
    }

    float Shadow = Sum / 8.0;

    if (Shadow != 0.0 && Shadow != 1.0) {
        for (int i = 4; i < SamplesDiv2; i++) {
            OffsetCoord.x = i;
            vec4 Offsets = texelFetch(gShadowMapOffsetTexture, OffsetCoord, 0) * gShadowMapRandomRadius;
            sc.xy = ShadowCoords.xy + Offsets.rg * TexelSize;
            Depth = texture(shadowMap, sc.xy).x;
            if (Depth + bias < ShadowCoords.z) {
                Sum += 0.0;
            }
            else {
                Sum += 1.0;
            }

            sc.xy = ShadowCoords.xy + Offsets.ba * TexelSize;
            Depth = texture(shadowMap, sc.xy).x;
            if (Depth + bias < ShadowCoords.z) {
                Sum += 0.0;
            }
            else {
                Sum += 1.0;
            }
        }

        Shadow = Sum / float(SamplesDiv2 * 2.0);
    }

    return Shadow;
}
*/


void main()
{
    /*
    // offset texture coordinates with Parallax Mapping
   
    vec2 texCoords = fs_in.TexCoords;

    texCoords = ParallaxMapping(fs_in.TexCoords, viewDir);
    if (texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0)
        discard;
    */
    vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
    vec3 normal = fs_in.FragNormal;
    
    if (HaveNormalMap)
	{
        normal = texture(texture_normal, fs_in.TexCoords).rgb;
        normal = normalize(fs_in.TBN * (normal * 2.0 - 1.0)); 
	}
    
    vec3 color = texture(texture_diffuse, fs_in.TexCoords).rgb;
    vec3 lightColor = vec3(1.0);

    // ambient
    vec3 ambient = 0.5 * lightColor;
    // diffuse
    vec3 lightDir2 = fs_in.TangentLightDir; //normalize(fs_in.TangentLightPos - fs_in.TangentFragPos); // 
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;
    // specular
    vec3 reflectDir = reflect(-lightDir2, normal);
    vec3 halfwayDir = normalize(lightDir2 + viewDir);
    float specFact = 1; 
    if(HaveSpecularMap)
        specFact = texture(texture_specular, fs_in.TexCoords).r;
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0) * specFact;
    vec3 specular = vec3(spec);

    // calculate shadow
    float shadow = ShadowCalculation(fs_in.FragPos, normal);
    //float shadow = CalcShadowFactorWithRandomSampling(lightDir2, normal);
    vec3 lighting = (ambient + vec3(1 - shadow) * (diffuse + specular)) * color;

    //vec3 CircleCol = CircleColor();
    //if (CircleCol.r > 0.0)
    //   lighting = CircleCol;

    // int index = GetTextureIndex(normal);

    FragColor = vec4(lighting, 1.0);
}
)";

const char* a10_shadow_mapping_depth_GS_shader = R"(
#version 410 core

layout(triangles, invocations = 5) in;
layout(triangle_strip, max_vertices = 3) out;

layout (std140) uniform LightSpaceMatrices
{
    mat4 lightSpaceMatrices[16];
};
/*
uniform mat4 lightSpaceMatrices[16];
*/

void main()
{          
	for (int i = 0; i < 3; ++i)
	{
		gl_Position = lightSpaceMatrices[gl_InvocationID] * gl_in[i].gl_Position;
		gl_Layer = gl_InvocationID;
		EmitVertex();
	}
	EndPrimitive();
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

const char* a10_debug_quad_depth_FS_shader = R"(
#version 410 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2DArray depthMap;
uniform float near_plane;
uniform float far_plane;
uniform int layer;

// required when using a perspective projection matrix
float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // Back to NDC 
    return (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));	
}

void main()
{             
    float depthValue = texture(depthMap, vec3(TexCoords, layer)).r;
    // FragColor = vec4(vec3(LinearizeDepth(depthValue) / far_plane), 1.0); // perspective
    FragColor = vec4(vec3(depthValue), 1.0); // orthographic
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

const char* DEBUG_VS_shader = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

out VS_OUT{
    vec3 FragPos;
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
    /*
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
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

    gl_Position = projection * view * model * vec4(aPos, 1.0);
    */

    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.TexCoords = aTexCoords;

    vec3 T = normalize(mat3(model) * aTangent);
    vec3 B = normalize(mat3(model) * aBitangent);
    vec3 N = normalize(mat3(model) * aNormal);
    mat3 TBN = transpose(mat3(T, B, N));

    vs_out.TangentLightPos = TBN * lightPos;
    vs_out.TangentViewPos = TBN * viewPos;
    vs_out.TangentFragPos = TBN * vs_out.FragPos;

    vs_out.FragPosLightSpace = lightSpaceMatrix * vec4(vs_out.FragPos, 1.0);

    gl_Position = projection * view * model * vec4(aPos, 1.0);
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

const char* a10_shadow_mapping_depth_VS_shader = R"(
#version 410 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;

void main()
{
    gl_Position = model * vec4(aPos, 1.0);
}

)";

const char* a10_shadow_mapping_FS_shader = R"(
#version 410 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

uniform sampler2D texture_diffuse;
uniform sampler2DArray shadowMap;

uniform vec3 lightDir;
uniform vec3 viewPos;
uniform float farPlane;

uniform mat4 view;

layout (std140) uniform LightSpaceMatrices
{
    mat4 lightSpaceMatrices[16];
};
uniform float cascadePlaneDistances[16];
uniform int cascadeCount;   // number of frusta - 1

float ShadowCalculation(vec3 fragPosWorldSpace)
{
    // select cascade layer
    vec4 fragPosViewSpace = view * vec4(fragPosWorldSpace, 1.0);
    float depthValue = abs(fragPosViewSpace.z);

    int layer = -1;
    for (int i = 0; i < cascadeCount; ++i)
    {
        if (depthValue < cascadePlaneDistances[i])
        {
            layer = i;
            break;
        }
    }
    if (layer == -1)
    {
        layer = cascadeCount;
    }

    vec4 fragPosLightSpace = lightSpaceMatrices[layer] * vec4(fragPosWorldSpace, 1.0);
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if (currentDepth > 1.0)
    {
        return 0.0;
    }
    // calculate bias (based on depth map resolution and slope)
    vec3 normal = normalize(fs_in.Normal);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    const float biasModifier = 0.5f;
    if (layer == cascadeCount)
    {
        bias *= 1 / (farPlane * biasModifier);
    }
    else
    {
        bias *= 1 / (cascadePlaneDistances[layer] * biasModifier);
    }

    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, vec3(projCoords.xy + vec2(x, y) * texelSize, layer)).r;
            shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
        
    return shadow;
}

void main()
{           
    vec3 color = texture(texture_diffuse, fs_in.TexCoords).rgb;
    vec3 normal = normalize(fs_in.Normal);
    vec3 lightColor = vec3(0.3);
    // ambient
    vec3 ambient = 0.3 * color;
    // diffuse
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;
    // specular
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0;
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = spec * lightColor;    
    // calculate shadow
    float shadow = ShadowCalculation(fs_in.FragPos);                      
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color;    
    
    FragColor = vec4(lighting, 1.0);
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

const char* shadow_mappingVS_shader = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

out VS_OUT{
    vec3 FragPos;
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
    /*
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
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

    gl_Position = projection * view * model * vec4(aPos, 1.0);
    */

    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.TexCoords = aTexCoords;

    vec3 T = normalize(mat3(model) * aTangent);
    vec3 B = normalize(mat3(model) * aBitangent);
    vec3 N = normalize(mat3(model) * aNormal);
    mat3 TBN = transpose(mat3(T, B, N));

    vs_out.TangentLightPos = TBN * lightPos;
    vs_out.TangentViewPos = TBN * viewPos;
    vs_out.TangentFragPos = TBN * vs_out.FragPos;

    vs_out.FragPosLightSpace = lightSpaceMatrix * vec4(vs_out.FragPos, 1.0);

    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* a10_shadow_mapping_depth_FS_shader = R"(
#version 410 core

void main()
{             
}

)";

const char* shadow_mappingFS_shader = R"(
#version 330 core

#define SAMPLES_COUNT 2
#define INV_SAMPLES_COUNT (1.0f / SAMPLES_COUNT)
#define PIXELS_COUNT ((SAMPLES_COUNT * 2 + 1) * (SAMPLES_COUNT * 2 + 1))
out vec4 FragColor;

in VS_OUT{
    vec3 FragPos;
    vec2 TexCoords;
    vec3 TangentLightPos;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
    vec4 FragPosLightSpace;
} fs_in;

uniform sampler2D texture_diffuse;
uniform sampler2D texture_specular;
uniform sampler2D texture_normal;
uniform sampler2D texture_height;
uniform sampler2D shadowMap;


uniform vec3 lightPos;
uniform vec3 lightDir;
uniform vec3 viewPos;
uniform float heightScale;
uniform bool HaveNormalMap;
uniform bool HaveSpecularMap;
uniform bool HaveHeightMap;
uniform vec2 offsets[PIXELS_COUNT];



vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{
    if (!HaveHeightMap)
        return texCoords;
    // number of depth layers
    const float minLayers = 8;
    const float maxLayers = 32;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    // depth of current layer
    float currentLayerDepth = 0.0;
    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy / viewDir.z * heightScale;
    vec2 deltaTexCoords = P / numLayers;

    // get initial values
    vec2  currentTexCoords = texCoords;
    float currentDepthMapValue = texture(texture_height, currentTexCoords).r;

    while (currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get depthmap value at current texture coordinates
        currentDepthMapValue = texture(texture_height, currentTexCoords).r;
        // get depth of next layer
        currentLayerDepth += layerDepth;
    }

    // get texture coordinates before collision (reverse operations)
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // get depth after and before collision for linear interpolation
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(texture_height, prevTexCoords).r - currentLayerDepth + layerDepth;

    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;
}

float ShadowCalculation(vec4 fragPosLightSpace, vec3 Normal)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    //projCoords.xy += offsets[0];
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // calculate bias (based on depth map resolution and slope)
    vec3 normal = normalize(Normal);
    float bias = max(0.01 * (1.0 - dot(normal, lightDir)), 0.005);
    // check whether current frag pos is in shadow
    // float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    int i = 0;
    for (int x = -SAMPLES_COUNT; x <= SAMPLES_COUNT; x++)
    {
        for (int y = -SAMPLES_COUNT; y <= SAMPLES_COUNT; y++)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + (vec2(x, y)) * texelSize + offsets[i++] * texelSize * projCoords.xy).r;
            shadow += currentDepth - bias >= pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= PIXELS_COUNT;

    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if (projCoords.z > 1.0)
        shadow = 0.0;
    if (shadow < 0.5)
        shadow = 0.0;

    return shadow;
}

void main()
{
    /*
    // offset texture coordinates with Parallax Mapping

    vec2 texCoords = fs_in.TexCoords;

    texCoords = ParallaxMapping(fs_in.TexCoords, viewDir);
    if (texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0)
        discard;
    */
    vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
    vec3 normal = vec3(0, 0, 1.0);
    if (HaveNormalMap)
    {
        normal = texture(texture_normal, fs_in.TexCoords).rgb;
        normal = normalize(normal * 2.0 - 1.0);  // this normal is in tangent space
    }

    vec3 color = texture(texture_diffuse, fs_in.TexCoords).rgb;
    vec3 lightColor = vec3(1.0);

    // ambient
    vec3 ambient = 0.1 * lightColor;
    // diffuse
    vec3 lightDir2 = normalize(fs_in.TangentLightPos - fs_in.TangentFragPos);
    float diff = max(dot(lightDir2, normal), 0.0);
    vec3 diffuse = diff * lightColor;
    // specular
    vec3 reflectDir = reflect(-lightDir2, normal);
    vec3 halfwayDir = normalize(lightDir2 + viewDir);
    float specFact = 0.2;
    if (HaveSpecularMap)
        specFact = texture(texture_specular, fs_in.TexCoords).r;
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0) * specFact;
    vec3 specular = vec3(spec);

    // calculate shadow
    float shadow = ShadowCalculation(fs_in.FragPosLightSpace, normal);
    vec3 lighting = (ambient + vec3(1 - shadow) * (diffuse + specular)) * color;


    FragColor = vec4(lighting, 1.0);
}
)";

const char* shadow_mapping_depthFS_shader = R"(
#version 330 core

void main()
{             
    gl_FragDepth = gl_FragCoord.z;
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

const char* a10_debug_quad_VS_shader = R"(
#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos, 1.0);
}

)";

