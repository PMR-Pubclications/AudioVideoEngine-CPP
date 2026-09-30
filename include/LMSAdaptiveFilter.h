#pragma once

#include <vector>

class LMSAdaptiveFilter {
public:
    LMSAdaptiveFilter(size_t taps = 64, double learningRate = 0.01);

    double processSample(double primaryMic, double referenceMic);

    void processBlock(const std::vector<double>& primary,
                      const std::vector<double>& reference,
                      std::vector<double>& output);

private:
    size_t filterTaps_;
    double mu_;
    std::vector<double> weights_;
    std::vector<double> buffer_;
};
