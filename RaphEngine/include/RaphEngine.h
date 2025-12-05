#pragma once
#include "LibManager.h"

#include <vector>
#include "Vector.h"
#include "GameObject.h"
#include "ObjLoader.h"
#include "Text.h"
#include "Images.h"
#include "Inputs.h"
#include "UI.h"
#include "RayCast.h"
#include "Guizmo.h"
#include "Shader.h"
#include "Settings.h"
#include "JSON.h"

typedef char byte;

class RAPHENGINE_API Time {
public:
	static double deltaTime;
	static double GetTime();
};

class RaphEngine
{
public:
	static RAPHENGINE_API void PassShaderVector3(char* name, Vector3 value);
	static RAPHENGINE_API void SetSkyBox(std::vector<std::string> skyboxTextures);
	static RAPHENGINE_API GameObject *Player;
	static RAPHENGINE_API void Init(const char* windowTitle, std::string font_name);
	static RAPHENGINE_API void Run();
	static RAPHENGINE_API void GetWindowSize(int* x, int* y);
	static RAPHENGINE_API void UpdateLogo(const char * newLogoPath);
	static RAPHENGINE_API Camera* camera;

	static std::vector<void (*)()> Starts;
	static std::vector<void (*)()> Updates;
	static const char* windowTitle;

};
