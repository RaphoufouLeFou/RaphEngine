#include "pch.h"
#include "include/Inputs.h"
#include "include/Renderer.h"
#include "SDL.h"

bool Key::KEYS[322] = { false };

bool Inputs::IsKeyPressed(Key::KeyCode key) {
	bool isPressed = Renderer::IsKeyPressed(key);
	return isPressed;
}
bool Inputs::IsMouseButtonPressed(int button){
	const Uint32 state = SDL_GetMouseState(NULL, NULL);
	if (state & SDL_BUTTON(button)) {
		return true;
	}
	return false;
}
double Inputs::GetMouseX(){
	int x;
	SDL_GetMouseState(&x, NULL);
	return x;
}
double Inputs::GetMouseY(){
	int y;
	SDL_GetMouseState(NULL, &y);
	return y;
}
double Inputs::GetMouseScroll(){
	return 0;
}
void Inputs::SetMousePosition(double x, double y){
	SDL_WarpMouseInWindow(NULL, x, y);
}

