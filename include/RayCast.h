#pragma once

#include "RaphEngine.h"

#include "LibManager.h"

typedef struct RayInfo
{
	Vector3 hitPoint;
	Vector3 hitNormal;
	float hitDistance;
	GameObject* hitObject;
} RayInfo;

class RAPHENGINE_API RayCast
{
public:
	static bool FromCamera(Vector2 screenPos, RayInfo* OutRayInfo, int layer = 0);
	static bool FromMouse(RayInfo* OutRayInfo, int layer = 0);
	static bool FromPoint(Vector3 origin, Vector3 direction, RayInfo* OutRayInfo, int layer = 0);
};

