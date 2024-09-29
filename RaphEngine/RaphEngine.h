#pragma once


#ifdef RAPHENGINE_EXPORTS
#define RAPHENGINE_API __declspec(dllexport)
#else
#define RAPHENGINE_API __declspec(dllimport)
#endif

extern "C" RAPHENGINE_API int RaphEngineAdd(int a, int b);