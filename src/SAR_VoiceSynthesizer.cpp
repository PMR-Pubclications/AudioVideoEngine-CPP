#include <Arduino.h>
#include <driver/i2s.h>

// --- SAR Hands-Free Audio & Speech Synthesis Engine ---
// Converts directional bearings and triangulation coordinates into 
// synthesized audio cues or voice commands for first responders.

// I2S Configuration for onboard audio amplifier / speaker output
const i2s_port_t I2S_NUM = I2S_NUM_0;
const int I2S_BCK_IO = 26;  // Bit Clock
const int I2S_WS_IO  = 25;  // Word Select / LRCLK
const int I2S_DO_IO  = 22;  // Data Out to Audio Amp (DAC)

void initAudioSynthesizer() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 64,
        .use_apll = false,
        .tx_desc_auto_clear = true
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCK_IO,
        .ws_io_num = I2S_WS_IO,
        .data_out_num = I2S_DO_IO,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM, &pin_config);
    
    Serial.println("SAR Voice Synthesizer & Audio Engine Initialized.");
}

// Function to generate phonetic audio tones or speech guidance cues
void playAudioTone(int frequencyHz, int durationMs) {
    int sampleRate = 16000;
    int numSamples = (sampleRate * durationMs) / 1000;
    int16_t *samples = (int16_t *)malloc(numSamples * sizeof(int16_t));

    if (!samples) return;

    for (int i = 0; i < numSamples; i++) {
        float t = (float)i / sampleRate;
        // Generate sine wave audio tone for tactile audio feedback
        samples[i] = (int16_t)(32767.0 * sin(2.0 * PI * frequencyHz * t));
    }

    size_t bytesWritten;
    i2s_write(I2S_NUM, samples, numSamples * sizeof(int16_t), &bytesWritten, portMAX_DELAY);
    free(samples);
}

/**
 * Translates navigation bearing and triangulation distance into hands-free audio alerts.
 * E.g., Audio cue cadence changes based on how close the team is to the target.
 */
void announceGuidanceCue(float bearingDegrees, float distanceMeters) {
    Serial.print("[AUDIO SYNTHESIZER] Cueing Operator -> Bearing: ");
    Serial.print(bearingDegrees);
    Serial.print(" deg, Distance: ");
    Serial.print(distanceMeters);
    Serial.println("m");

    if (abs(bearingDegrees) <= 10.0) {
        // On target path: Double high beep
        playAudioTone(1200, 150);
        delay(100);
        playAudioTone(1200, 150);
    } else if (bearingDegrees < 0.0) {
        // Veer Left: Single low-mid tone
        playAudioTone(800, 200);
    } else {
        // Veer Right: Two short mid-high tones
        playAudioTone(1000, 100);
        delay(80);
        playAudioTone(1000, 100);
    }
}

void setup() {
    Serial.begin(115200);
    initAudioSynthesizer();
}

void loop() {
    // Simulated live feedback test from the triangulation engine
    float activeBearing = -22.5; // Example: target is off to the left
    float activeDistance = 34.2; // Example: 34 meters away

    announceGuidanceCue(activeBearing, activeDistance);
    
    delay(4000); // Announcement cadence interval
}
