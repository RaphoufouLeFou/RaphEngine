#version 330 core

#define SAMPLES_COUNT 0
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

uniform vec3 PointA;
uniform vec3 PointB;
uniform float LineThikness;
uniform int LineState;

uniform int gShadowMapFilterSize = 0;
uniform float gShadowMapOffsetTextureSize;
uniform float gShadowMapOffsetFilterSize;
uniform float gShadowMapRandomRadius = 0.0;

uniform vec3 lightPos;
uniform vec3 lightDir;
uniform vec3 viewPos;
uniform float heightScale;
uniform float farPlane;
uniform bool HaveTexture;
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
/*
vec4 CircleColor()
{
    vec4 color = vec4(0.0);
    float dist = distance(fs_in.FragPos, CircleCenter);
    if (dist < 7)
        color = vec4(0.0, 1.0, 0.0, 1.0);
    else if (dist < 8)
    {
        dist -= 7;
        color = vec4(0.0, 1, dist, 1.0);
    }
    else if (dist < 10)
    {
        dist -= 8;
        dist /= 2;
        color = vec4(0.0, 1 - dist, 1 - dist, 1);
    }
        
    return color;
}
*/
/*
vec4 CircleColor()
{
    vec4 color = vec4(0.0);
    float dist = distance(fs_in.FragPos, CircleCenter);

    if (dist < 10)
    {
        color = vec4(0.3, 0.56, 1, .8);
    }
    return color;
}
*/

vec3 GetLineEquation(vec2 pointA, vec2 pointB)
{
    vec3 line;
    line.x = pointA.y - pointB.y;
    line.y = pointB.x - pointA.x;
    line.z = pointA.x * pointB.y - pointB.x * pointA.y;
    return line;
}

float LineDistance(vec2 pct1, vec2 pct2, vec2 pct3)
{
    vec2 ab = pct3 - pct2;
    vec2 ap = pct1 - pct2;

    float abLenSquared = dot(ab, ab);
    float t = dot(ap, ab) / abLenSquared;

    if (t < 0.0) {
        // Closest to pct2
        return length(pct1 - pct2);
    } else if (t > 1.0) {
        // Closest to pct3
        return length(pct1 - pct3);
    } else {
        // Projection lies on the segment
        vec2 projection = pct2 + t * ab;
        return length(pct1 - projection);
    }
}

vec4 CircleColor()
{
    if(LineState == 0)
        return vec4(0.0);
    
    vec4 CoreColor;
    if(LineState == 1)
        CoreColor = vec4(0.3, 0.56, 1, .8);
    else
        CoreColor = vec4(1, 0, 0, .8);

    vec2 center = (PointA.xy + PointB.xy) / 2;
    float dist = distance(PointA.xy, PointB.xy);


    if(distance(fs_in.FragPos.xy, center) > dist / 2 + LineThikness + 1)
        return vec4(0.0);
    

    float d = LineDistance(fs_in.FragPos.xy, PointA.xy, PointB.xy);
    float Adist = distance(fs_in.FragPos.xy, PointA.xy);
    float Bdist = distance(fs_in.FragPos.xy, PointB.xy);
    if(Adist < LineThikness || Bdist < LineThikness)
        return vec4(0.29, 0.26, .79, .8);
        
    if(Adist < LineThikness + 0.3 || Bdist < LineThikness + 0.3)
        return vec4(1, 1, 1, .8);

    if (d < LineThikness)
        return CoreColor;

    //if (d < LineThikness + 0.3)
    //    return vec4(1, 1, 1, .8);
    return vec4(0.0);
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
    float dotLightNormal = dot(normal, lightDir);
    if(dotLightNormal <= 0.1) return 0.0;

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
    
    float bias = max(0.05 * (1.0 - dotLightNormal), 0.005);  
    if (layer < 2)
    {
        bias *= 0.015;
    }
    else
    {
        bias *= 0.008; //1 / ((farPlane / cascadePlaneDistances[layer]) * biasModifier);
    }

    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    float difference = 1.0;
    float lightSize = farPlane / cascadePlaneDistances[layer];
    int i = 0;
    for(int x = -SAMPLES_COUNT; x <= SAMPLES_COUNT; x++)
    {
        for(int y = -SAMPLES_COUNT; y <= SAMPLES_COUNT; y++)
        {
            
            float pcfDepth = texture(shadowMap, vec3(projCoords.xy + vec2(x, y) * offsets[(i++ + int(projCoords.x))%PIXELS_COUNT] * texelSize, layer)).r;
            shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;    
            if(x == 0 && y == 0)
                difference = (currentDepth - bias) - pcfDepth;
        }    
    }
    shadow /= PIXELS_COUNT;

    // shadow /= max(difference * lightSize, 1);
        
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
    vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.FragPos);
    vec3 normal = fs_in.FragNormal;
    
    if (HaveNormalMap)
	{
        normal = texture(texture_normal, fs_in.TexCoords).rgb;
        normal = normalize(fs_in.TBN * (normal * 2.0 - 1.0)); 
	}
    
    vec3 color = vec3(191, 64, 191) / 255.0; // default color
    if (HaveTexture)
        color = texture(texture_diffuse, fs_in.TexCoords).rgb;
    
    vec3 lightColor = vec3(1.0);

    // ambient
    vec3 ambient = .5 * lightColor;
    // diffuse
    vec3 lightDir2 = fs_in.TangentLightDir; //normalize(fs_in.TangentLightPos - fs_in.TangentFragPos); // 
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;
    // specular
    vec3 reflectDir = reflect(-lightDir2, normal);
    vec3 halfwayDir = normalize(lightDir2 + viewDir);
    float specFact = .2; 
    if(HaveSpecularMap)
        specFact = texture(texture_specular, fs_in.TexCoords).r;
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0) * specFact;
    //vec3 specular = vec3(spec) * vec3(0, 1, 0);
    vec3 specular = vec3(spec);

    // calculate shadow
    float shadow = ShadowCalculation(fs_in.FragPos, normal);
    //float shadow = CalcShadowFactorWithRandomSampling(lightDir2, normal);
    vec3 lighting = (ambient + vec3(1 - shadow) * (diffuse + specular)) * color;

    vec4 CircleCol = CircleColor();
    if(CircleCol.a > 0.01)
        lighting = mix(lighting, CircleCol.rgb, CircleCol.a);

    // int index = GetTextureIndex(normal);

    normal = abs(normal);

    FragColor = vec4(lighting, 1.0);
}