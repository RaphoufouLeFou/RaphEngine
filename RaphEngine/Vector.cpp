#include "pch.h"

#include "include/RaphEngine.h"


Vector2 Vector2::operator+(const Vector2& other) {
	return Vector2(x + other.x, y + other.y);
}

Vector2 Vector2::operator*(const Vector2& other) {
	return Vector2(x * other.x, y * other.y);
}

Vector2 Vector2::operator-(const Vector2& other) {
	return Vector2(x - other.x, y - other.y);
}

Vector2 Vector2::operator/(const Vector2& other) {
	return Vector2(x / other.x, y / other.y);
}

Vector2 Vector2::operator+=(const Vector2& other) {
	return Vector2(x += other.x, y += other.y);
}

Vector2 Vector2::operator+(const float& other) {
	return Vector2(x + other, y + other);
}

Vector2 Vector2::operator*(const float& other) {
	return Vector2(x * other, y * other);
}

Vector2 Vector2::operator-(const float& other) {
	return Vector2(x - other, y - other);
}

Vector2 Vector2::operator/(const float& other) {
	return Vector2(x / other, y / other);
}

Vector2 Vector2::operator+=(const float& other) {
	return Vector2(x += other, y += other);
}

Vector2 Vector2::operator=(const glm::vec2& other)
{
	return Vector2(other.x, other.y);
}

glm::vec2 Vector2::operator=(const Vector2& other)
{
	return glm::vec2(other.x, other.y);
}
