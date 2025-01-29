#pragma once

#include "RaphEngine.h"
#include <functional>


class RAPHENGINE_API UIElement
{
public:
	virtual void DrawUI() {}
	Transform* transform;
	bool visible = true;
#ifdef RAPHENGINE_EXPORTS
	static std::vector<UIElement*> Elements;
	static void DrawAllUI();
#endif
};

class RAPHENGINE_API TextUI : public UIElement
{
public:
	std::string text;
	Vector3 color;
	void DrawUI();
	TextUI(std::string text = "", Vector3 color = Vector3(0,0,0), Vector3 position = Vector3(0, 0, 0));
};

class RAPHENGINE_API ImageUI : public UIElement
{
public:
	std::string texture;
	void DrawUI();
	ImageUI(std::string texture = "", Vector3 position = Vector3(0, 0, 0));
};

class RAPHENGINE_API ButtonUI : public UIElement
{
public:
	std::string texture;
	std::string text;
	Vector3 color;
	bool clickable = true;
	void DrawUI();
	ButtonUI(std::string texture = "", std::string text = "", Vector3 color = Vector3(0, 0, 0), Vector3 position = Vector3(0, 0, 0));
	void SetOnClick(std::function<void()> onClick);

#ifdef RAPHENGINE_EXPORTS
	static std::vector<ButtonUI*> Buttons;
	static void CheckAllClicks(Vector2 mousePos);
#endif

private:
	std::function<void()> onClick;
};