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
uniform vec3 fogColor;
uniform int RenderDistance;
uniform vec3 lightPos;
uniform bool isTerrain;

void main() {
	if (isTerrain) {
		vec3 lightColor = vec3(1, 1, 1);
		vec3 norm = normalize(Normal);
		vec3 lightDir = normalize(lightPos - FragPos);
		float diff = max(dot(norm, lightDir), 0.0);
		vec3 diffuse = diff * lightColor;

		float ambientStrength = 0.1;
		vec3 ambient = ambientStrength * lightColor;

		float smoothness = 5 * RenderDistance;
		float value = (-distToCamera / smoothness) + (16 * RenderDistance) / smoothness;

		vec3 objectColor = texture(myTextureSampler, UV).rgb;

		vec3 result = (ambient + diffuse) * objectColor;
		FragColor = result * clamp(value, 0.001, 1)
			+ fogColor * (1 - clamp(value, 0.001, 1));
	}

	else {
		vec3 lightColor = vec3(1, 1, 1);
		vec3 norm = normalize(Normal);
		vec3 lightDir = normalize(lightPos - FragPos);
		float diff = max(dot(norm, lightDir), 0.0);
		vec3 diffuse = diff * lightColor;

		float ambientStrength = 0.1;
		vec3 ambient = ambientStrength * lightColor;

		vec3 objectColor = texture(myTextureSampler, UV).rgb;

		FragColor = (ambient + diffuse) * objectColor;
	}
}