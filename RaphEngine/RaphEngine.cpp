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
GameObject* RaphEngine::Player = nullptr;

double Time::deltaTime = 0.0f;

#ifdef _WIN32_
extern "C" {
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;
}
#endif

void ExecuteStarts() {
	for (size_t i = 0; i < GameObject::SpawnedGameObjects.size(); i++)
	{
		GameObject* go = GameObject::SpawnedGameObjects[i];
		go->InitGO();
		go->Awake();
	}
    for (size_t i = 0; i < GameObject::SpawnedGameObjects.size(); i++)
    {
        GameObject* go = GameObject::SpawnedGameObjects[i];
        go->Start();
    }
}

void ExecuteUpdates() {
	for (size_t i = 0; i < GameObject::SpawnedGameObjects.size(); i++)
		GameObject::SpawnedGameObjects[i]->Update();
}

void Close() {
    for (size_t i = 0; i < GameObject::SpawnedGameObjects.size(); i++)
    {
        GameObject::SpawnedGameObjects[i]->OnExit();
        delete GameObject::SpawnedGameObjects[i];
    }
/*
    for (size_t i = 0; i < UIElement::Elements.size(); i++)
        delete UIElement::Elements[i];
    for (size_t i = 0; i < ButtonUI::Buttons.size(); i++)
        delete ButtonUI::Buttons[i];
    for (size_t i = 0; i < Shader::LoadedShaders.size(); i++)
        delete Shader::LoadedShaders[i];
        */
	SDL_Quit();
}

void MainLoop() {
	// Main loop
	while (true) {
		// Handle events

		double start = Time::GetTime();
		Renderer::StartFrameRender();
		ButtonUI::CheckAllClicks(Inputs::GetMousePos());
		ExecuteUpdates();
		bool shouldNotClose = Renderer::RenderFrame();
		if (!shouldNotClose) {
			Close();
			break;
		}
		Time::deltaTime = (Time::GetTime() - start) / 1000.0;
	}
}

void RaphEngine::Init(const char* windowTitle, std::string font_name) {
	RaphEngine::windowTitle = windowTitle;
	SDL_Init(SDL_INIT_EVERYTHING);
	Renderer::Init(false, font_name);
	Shader::Prepare();
}

void RaphEngine::Run() {
	ExecuteStarts();
	MainLoop();
}

void RaphEngine::GetWindowSize(int* x, int* y) {
	*x = *Renderer::ResX;
	*y = *Renderer::ResY;
}

void RaphEngine::UpdateLogo(const char* newLogoPath) {
	Renderer::UpdateLogoGL(newLogoPath);
}
