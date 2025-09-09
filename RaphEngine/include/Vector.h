#pragma once
#include "LibManager.h"
#ifdef _WIN32
#include <glm.hpp>
#else
#include <glm/glm.hpp>
#endif
#ifdef RAPHENGINE_EXPORTS
#endif


typedef glm::vec4 Vector4;
typedef glm::vec3 Vector3;
typedef glm::vec2 Vector2;

typedef glm::mat2 Matrix2;
typedef glm::mat3 Matrix3;
typedef glm::mat4 Matrix4;


Matrix4 RAPHENGINE_API GetRotationMatrix(Vector3 rotation);



/*
class RAPHENGINE_API Vector3 : public glm::vec3{
public :
	Vector3(float X = 0, float Y = 0, float Z = 0) :
		glm::vec3(X, Y, Z) {}
};

class RAPHENGINE_API Vector2 : public glm::vec2 {
public:	
	Vector2(float X = 0, float Y = 0) :
		glm::vec2(X, Y) {}
};
*/
