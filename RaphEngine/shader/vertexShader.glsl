#version 330 core


layout(location = 0) in vec3 vertexRelativePosition;
layout(location = 1) in vec2 vertexUV;
layout(location = 2) in vec3 vertexNormal;

out vec3 Normal;
out vec3 FragPos;

out vec2 UV;
out float distToCamera;

uniform mat4 MVP;
uniform vec3 PlayerPosition;
uniform vec3 lightPos;
uniform vec3 ObjectPosition;
uniform vec3 ObjectRotation;
uniform vec3 ObjectScale;
uniform bool isTerrain;

float DEG2RAD(float deg) {
	return deg * 3.14159265358979323846 / 180.0;
}

vec3 Rotate(vec3 v, vec3 r) {
	vec3 res = vec3(

		v.x * cos(DEG2RAD(r.y)) - v.z * sin(DEG2RAD(r.y)),
		v.y,
		v.x * sin(DEG2RAD(r.y)) + v.z * cos(DEG2RAD(r.y))
	);
	return res;
}

vec3 scale(vec3 v, vec3 s) {
	return vec3(v.x * s.x, v.y * s.y, v.z * s.z);
}

void main() {

	vec3 vertexPosition = Rotate(vertexRelativePosition, ObjectRotation);
	vertexPosition = scale(vertexPosition, ObjectScale);
	if (!isTerrain) {
		vertexPosition += ObjectPosition;
	}

	gl_Position = MVP * vec4(vertexPosition, 1);
	vec3 vposition = vertexPosition.xyz;
	distToCamera = distance(vposition, PlayerPosition);
	mat4 modelView = mat4(1.0);
	FragPos = vec3(modelView * vec4(vertexPosition, 1.0));
	UV = vertexUV;
	if (!isTerrain) {
		Normal = Rotate(vertexNormal, ObjectRotation);
	}
	else {
		Normal = vertexNormal;
	}
}
