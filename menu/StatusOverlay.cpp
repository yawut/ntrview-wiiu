#include "StatusOverlay.hpp"

StatusOverlay::StatusOverlay(const std::string& host) :
    connecting_text(""),
    attempt_text(", attempt "),
    connected_text("Connected."),
    bad_ip_text("Bad IP - press Left Stick for menu"),
    input_priority_text("") {
    ChangeHost(host);
}

const static Gfx::Rect pad = (Gfx::Rect) {
    .x = 30,
    .y = 30,
};

void StatusOverlay::Render() {
    int height = Gfx::GetCurrentScreenHeight();
    if (network_state == Network::CONNECTING) {
        int x = pad.x;
        connecting_text.Render(x, height - pad.y);
        x += connecting_text.d.w;

        int connect_attempts = Network::GetConnectionAttempts();
        if (connect_attempts > 0) {
            attempt_text.Render(x, height - pad.y);
            x += attempt_text.d.w;

            Text::Text attempts_num(std::to_string(connect_attempts));
            attempts_num.Render(x, height - pad.y);
            x += attempts_num.d.w;
        }
    } else if (network_state == Network::CONNECTED_WAIT) {
        connected_text.Render(pad.x, height - pad.y);
    } else if (network_state == Network::ERR_BAD_IP) {
        bad_ip_text.Render(pad.x, height - pad.y);
    } else if (input_message_timeout) {
        if (OSGetTime() < input_message_timeout) {
            input_priority_text.Render(pad.x, height - pad.y);
        } else input_message_timeout = 0;
    }
}

void StatusOverlay::InputPriorityMessage(Input::Priority priority, Input::ExtType extension) {
    this->input_message_timeout = OSGetTime() + OSSecondsToTicks(3);

    if (priority == Input::Priority::VPAD) {
        this->input_priority_text.Change("Using GamePad");
        return;
    }
    std::string msg = "Using ";
    switch (extension) {
        case Input::ExtType::Core: {
            msg += "Wii Remote ";
            break;
        }
        case Input::ExtType::Nunchuk: {
            msg += "Nunchuk ";
            break;
        }
        case Input::ExtType::Classic: {
            msg += "Classic Controller ";
            break;
        }
        case Input::ExtType::Pro: {
            msg += "Pro Controller ";
            break;
        }
        default: {
            msg += "Controller ";
            break;
        }
    }
    switch (priority) {
        case Input::Priority::KPAD1: {
            msg += "#1";
            break;
        }
        case Input::Priority::KPAD2: {
            msg += "#2";
            break;
        }
        case Input::Priority::KPAD3: {
            msg += "#3";
            break;
        }
        case Input::Priority::KPAD4: {
            msg += "#4";
            break;
        }
        default: break;
    }
    this->input_priority_text.Change(msg);
}
