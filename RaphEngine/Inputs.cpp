#include "pch.h"
#include "include/Inputs.h"
#include "include/Renderer.h"
#include "SDL.h"

bool Inputs::IsKeyPressed(KeyCode key) {
	bool isPressed = Renderer::IsKeyPressed(key);
	return isPressed;
}
bool Inputs::IsMouseButtonPressed(int button){
	return glfwGetMouseButton(Renderer::GetWindow(), button);
}
Vector2 Inputs::GetMousePos(){
	double x, y;
	//SDL_GetMouseState(&x, &y);
	glfwGetCursorPos(Renderer::GetWindow(), &x, &y);
	return Vector2(x, y);
}

double Inputs::GetMouseScroll(){
	return 0;
}
void Inputs::SetMousePosition(double x, double y){
	SDL_WarpMouseInWindow(NULL, x, y);
}

