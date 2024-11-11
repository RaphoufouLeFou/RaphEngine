#include "pch.h" // use stdafx.h in Visual Studio 2017 and earlier

#include <utility>
#include <limits.h>
#define SDL_MAIN_HANDLED
#include "SDL.h"
#include "include/RaphEngine.h"
#include "include/Renderer.h"
#include <iostream>

const char * RaphEngine::windowTitle = "RaphEngine";
Camera* RaphEngine::camera = new Camera();

void ExecuteStarts() {
	for (size_t i = 0; i < GameObject::SpawnedGameObjects.size(); i++)
	{
		std::cout << "Initing " << GameObject::SpawnedGameObjects[i] << std::endl;
		GameObject* go = GameObject::SpawnedGameObjects[i];
		go->Init();
		std::cout << "shader is null on " << go->name << " 2 ? : " << (go->mesh->shader == nullptr) << std::endl;
		go->Start();
		std::cout << "shader is null on " << go->name << " 3 ? : " << (go->mesh->shader == nullptr) << std::endl;

	}
}

void ExecuteUpdates() {
	for (size_t i = 0; i < GameObject::SpawnedGameObjects.size(); i++)
		GameObject::SpawnedGameObjects[i]->Update();
}

void Close() {
	SDL_Quit();
}


void MainLoop() {
	// Main loop
	while (true) {
		// Handle events
		bool shouldNotClose = Renderer::RenderFrame();
		if (!shouldNotClose) {
			Close();
			break;
		}
		ExecuteUpdates();
	}
}

void RaphEngine::Init(const char* windowTitle) {

	for (int i = 0; i < 322; i++) { // init them all to false
		Key::KEYS[i] = false;
	}
	RaphEngine::windowTitle = windowTitle;
	SDL_Init(SDL_INIT_EVERYTHING);
	Renderer::Init(false);
	ExecuteStarts();
}

void RaphEngine::Run() {
	MainLoop();
}

void RaphEngine::UpdateLogo(const char* newLogoPath) {
	Renderer::UpdateLogoGL(newLogoPath);
}

