#include "LMSAdaptiveFilter.h"

#include <algorithm>
#include <cmath>
#include <vector>

LMSAdaptiveFilter::LMSAdaptiveFilter(size_t taps, double learningRate)
    : filterTaps_(taps),
      mu_(learningRate),
      weights_(taps, 0.0),
      buffer_(taps, 0.0) {}

double LMSAdaptiveFilter::processSample(double primaryMic, double referenceMic) {
    for (size_t i = filterTaps_ - 1; i > 0; --i) {
        buffer_[i] = buffer_[i - 1];
    }
    buffer_[0] = referenceMic;

    double estimatedNoise = 0.0;
    for (size_t i = 0; i < filterTaps_; ++i) {
        estimatedNoise += weights_[i] * buffer_[i];
    }

    double cleanOutput = primaryMic - estimatedNoise;

    for (size_t i = 0; i < filterTaps_; ++i) {
        weights_[i] += 2.0 * mu_ * cleanOutput * buffer_[i];
        weights_[i] = std::clamp(weights_[i], -2.0, 2.0);
    }

    return cleanOutput;
}

void LMSAdaptiveFilter::processBlock(const std::vector<double>& primary,
                                    const std::vector<double>& reference,
                                    std::vector<double>& output) {
    output.resize(primary.size());
    for (size_t i = 0; i < primary.size(); ++i) {
        output[i] = processSample(primary[i], reference[i]);
    }
}
