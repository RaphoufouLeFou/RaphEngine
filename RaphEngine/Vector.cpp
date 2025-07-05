#include "pch.h"
#include "include/RaphEngine.h"
#include "include/Vector.h"

#ifdef _WIN32

#include <gtx/quaternion.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#else

#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#endif

Matrix4 GetRotationMatrix(Vector3 rotation)
{
    return glm::toMat4(glm::quat(glm::radians(rotation)));
}
