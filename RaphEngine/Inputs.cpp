#include "pch.h"
#include "include/Inputs.h"
#include "include/Renderer.h"
#include "SDL.h"

bool Inputs::IsKeyPressed(KeyCode key) {
	bool isPressed = Renderer::IsKeyPressed(key);
	return isPressed;
}
bool Inputs::IsMouseButtonPressed(MouseButton button){
	return glfwGetMouseButton(Renderer::GetWindow(), (int)button);
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
	glfwSetCursorPos(Renderer::GetWindow(), x, y);
}

void Inputs::SetMouseVisibility(bool visible)
{
	if(visible)
		glfwSetInputMode(Renderer::GetWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	else
		glfwSetInputMode(Renderer::GetWindow(), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
}
