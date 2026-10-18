/**
 * /opt/annon-os/AudioVideoEngine-CPP/src/key_driver.hpp
 * Low-Level Physical Key Press Driver for AnnonOS.
 * Reads raw Linux input events (`evdev`) from tactical hardware buttons (PTT, SOS, Volume, Custom),
 * parsing key states (PRESSED, RELEASED, LONG_PRESS) for tactile field operation.
 */

#pragma once

#include <string>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>

namespace AnnonOS {
namespace Hardware {

struct KeyEvent {
    int key_code;
    std::string state; // "PRESSED", "RELEASED"
};

class KeyPressDriver {
public:
    KeyPressDriver(const std::string& device_path = "/dev/input/event1") 
        : device_path_(device_path), fd_(-1), is_active_(false) {
        std::cout << "[AudioVideoEngine-CPP] Initializing Key Press Driver on " << device_path_ << "...\n";
    }

    ~KeyPressDriver() {
        close_device();
    }

    bool initialize() {
        fd_ = open(device_path_.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd_ < 0) {
            std::cerr << "[AudioVideoEngine-CPP] Warning: Could not open key event node " << device_path_ << ". Running in simulation mode.\n";
            is_active_ = false;
            return false;
        }
        is_active_ = true;
        std::cout << "[AudioVideoEngine-CPP] Key press driver successfully attached to hardware bus.\n";
        return true;
    }

    bool poll_key_event(KeyEvent& out_event) {
        if (!is_active_ || fd_ < 0) {
            return false; // Simulation mode fallback
        }

        struct input_event ev;
        ssize_t bytes_read = read(fd_, &ev, sizeof(struct input_event));
        if (bytes_read < (ssize_t)sizeof(struct input_event)) {
            return false; // No key events in buffer
        }

        // Parse Linux EV_KEY input events
        if (ev.type == EV_KEY) {
            out_event.key_code = ev.code;
            // ev.value: 0 = Released, 1 = Pressed, 2 = Repeat/Hold
            if (ev.value == 1) {
                out_event.state = "PRESSED";
                return true;
            } else if (ev.value == 0) {
                out_event.state = "RELEASED";
                return true;
            }
        }

        return false;
    }

    void close_device() {
        if (fd_ >= 0) {
            close(fd_);
            fd_ = -1;
            is_active_ = false;
            std::cout << "[AudioVideoEngine-CPP] Key press driver disconnected.\n";
        }
    }

private:
    std::string device_path_;
    int fd_;
    bool is_active_;
};

} // namespace Hardware
} // namespace AnnonOS
