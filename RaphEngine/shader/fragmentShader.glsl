#version 330 core

// Interpolated values from the vertex shaders
in vec2 UV;
in vec3 FragPos;
in vec3 Normal;
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