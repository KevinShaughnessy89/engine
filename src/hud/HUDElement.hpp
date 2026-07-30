#pragma once
#include "core/PrecompiledHeader.hpp"



class Texture;

class HUDElement {

    public:
        HUDElement(const std::string& xmlData);
        ~HUDElement() = default;

        // data members
        std::string id;
        glm::vec2 position;
        glm::vec2 size;  // UVector2 for size as it can handle different types of measurements
        bool visible;
        std::string type;
        std::map<std::string, std::string> properties;
        Texture* texture;

        // functions
        void parseXML();
        virtual void update() = 0;
        virtual void render() = 0;
        void setSize(glm::vec2 size) { this->size = size; }
        void setPosition(glm::vec2 position) { this->position = position; }
        void setVisibility(bool visibility) { this->visible = visibility; } 
};
