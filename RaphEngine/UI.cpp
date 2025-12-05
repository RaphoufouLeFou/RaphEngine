#include "pch.h"

#include "include/UI.h"
#include "include/Renderer.h"

std::vector<UIElement*> UIElement::Elements;
std::vector<ButtonUI*> ButtonUI::Buttons;
std::vector<UIElement> UIPanel::Elements;
std::map<std::string, int> UIPanel::NameToIndexes;

void TextUI::DrawUI()
{
	if (this->text == "" || this->visible == false)
	{
		return;
	}
	Vector3 position = this->CalculatedTransform->GetPosition();
	Text::RenderText(this->text.c_str(), position.x, position.y, this->transform->GetScale().y, this->color);
}

TextUI::TextUI(std::string name, std::string text, Vector3 color, Vector3 position, float fontSize, UISnapPoint SnapPoint)
{
	UIElement::Elements.push_back(this);
    if(!name.empty())
		this->name = name;
    else
		this->name = "TextUI_" + std::to_string(UIElement::Elements.size());
	this->text = text;
	this->color = color;
	this->transform = new Transform();
	this->transform->SetPosition(position);
    this->transform->SetScale(Vector3(1, fontSize, 1));
	this->SnapPoint = SnapPoint;
	CalculateSnapedPosition();
}

void ImageUI::DrawUI()
{
	if(this->texture == "" || this->visible == false)
	{
		return;
	}
    CalculateSnapedPosition();
	Vector3 position = this->transform->GetPosition();
	Vector3 scale = this->transform->GetScale();
	Image::RenderImage(this->texture, position.x, position.y, scale.x, scale.y, position.z);
}

ImageUI::ImageUI(std::string name, std::string texture, Vector3 position, Vector3 scale, UISnapPoint SnapPoint)
{
    if(!name.empty())
		this->name = name;
    else
		this->name = "ImageUI_" + std::to_string(UIElement::Elements.size());
	this->texture = texture;
	this->transform = new Transform();
	this->transform->SetPosition(position);
	this->transform->SetScale(scale);
	this->SnapPoint = SnapPoint;
	UIElement::Elements.push_back(this);
	CalculateSnapedPosition();
}

void ButtonUI::DrawUI()
{
	if (this->texture == "" || this->visible == false)
	{
		return;
	}
	Vector3 position = this->transform->GetPosition();
	Vector3 scale = this->transform->GetScale();
	Image::RenderImage(this->texture, position.x, position.y, scale.x, scale.y, position.z);
	Text::RenderText(this->text.c_str(), position.x, position.y, scale.z, this->color);
}

ButtonUI::ButtonUI(std::string texture, std::string text, Vector3 color, Vector3 position, UISnapPoint SnapPoint)
{
	this->texture = texture;
	this->text = text;
	this->color = color;
	this->transform = new Transform();
	this->transform->SetPosition(position);
	this->SnapPoint = SnapPoint;
	UIElement::Elements.push_back(this);
	ButtonUI::Buttons.push_back(this);
	CalculateSnapedPosition();
}

void ButtonUI::CheckAllClicks(Vector2 mousePos)
{
	for (ButtonUI* button : ButtonUI::Buttons)
	{
		if (!button->visible || !button->clickable)
		{
			continue;
		}
		Vector3 position = button->transform->GetPosition();
		Vector3 scale = button->transform->GetScale();
		if (mousePos.x >= position.x && mousePos.x <= position.x + scale.x &&
			mousePos.y <= position.y && mousePos.y >= position.y - scale.y)
		{
			if(button->onClick != NULL)
				button->onClick();
		}
	}
}

void ButtonUI::SetOnClick(std::function<void()> onClick)
{
	this->onClick = onClick;
}

void UIElement::DrawAllUI()
{
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	int length = UIElement::Elements.size();
	for (int i = 0; i < length; i++)
	{
		UIElement* element = UIElement::Elements[i];
		element->DrawUI();
	}
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_LIGHTING);
}

void UIElement::CalculateSnapedPosition()
{
	Vector3 pos = transform->GetPosition();
	CalculatedTransform = new Transform(transform);
	Vector3 CalculatedPosition;
	Vector2 ScreenSize = Vector2(*(Renderer::ResX), *(Renderer::ResY));

	switch (SnapPoint)
	{
		case UISnapPoint::TOP_LEFT:
			CalculatedPosition = Vector3(pos.x, pos.y, pos.z);
			break;
		case UISnapPoint::TOP_RIGHT:
			CalculatedPosition = Vector3(ScreenSize.x - pos.x, pos.y, pos.z);
			break;
		case UISnapPoint::BOTTOM_LEFT:
			CalculatedPosition = Vector3(pos.x, ScreenSize.y - pos.y, pos.z);
			break;
		case UISnapPoint::BOTTOM_RIGHT:
			CalculatedPosition = Vector3(ScreenSize.x - pos.x, ScreenSize.y - pos.y, pos.z);
			break;
		case UISnapPoint::CENTER:
			CalculatedPosition = Vector3(ScreenSize.x / 2 - pos.x, ScreenSize.y / 2 - pos.y, pos.z);
			break;
		default: break;
	}
	CalculatedTransform->SetPosition(CalculatedPosition);

}

void UIElement::RecalculateTransforms()
{
	for (UIElement *UIe : UIElement::Elements)
	{
		UIe->CalculateSnapedPosition();
	}
}

bool UIPanel::LoadPanels(std::string Path)
{
	Path = Path + "UI/";
	// TODO
	return false;
}

UIElement UIPanel::GetElement(int id)
{
	return Elements[id];
}

UIElement UIPanel::GetElement(std::string name)
{
	return Elements[NameToIndexes[name]];
}
