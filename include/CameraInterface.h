#pragma once

#include <string>
#include <vector>

#include "Platform.h"

enum class LensFacing {
    Front,
    Back,
    External,
    Unknown
};

struct CameraInfo {
    std::string id;
    std::string name;
    LensFacing facing;
};

class CameraBackend {
public:
    virtual ~CameraBackend() = default;
    virtual std::vector<CameraInfo> findCameras() = 0;
    virtual bool openCamera(const std::string& cameraId) = 0;
};

std::vector<CameraInfo> findAvailableCameras();
