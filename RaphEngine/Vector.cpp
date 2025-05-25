#include "pch.h"
#include "include/RaphEngine.h"
#include "include/Vector.h"
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

Matrix4 GetRotationMatrix(Vector3 rotation)
{
    return glm::toMat4(glm::quat(glm::radians(rotation)));
}
