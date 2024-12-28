#pragma once

#ifdef RAPHENGINE_EXPORTS
#define RAPHENGINE_API __declspec(dllexport)
#else
#define RAPHENGINE_API __declspec(dllimport)
#endif

#ifdef RAPHENGINE_EXPORTS
#include <glm.hpp>
#endif

class RAPHENGINE_API Vector3 {
public:
	float x, y, z;
	Vector3(float x = 0, float y = 0, float z = 0) :
		x(x), y(y), z(z) {}
	Vector3 operator+(const Vector3& other);
	Vector3 operator-(const Vector3& other);
	Vector3 operator*(const Vector3& other);
	Vector3 operator/(const Vector3& other);
	Vector3 operator+=(const Vector3& other);

	Vector3 operator+(const float& other);
	Vector3 operator-(const float& other);
	Vector3 operator*(const float& other);
	Vector3 operator/(const float& other);
	Vector3 operator+=(const float& other);
#ifdef RAPHENGINE_EXPORTS
	Vector3 operator=(const glm::vec3& other);
	glm::vec3 operator=(const Vector3& other);
#endif
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