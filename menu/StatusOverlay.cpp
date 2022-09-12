#include "StatusOverlay.hpp"

StatusOverlay::StatusOverlay(const std::string& host) :
    connecting_text(u""),
    attempt_text(u", attempt "),
    connected_text(u"Connected."),
    bad_ip_text(u"Bad IP - press Left Stick for menu"),
    input_priority_text(u""),
    menu_input_text(u"\uE08A Menu") {
    ChangeHost(host);
}

const static Gfx::Rect pad = (Gfx::Rect) {
    .x = 30,
    .y = 30,
};

void StatusOverlay::Render() {
    int width = Gfx::GetCurrentScreenWidth();
    int height = Gfx::GetCurrentScreenHeight();
    if (network_state == Network::CONNECTING) {
        int x = pad.x;
        connecting_text.Render(x, height - pad.y, text_colour);
        x += connecting_text.d.w;

        int connect_attempts = Network::GetConnectionAttempts();
        if (connect_attempts > 0) {
            attempt_text.Render(x, height - pad.y, text_colour);
            x += attempt_text.d.w;

            Text::Text attempts_num(to_u16string(std::to_string(connect_attempts)));
            attempts_num.Render(x, height - pad.y, text_colour);
            x += attempts_num.d.w;
        }
    } else if (network_state == Network::CONNECTED_WAIT) {
        connected_text.Render(pad.x, height - pad.y, text_colour);
    } else if (network_state == Network::ERR_BAD_IP) {
        bad_ip_text.Render(pad.x, height - pad.y, text_colour);
    } else if (input_message_timeout) {
        if (OSGetTime() < input_message_timeout) {
            input_priority_text.Render(pad.x, height - pad.y, text_colour);
        } else input_message_timeout = 0;
    }

    if (network_state != Network::CONNECTED_STREAMING) {
        menu_input_text.Render(width - pad.x - menu_input_text.d.w, height - pad.y, text_colour);
    }
}

void StatusOverlay::InputPriorityMessage(Input::Priority priority, Input::ExtType extension) {
    this->input_message_timeout = OSGetTime() + OSSecondsToTicks(3);

    if (priority == Input::Priority::VPAD) {
        this->input_priority_text.Change(u"Using GamePad");
        return;
    }
    std::u16string msg = u"Using ";
    switch (extension) {
        case Input::ExtType::Core: {
            msg += u"Wii Remote ";
            break;
        }
        case Input::ExtType::Nunchuk: {
            msg += u"Nunchuk ";
            break;
        }
        case Input::ExtType::Classic: {
            msg += u"Classic Controller ";
            break;
        }
        case Input::ExtType::Pro: {
            msg += u"Pro Controller ";
            break;
        }
        default: {
            msg += u"Controller ";
            break;
        }
    }
    switch (priority) {
        case Input::Priority::KPAD1: {
            msg += u"#1";
            break;
        }
        case Input::Priority::KPAD2: {
            msg += u"#2";
            break;
        }
        case Input::Priority::KPAD3: {
            msg += u"#3";
            break;
        }
        case Input::Priority::KPAD4: {
            msg += u"#4";
            break;
        }
        default: break;
    }
    this->input_priority_text.Change(msg);
}
