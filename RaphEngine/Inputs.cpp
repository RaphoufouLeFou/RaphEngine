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

bool Inputs::IsMouseOnScreen()
{
    double x, y;
    glfwGetCursorPos(Renderer::GetWindow(), &x, &y);
    int* resX = Renderer::ResX;
    int* resY = Renderer::ResY;
    return (x >= 0 && x <= *resX && y >= 0 && y <= *resY);
}

bool Inputs::IsWindowFocused()
{
    return glfwGetWindowAttrib(Renderer::GetWindow(), GLFW_FOCUSED);
}