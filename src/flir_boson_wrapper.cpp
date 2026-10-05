#include "flir_boson_wrapper.hpp"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

namespace Trust::FirstResponder {

FlirBosonCamera::FlirBosonCamera(const std::string& device_path)
    : device_path_(device_path) {}

FlirBosonCamera::~FlirBosonCamera() {
    close();
}

bool FlirBosonCamera::initialize() {
    fd_ = open(device_path_.c_str(), O_RDWR | O_NONBLOCK, 0);
    if (fd_ < 0) {
        std::cerr << "[FLIR_BOSON] Failed to open V4L2 device: " << device_path_ << std::endl;
        return false;
    }

    v4l2_format fmt{};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = width_;
    fmt.fmt.pix.height = height_;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_Y16; // 16-bit uncompressed radiometric stream
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    if (ioctl(fd_, VIDIOC_S_FMT, &fmt) < 0) {
        std::cerr << "[FLIR_BOSON] Failed to set Y16 radiometric format." << std::endl;
        close();
        return false;
    }

    is_initialized_ = true;
    return true;
}

bool FlirBosonCamera::capture_frame(ThermalFrame& out_frame) {
    if (!is_initialized_) return false;

    size_t frame_pixels = width_ * height_;
    std::vector<uint16_t> raw_buffer(frame_pixels);

    // Read 16-bit raw frame buffer from camera stream
    ssize_t bytes_read = read(fd_, raw_buffer.data(), frame_pixels * sizeof(uint16_t));
    if (bytes_read < static_cast<ssize_t>(frame_pixels * sizeof(uint16_t))) {
        return false; // Frame not ready or dropped
    }

    out_frame.width = width_;
    out_frame.height = height_;
    out_frame.data_celsius.resize(frame_pixels);

    // Convert raw radiometric counts to float array of Celsius temperatures
    for (size_t i = 0; i < frame_pixels; ++i) {
        out_frame.data_celsius[i] = raw_to_celsius(raw_buffer[i]);
    }

    return true;
}

void FlirBosonCamera::close() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
    is_initialized_ = false;
}

} // namespace Trust::FirstResponder
