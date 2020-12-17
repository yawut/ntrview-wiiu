#pragma once

#include <string>

#include "gfx/Gfx.hpp"
#include "gfx/font/Text.hpp"
#include "Network.hpp"

class StatusOverlay {
public:
    StatusOverlay(const std::string& host);
    void Render(Network::State networkState);

private:
    Text::Text connecting_text;
    Text::Text attempt_text;
    Text::Text connected_text;
    Text::Text bad_ip_text;
};
