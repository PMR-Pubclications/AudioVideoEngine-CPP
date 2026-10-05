#pragma once
#include <vector>
#include <cstdint>
#include <memory>
#include <string>

namespace Trust::FirstResponder {

struct ThermalFrame {
    uint32_t width;
    uint32_t height;
    std::vector<float> data_celsius; // Radiometric matrix in degrees C
};

class FlirBosonCamera {
public:
    FlirBosonCamera(const std::string& device_path = "/dev/video0");
    ~FlirBosonCamera();

    bool initialize();
    bool capture_frame(ThermalFrame& out_frame);
    void close();

private:
    std::string device_path_;
    int fd_ = -1;
    uint32_t width_ = 640;  // Boson 640 standard resolution
    uint32_t height_ = 512;
    bool is_initialized_ = false;

    float raw_to_celsius(uint16_t raw_val) const {
        // High-Gain Radiometric Conversion (0.01K per LSB)
        return (static_cast<float>(raw_val) * 0.01f) - 273.15f;
    }
};

} // namespace Trust::FirstResponder
