#include "pch.h" // use stdafx.h in Visual Studio 2017 and earlier
#include "RaphEngine.h"

// DLL internal state variables:
static unsigned long long previous_;  // Previous value, if any
static unsigned long long current_;   // Current sequence value
static unsigned index_;               // Current seq. position

int RaphEngineAdd(int a, int b)
{
	return a + b;
}