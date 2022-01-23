#pragma once

#include <string>

#include "gfx/Gfx.hpp"
#include "gfx/font/Text.hpp"
#include "Network.hpp"

class StatusOverlay {
public:
    StatusOverlay(const std::string& host);
    void Render();
    void ChangeHost(const std::string& host) {
        this->connecting_text.Change("Connecting to " + host);
    }
    void NetworkState(Network::State networkState) {
        this->network_state = networkState;
    };
    //void InputPriority()

private:
    Network::State network_state;

    Text::Text connecting_text;
    Text::Text attempt_text;
    Text::Text connected_text;
    Text::Text bad_ip_text;
};
