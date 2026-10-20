#include <Arduino.h>
#include <driver/i2s.h>

// --- Audio/Video Engine: Hands-Free Voice Note & Audio Capture ---
// Manages digital microphone (I2S RX) streaming, voice activity buffering, 
// and structured field-note packetization for the Trust-Main AI pipeline.

// --- I2S Microphone (RX) Pin Mapping ---
const i2s_port_t I2S_MIC_PORT = I2S_NUM_0;
const int I2S_MIC_SCK       = 26;  // Serial Clock / Bit Clock (BCK)
const int I2S_MIC_WS        = 25;  // Word Select / LRCLK
const int I2S_MIC_SD        = 33;  // Serial Data In (DIN from microphone like INMP441)

// Audio Buffer Configuration
const int SAMPLE_RATE       = 16000; // 16kHz sampling rate optimized for voice recognition
const int BUFFER_SIZE       = 512;
int32_t rawAudioBuffer[BUFFER_SIZE];

struct VoiceFieldNote {
    uint8_t unitID;
    unsigned long timestamp;
    uint32_t audioDataLength;
    bool noteComplete;
};

// Initialize I2S Master Receive for digital microphone array
void initAudioInputEngine() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT, // Standard for high-res digital mics
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 256,
        .use_apll = false,
        .tx_desc_auto_clear = false
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_MIC_SCK,
        .ws_io_num = I2S_MIC_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_MIC_SD
    };

    i2s_driver_install(I2S_MIC_PORT, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_MIC_PORT, &pin_config);
    i2s_zero_dma_buffer(I2S_MIC_PORT);

    Serial.println("Audio/Video Engine [Voice Capture] Initialized.");
}

/**
 * Captures microphone input, checks for voice activity, and packages 
 * the stream into a structured payload for AI database transcription.
 */
bool captureAndProcessVoiceNote(VoiceFieldNote &outNote) {
    size_t bytesRead = 0;
    
    // Read raw audio samples from I2S DMA buffer
    i2s_read(I2S_MIC_PORT, (void*)&rawAudioBuffer, sizeof(rawAudioBuffer), &bytesRead, portMAX_DELAY);
    
    if (bytesRead > 0) {
        int sampleCount = bytesRead / sizeof(int32_t);
        long energySum = 0;

        // Simple RMS / Voice Activity Detection (VAD) filter to ignore wind/ambient background noise
        for (int i = 0; i < sampleCount; i++) {
            int32_t sample = rawAudioBuffer[i] >> 11; // Scale down 32-bit to manageable range
            energySum += abs(sample);
        }
        
        long averageEnergy = energySum / sampleCount;
        
        // Threshold check: Ignore quiet background, trigger when operator speaks
        const long VOICE_THRESHOLD = 1500; 
        
        if (averageEnergy > VOICE_THRESHOLD) {
            outNote.unitID = 1;
            outNote.timestamp = millis();
            outNote.audioDataLength = bytesRead;
            outNote.noteComplete = true;

            Serial.print("[AV ENGINE] Voice Activity Detected! Energy: ");
            Serial.print(averageEnergy);
            Serial.println(" | Stream buffered for AI parsing.");
            return true;
        }
    }

    outNote.noteComplete = false;
    return false;
}

void setup() {
    Serial.begin(115200);
    initAudioInputEngine();
}

void loop() {
    VoiceFieldNote activeNote;

    // Continuously monitor microphone input for hands-free field notes
    if (captureAndProcessVoiceNote(activeNote)) {
        // Hand off activeNote buffer pointer to the Annon AI speech parser / local database
        Serial.println("[AV ENGINE] Handing audio payload to Trust-Main AI pipeline...");
    }

    delay(50); // High-frequency polling for responsive voice activation
}
