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