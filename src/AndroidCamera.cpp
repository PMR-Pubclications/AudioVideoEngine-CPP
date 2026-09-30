#if defined(PLATFORM_ANDROID)

#include "CameraInterface.h"

#include <android/log.h>

#define LOG_TAG "AudioVideoEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

std::vector<CameraInfo> findAvailableCameras() {
    LOGI("Android camera discovery is enabled through a toolchain-specific implementation.");
    return {};
}

#endif
