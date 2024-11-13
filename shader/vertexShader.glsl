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
