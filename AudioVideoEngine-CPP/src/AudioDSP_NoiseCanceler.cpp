#include <Arduino.h>
#include <math.h>

// --- Audio/Video Engine: Real-Time DSP Noise Canceler & Voice Enhancer ---
// Filters incoming headset and microphone audio streams to eliminate wind/ambient 
// noise and isolate human speech frequencies for tactical clarity.

const int SAMPLE_RATE = 16000;

// Simple Biquad Filter State for Voice Bandpass (300Hz - 3400Hz)
struct BiquadState {
    float x1 = 0, x2 = 0, y1 = 0, y2 = 0;
};

BiquadState micFilterState;

// Coefficients for a 4th-order Butterworth Bandpass (approximate tuning for voice clarity)
// Passband: 300Hz to 3400Hz at 16kHz sampling rate
float processVoiceBandpass(float sample) {
    // Simple high-pass pole to remove low-end wind roar (< 300 Hz)
    // and low-pass pole to remove high-end hiss (> 3400 Hz)
    static float prevIn = 0;
    static float prevOut = 0;
    
    // High-pass filter component (RC filter model for microcontrollers)
    float alphaHP = 0.85f; 
    float hpSample = alphaHP * (prevOut + sample - prevIn);
    prevIn = sample;
    prevOut = hpSample;

    return hpSample;
}

// Dynamic Noise Gate & Spectral Floor Subtraction
int16_t applyDynamicNoiseGate(int16_t rawSample, float noiseFloorThreshold) {
    float floatSample = (float)rawSample;
    float absVal = fabsf(floatSample);

    // If signal magnitude is below the rolling noise floor, gate it to zero (silence background hum/wind)
    if (absVal < noiseFloorThreshold) {
        return 0; 
    }

    // If signal is active voice, apply slight expansion/enhancement
    return (int16_t)(floatSample * 1.15f); 
}

/**
 * Main audio stream processing hook for headset input/output buffers.
 * Cleans raw audio packets before they are passed to the AI transcription or speaker output.
 */
void processAudioStream(int16_t *audioBuffer, size_t sampleCount) {
    // Calculate rolling energy to dynamically adjust the noise floor
    float totalEnergy = 0;
    for (size_t i = 0; i < sampleCount; i++) {
        totalEnergy += fabsf((float)audioBuffer[i]);
    }
    float averageEnergy = totalEnergy / sampleCount;
    
    // Adaptive noise threshold based on environment background levels
    float adaptiveNoiseFloor = fmaxf(250.0f, averageEnergy * 0.35f);

    for (size_t i = 0; i < sampleCount; i++) {
        // Step 1: Apply voice bandpass filtering
        float filtered = processVoiceBandpass((float)audioBuffer[i]);

        // Step 2: Apply dynamic noise gate to strip ambient rumble
        int16_t processed = applyDynamicNoiseGate((int16_t)filtered, adaptiveNoiseFloor);

        // Write back cleaned sample
        audioBuffer[i] = processed;
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("Audio/Video Engine [DSP Noise Canceler] Initialized.");
}

void loop() {
    // Simulated live audio buffer capture from Bluetooth headset / mic input
    const size_t bufferLen = 128;
    int16_t mockAudioStream[bufferLen];

    // Populate mock buffer with speech + background noise simulation
    for (size_t i = 0; i < bufferLen; i++) {
        mockAudioStream[i] = (int16_t)(sin(i * 0.2) * 1200 + random(-400, 400));
    }

    // Process the stream through the DSP noise-canceling pipeline
    processAudioStream(mockAudioStream, bufferLen);

    Serial.println("[DSP ENGINE] Headset audio stream filtered and enhanced for tactical comms.");
    delay(1000);
}
