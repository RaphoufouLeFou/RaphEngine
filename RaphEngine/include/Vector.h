#pragma once

#ifdef RAPHENGINE_EXPORTS
#define RAPHENGINE_API __declspec(dllexport)
#else
#define RAPHENGINE_API __declspec(dllimport)
#endif

#include <glm.hpp>
#ifdef RAPHENGINE_EXPORTS
#endif

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