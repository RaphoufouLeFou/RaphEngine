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