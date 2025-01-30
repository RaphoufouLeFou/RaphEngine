#include "pch.h"
#include "include/RayCast.h"
#include "include/Renderer.h"
#include "optional"
#include <limits>

using namespace glm;


vec3 GetDirectionFromScreen(Vector2 screenPos)
{
    float ndcX = (2.0f * screenPos.x) / *Renderer::ResX -1.0f;
    float ndcY = 1.0f - (2.0f * screenPos.y) / *Renderer::ResY; // Flip Y

    // Clip Space Coordinates
    vec4 clipCoords(ndcX, ndcY, -1.0f, 1.0f);

    // Eye Space (View Space)

    vec4 eyeCoords = inverse(Renderer::GetProjectionMatrix()) * clipCoords;
    eyeCoords = vec4(eyeCoords.x, eyeCoords.y, -1.0, 0.0); // w = 0 for direction

    // World Space
    vec4 worldCoords = inverse(Renderer::GetViewMatrix()) * eyeCoords;
    vec3 rayDirection = normalize(glm::vec3(worldCoords));
	return rayDirection;
}

typedef struct triangle
{
	vec3 a;
	vec3 b;
	vec3 c;
} triangle;


// this function is from wikipedia
bool ray_intersects_triangle(const vec3& ray_origin,
    const vec3& ray_vector,
    const triangle& triangle, vec3& out_intersection_point)
{
    constexpr float epsilon = std::numeric_limits<float>::epsilon();

    vec3 edge1 = triangle.b - triangle.a;
    vec3 edge2 = triangle.c - triangle.a;
    vec3 ray_cross_e2 = cross(ray_vector, edge2);
    float det = dot(edge1, ray_cross_e2);

    if (det > -epsilon && det < epsilon)
        return false;    // This ray is parallel to this triangle.

    float inv_det = 1.0 / det;
    vec3 s = ray_origin - triangle.a;
    float u = inv_det * dot(s, ray_cross_e2);

    if ((u < 0 && abs(u) > epsilon) || (u > 1 && abs(u - 1) > epsilon))
        return false;

    vec3 s_cross_e1 = cross(s, edge1);
    float v = inv_det * dot(ray_vector, s_cross_e1);

    if ((v < 0 && abs(v) > epsilon) || (u + v > 1 && abs(u + v - 1) > epsilon))
        return false;

    // At this stage we can compute t to find out where the intersection point is on the line.
    float t = inv_det * dot(edge2, s_cross_e1);

    if (t > epsilon) // ray intersection
    {
        out_intersection_point = vec3(ray_origin + ray_vector * t);
        return true;
    }
    else // This means that there is a line intersection but not a ray intersection.
        return false;
}

bool InInfluenceSphere(vec3 rayOrigin, vec3 rayDirection, vec3 center, float radius)
{
	vec3 oc = center - rayOrigin;
    float a = dot(rayDirection, rayDirection);
    float b = 2.0f * dot(rayDirection, oc);
    float c = dot(oc, oc) - radius * radius;
    float discriminant = b * b - 4 * a * c;
    return (discriminant > 0);
}

bool haveCollision(vec3 origin, vec3 direction, triangle tri, vec3& out_intersection_point, vec3& out_normal, GameObject** objOut, int layer)
{
    vec3 oldIntersectionPoint;
    bool haveCollision = false;

    int objCount = GameObject::SpawnedGameObjects.size();
    for (int i = 0; i < objCount; i++)
    {
		GameObject* obj = GameObject::SpawnedGameObjects[i];
        if (obj->layer != layer)
            continue;
        int meshCount = obj->meshes.size();
        mat4 model = obj->transform->ModelMatrix;
        for (int j = 0; j < meshCount; j++)
        {
			Mesh mesh = obj->meshes[j];
            vec3 Scale = obj->transform->GetScale();
            float maxScale = max(max(Scale.x, Scale.y), Scale.z);
            if(!InInfluenceSphere(origin, direction, model * vec4(mesh.InfSpehereCenter, 1), maxScale * mesh.InfSphereRadius))
                continue;
			int triCount = mesh.indices.size() / 3;
            
            for (int k = 0; k < triCount; k++)
            {
				triangle objTri;
                objTri.a = model * vec4(mesh.vertices[mesh.indices[k * 3]].Position, 1);
                objTri.b = model * vec4(mesh.vertices[mesh.indices[k * 3 + 1]].Position, 1);
                objTri.c = model * vec4(mesh.vertices[mesh.indices[k * 3 + 2]].Position, 1);
                if (!ray_intersects_triangle(origin, direction, objTri, out_intersection_point))
                    continue;
                if (haveCollision == false || distance2(out_intersection_point, origin) < distance2(oldIntersectionPoint, origin))
                {
					oldIntersectionPoint = out_intersection_point;
					objOut = &obj;
                    haveCollision = true;
                    out_normal = normalize(cross(objTri.b - objTri.a, objTri.c - objTri.a));
				}
			}
		}
	}
    return haveCollision;
}

bool RayCast::FromCamera(Vector2 screenPos, RayInfo* OutRayInfo, int layer)
{
    vec3 direction = GetDirectionFromScreen(screenPos);
	Vector3 camPos = RaphEngine::camera->transform->GetPosition();
    Vector3 directionV3 = Vector3(direction.x, direction.y, direction.z);
    return FromPoint(camPos, directionV3, OutRayInfo, layer);
}
bool RayCast::FromMouse(RayInfo* OutRayInfo, int layer)
{
    Vector2 screenPos = Inputs::GetMousePos();
	return FromCamera(screenPos, OutRayInfo, layer);
}

bool RayCast::FromPoint(Vector3 origin, Vector3 direction, RayInfo* OutRayInfo, int layer)
{
    vec3 intersectionPoint;
    vec3 normal;
    GameObject* objOut = nullptr;
    if (haveCollision(origin, direction, triangle(), intersectionPoint, normal, &objOut, layer))
    {
		OutRayInfo->hitPoint.x = intersectionPoint.x;
        OutRayInfo->hitPoint.y = intersectionPoint.y;
        OutRayInfo->hitPoint.z = intersectionPoint.z;
        OutRayInfo->hitNormal.x = normal.x;
        OutRayInfo->hitNormal.y = normal.y;
        OutRayInfo->hitNormal.z = normal.z;
		OutRayInfo->hitObject = objOut;
		return true;
	}
	return false;
}