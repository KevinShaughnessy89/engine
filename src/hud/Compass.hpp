#pragma once
#include "core/PrecompiledHeader.hpp"

#include "HUDElement.hpp"

class Compass : public HUDElement {

    public:
        float radians;
        void update() override;
        void render() override {}

};
