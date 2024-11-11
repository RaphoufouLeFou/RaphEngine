#pragma once

#ifdef RAPHENGINE_EXPORTS
#define RAPHENGINE_API __declspec(dllexport)
#else
#define RAPHENGINE_API __declspec(dllimport)
#endif

#include <vector>
#include "Vector.h"
#include "GameObject.h"
#include "ObjLoader.h"
#include "Text.h"
#include "Images.h"
#include "Inputs.h"

typedef char byte;

class RAPHENGINE_API Time {
public:
	static double deltaTime;
	static double GetTime();
};

class RaphEngine
{
public:

	static const char* windowTitle;
	static RAPHENGINE_API void Init(const char* windowTitle);
	static RAPHENGINE_API void Run();
	static RAPHENGINE_API void GetWindowSize(int* x, int* y);

	static std::vector<void (*)()> Starts;
	static std::vector<void (*)()> Updates;
	static RAPHENGINE_API void UpdateLogo(const char * newLogoPath);
	static RAPHENGINE_API Camera* camera;
};
