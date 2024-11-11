#pragma once

#ifdef RAPHENGINE_EXPORTS
#define RAPHENGINE_API __declspec(dllexport)
#else
#define RAPHENGINE_API __declspec(dllimport)
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
};