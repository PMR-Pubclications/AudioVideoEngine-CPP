#include "CameraInterface.h"
#include "LMSAdaptiveFilter.h"
#include "Platform.h"

#include <iostream>
#include <vector>

int main() {
    std::cout << "AudioVideoEngine-CPP standalone runtime" << std::endl;
    std::cout << "Platform: " << OS_NAME << std::endl;

    LMSAdaptiveFilter filter(64, 0.01);

    std::vector<double> primary = {1.0, 0.8, 0.4, 0.2, 0.1, 0.05, 0.0, -0.05};
    std::vector<double> reference = {0.9, 0.7, 0.3, 0.15, 0.05, 0.02, 0.0, -0.02};
    std::vector<double> output;

    filter.processBlock(primary, reference, output);

    std::cout << "Processed samples:" << std::endl;
    for (size_t i = 0; i < output.size(); ++i) {
        std::cout << "  sample[" << i << "] = " << output[i] << std::endl;
    }

#if defined(PLATFORM_LINUX) && !defined(PLATFORM_ANDROID)
    auto cameras = findAvailableCameras();
    std::cout << "Detected cameras: " << cameras.size() << std::endl;
    for (const auto& camera : cameras) {
        std::cout << " - " << camera.name << " [" << camera.id << "]" << std::endl;
    }
#endif

    return 0;
}
