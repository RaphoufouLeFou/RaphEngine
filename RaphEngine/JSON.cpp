#include "pch.h"
#include "include/JSON.h"
#include "magic_enum.hpp"

Json::Value JSON_UI_Manager::LoadJSONFile(const char* path)
{
    Json::Value root;
    std::ifstream file(path, std::ifstream::binary);
    if (!file.is_open())
    {
        std::cerr << "Could not open JSON file: " << path << std::endl;
        return root;
    }

    file >> root;
    return root;
}

bool JSON_UI_Manager::SaveJSONFile(const char* path, Json::Value root)
{
    std::ofstream file(path, std::ofstream::binary);
    if (!file.is_open())
    {
        std::cerr << "Could not open JSON file for writing: " << path << std::endl;
        return false;
    }

    file << root;
    return true;
}


std::vector<UIElement*> JSON_UI_Manager::LoadUIElementFromJSON(std::string path)
{
/* 
FORMAT :
{
    "images" :
    [
        {
            "name" : "InfoImage",
            "path" : "Assets/UI/info.png",
            "position" : [50, 50, 1],
            "size" : [50, 50],
            "snapPoint" : "BOTTOM_RIGHT"
        }
    ],
    "texts" :
    [
        {
            "name" : "WelcomeText",
            "text" : "Hello World!",
            "position" : [0, 0, 0],
            "color" : [255, 255, 255],
            "fontSize" : 32,
            "snapPoint" : "CENTER"
        }
    ]
}
*/
    Json::Value fileData = LoadJSONFile(path.c_str());
    std::vector<UIElement*> uiElements;
    Json::Value images = fileData["images"];
    for (Json::Value& imageData : images)
    {
        std::string name = imageData["name"].asString();
        std::string path = imageData["path"].asString();
        Json::Value positionData = imageData["position"];
        Vector3 position = Vector3(positionData[0].asFloat(), positionData[1].asFloat(), positionData[2].asFloat());
        Json::Value sizeData = imageData["size"];
        Vector3 size = Vector3(sizeData[0].asFloat(), sizeData[1].asFloat(), 1);
        std::string snapPointStr = imageData["snapPoint"].asString();
        UISnapPoint snapPoint = magic_enum::enum_cast<UISnapPoint>(snapPointStr).value_or(UISnapPoint::TOP_LEFT);
        ImageUI* imageUI = new ImageUI(name, path, position, size, snapPoint);
        uiElements.push_back(imageUI);
    }
    Json::Value texts = fileData["texts"];
    for (Json::Value& textData : texts)
    {
        std::string name = textData["name"].asString();
        std::string text = textData["text"].asString();
        Json::Value positionData = textData["position"];
        Vector3 position = Vector3(positionData[0].asFloat(), positionData[1].asFloat(), positionData[2].asFloat());
        Json::Value colorData = textData["color"];
        Vector3 color = Vector3(colorData[0].asFloat(), colorData[1].asFloat(), colorData[2].asFloat());
        float fontSize = textData["fontSize"].asFloat();
        std::string snapPointStr = textData["snapPoint"].asString();
        UISnapPoint snapPoint = magic_enum::enum_cast<UISnapPoint>(snapPointStr).value_or(UISnapPoint::TOP_LEFT);
        TextUI* textUI = new TextUI(name, text, color, position, fontSize, snapPoint);
        uiElements.push_back(textUI);
    }
    return uiElements;
}