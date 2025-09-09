#pragma once

#include "RaphEngine.h"
#include <functional>
#include <map>

enum class UISnapPoint
{
	TOP_LEFT,
	TOP_RIGHT,
	BOTTOM_LEFT,
	BOTTOM_RIGHT,
	CENTER,
};

class RAPHENGINE_API UIElement
{
public:
	virtual void DrawUI() {}
	static void RecalculateTransforms();
	Transform* transform;
	Transform* CalculatedTransform;
	bool visible = true;
#ifdef RAPHENGINE_EXPORTS
	static std::vector<UIElement*> Elements;
	static void DrawAllUI();
#endif
	UISnapPoint SnapPoint;
protected:
	void CalculateSnapedPosition();
};

class RAPHENGINE_API TextUI : public UIElement
{
public:
	std::string text;
	Vector3 color;
	void DrawUI();
	TextUI(std::string text = "", Vector3 color = Vector3(0,0,0), Vector3 position = Vector3(0, 0, 0), UISnapPoint SnapPoint = UISnapPoint::TOP_LEFT);
};

class RAPHENGINE_API ImageUI : public UIElement
{
public:
	std::string texture;
	void DrawUI();
	ImageUI(std::string texture = "", Vector3 position = Vector3(0, 0, 0), UISnapPoint SnapPoint = UISnapPoint::TOP_LEFT);
};

class RAPHENGINE_API ButtonUI : public UIElement
{
public:
	std::string texture;
	std::string text;
	Vector3 color;
	bool clickable = true;
	void DrawUI();
	ButtonUI(std::string texture = "", std::string text = "", Vector3 color = Vector3(0, 0, 0), Vector3 position = Vector3(0, 0, 0), UISnapPoint SnapPoint = UISnapPoint::TOP_LEFT);
	void SetOnClick(std::function<void()> onClick);

#ifdef RAPHENGINE_EXPORTS
	static std::vector<ButtonUI*> Buttons;
	static void CheckAllClicks(Vector2 mousePos);
#endif

private:
	std::function<void()> onClick;
};

class RAPHENGINE_API UIPanel
{
public:
	static bool LoadPanels(std::string Path);
	static UIElement GetElement(int id);
	static UIElement GetElement(std::string name);
	bool PanelActive = true;

private:
	static std::vector<UIElement> Elements;
	static std::map<std::string, int> NameToIndexes;
};