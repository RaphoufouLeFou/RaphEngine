#pragma once
#include "LibManager.h"

#include "UI.h"

#include <jsoncpp/json/json.h>
#include <vector>
#include <string>

class RAPHENGINE_API JSON_UI_Manager
{
public:
    static std::vector<UIElement *> LoadUIElementFromJSON(std::string path);
private:
    static Json::Value LoadJSONFile(const char* path);
    static bool SaveJSONFile(const char* path, Json::Value root);
};
