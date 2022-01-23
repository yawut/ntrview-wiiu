#include "StatusOverlay.hpp"

StatusOverlay::StatusOverlay(const std::string& host) :
    connecting_text(""),
    attempt_text(", attempt "),
    connected_text("Connected."),
    bad_ip_text("Bad IP - press Left Stick for menu") {
    ChangeHost(host);
}

void StatusOverlay::Render() {
    int height = Gfx::GetCurrentScreenHeight();
    if (network_state == Network::CONNECTING) {
        int x = connecting_text.baseline_y;
        connecting_text.Render(x, height - connecting_text.d.h);
        x += connecting_text.d.w;

        int connect_attempts = Network::GetConnectionAttempts();
        if (connect_attempts > 0) {
            attempt_text.Render(x, height - attempt_text.d.h);
            x += attempt_text.d.w;

            Text::Text attempts_num(std::to_string(connect_attempts));
            attempts_num.Render(x, height - attempts_num.d.h);
            x += attempts_num.d.w;
        }
    } else if (network_state == Network::CONNECTED_WAIT) {
        connected_text.Render(connected_text.baseline_y, height - connected_text.d.h);
    } else if (network_state == Network::ERR_BAD_IP) {
        bad_ip_text.Render(bad_ip_text.baseline_y, height - bad_ip_text.d.h);
    }
}
