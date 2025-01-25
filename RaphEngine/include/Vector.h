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

class RAPHENGINE_API Vector2 {
public:
	float x, y;
	Vector2(float x = 0, float y = 0) :
		x(x), y(y) {}
	Vector2 operator+(const Vector2& other);
	Vector2 operator-(const Vector2& other);
	Vector2 operator*(const Vector2& other);
	Vector2 operator/(const Vector2& other);
	Vector2 operator+=(const Vector2& other);

	Vector2 operator+(const float& other);
	Vector2 operator-(const float& other);
	Vector2 operator*(const float& other);
	Vector2 operator/(const float& other);
	Vector2 operator+=(const float& other);

#ifdef RAPHENGINE_EXPORTS
	Vector2 operator=(const glm::vec2& other);
	glm::vec2 operator=(const Vector2& other);
#endif
};