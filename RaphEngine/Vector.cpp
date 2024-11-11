#include "pch.h"

#include "include/RaphEngine.h"


Vector3 Vector3::operator+(const Vector3& other) {
	return Vector3(x + other.x, y + other.y, z + other.z);
}

Vector3 Vector3::operator-(const Vector3& other) {
	return Vector3(x - other.x, y - other.y, z - other.z);
}

Vector3 Vector3::operator*(const Vector3& other) {
	return Vector3(x * other.x, y * other.y, z * other.z);
}

Vector3 Vector3::operator/(const Vector3& other){
	return Vector3(x / other.x, y / other.y, z / other.z);
}

Vector3 Vector3::operator+=(const Vector3& other){
	return Vector3(x += other.x, y += other.y, z += other.z);
}

Vector3 Vector3::operator+(const float& other) {
	return Vector3(x + other, y + other, z + other);
}

Vector3 Vector3::operator-(const float& other){
	return Vector3(x - other, y - other, z - other);
}

Vector3 Vector3::operator*(const float& other){
	return Vector3(x * other, y * other, z * other);
}

Vector3 Vector3::operator/(const float& other){
	return Vector3(x / other, y / other, z / other);
}

Vector3 Vector3::operator+=(const float& other){
	return Vector3(x += other, y += other, z += other);
}

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
