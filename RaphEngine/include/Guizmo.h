#pragma once
#include "LibManager.h"

#include "GameObject.h"

class Guizmo : GameObject
{
public:
    Transform *transform;
    GameObject *attatchedGameObject;

    Guizmo(Transform *t);
    Guizmo(Transform *t, GameObject *go);
    Guizmo(Vector3 pos);
    Guizmo(Vector3 pos, Vector3 rot);
};