#include "Platform.h"
#include "CameraInterface.h"

#if defined(PLATFORM_LINUX) && !defined(PLATFORM_ANDROID)

#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

class LinuxCameraBackend : public CameraBackend {
public:
    std::vector<CameraInfo> findCameras() override {
        std::vector<CameraInfo> cameras;

        for (int i = 0; i < 16; ++i) {
            std::string devicePath = "/dev/video" + std::to_string(i);
            if (!std::filesystem::exists(devicePath)) {
                continue;
            }

            int fd = open(devicePath.c_str(), O_RDWR | O_NONBLOCK, 0);
            if (fd < 0) {
                continue;
            }

            struct v4l2_capability cap;
            if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == 0) {
                if (cap.device_caps & V4L2_CAP_VIDEO_CAPTURE) {
                    CameraInfo info;
                    info.id = devicePath;
                    info.name = reinterpret_cast<char*>(cap.card);
                    info.facing = LensFacing::External;
                    cameras.push_back(info);
                }
            }

            close(fd);
        }

        return cameras;
    }

    bool openCamera(const std::string& cameraId) override {
        std::cout << "[Linux] Initializing V4L2 device: " << cameraId << std::endl;
        return true;
    }
};

std::vector<CameraInfo> findAvailableCameras() {
    LinuxCameraBackend backend;
    return backend.findCameras();
}

#else

std::vector<CameraInfo> findAvailableCameras() {
    return {};
}

#endif
