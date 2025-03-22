#include "pch.h"

#include "include/UI.h"

std::vector<UIElement*> UIElement::Elements;
std::vector<ButtonUI*> ButtonUI::Buttons;

void TextUI::DrawUI()
{
	if (this->text == "" || this->visible == false)
	{
		return;
	}
	Vector3 position = this->transform->GetPosition();
	Text::RenderText(this->text.c_str(), position.x, position.y, this->transform->GetScale().y, this->color);
}

TextUI::TextUI(std::string text, Vector3 color, Vector3 position)
{
	UIElement::Elements.push_back(this);
	this->text = text;
	this->color = color;
	this->transform = new Transform();
	this->transform->SetPosition(position);
}

void ImageUI::DrawUI()
{
	if(this->texture == "" || this->visible == false)
	{
		return;
	}
	Vector3 position = this->transform->GetPosition();
	Vector3 scale = this->transform->GetScale();
	Image::RenderImage(this->texture, position.x, position.y, scale.x, scale.y, position.z);
}

ImageUI::ImageUI(std::string texture, Vector3 position)
{
	this->texture = texture;
	this->transform = new Transform();
	this->transform->SetPosition(position);
	UIElement::Elements.push_back(this);
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

ButtonUI::ButtonUI(std::string texture, std::string text, Vector3 color, Vector3 position)
{
	this->texture = texture;
	this->text = text;
	this->color = color;
	this->transform = new Transform();
	this->transform->SetPosition(position);
	UIElement::Elements.push_back(this);
	ButtonUI::Buttons.push_back(this);
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
	int length = UIElement::Elements.size();
	for (int i = 0; i < length; i++)
	{
		UIElement* element = UIElement::Elements[i];
		element->DrawUI();
	}
}

bool UIPanel::LoadPanels(std::string Path)
{
	// TODO
}

UIElement UIPanel::GetElement(int id)
{
	return Elements[id];
}

UIElement UIPanel::GetElement(std::string name)
{
	return Elements[NameToIndexes[name]];
}
