#include <Arduino.h>

// --- 10-Pin Interface Mapping for Multi-Spectral Array ---
const int LED_UV_PIN      = 3;    // Source 1: UV Die
const int LED_VIS_PIN     = 4;    // Source 2: Visible Die
const int LED_NIR_PIN     = 5;    // Source 3: Near-IR Die (LiDAR)
const int LED_IR_HEAT_PIN = 6;    // Source 4: Infrared Die (Thermal/Heat)

const int THERMISTOR_PIN  = A0;   // Analog feedback from bronze core
const int LIDAR_SYNC_PIN  = 7;    // Hardware sync input

// --- Thermal & Operational Thresholds ---
// Infrared heating modes generate intense localized heat, so thermal limits are critical
const float WARNING_TEMP_C  = 60.0;   
const float CRITICAL_TEMP_C = 75.0; 

// --- Spectral Operating Modes ---
enum SpectrumMode {
    MODE_UV,
    MODE_VISIBLE,
    MODE_LIDAR_NIR,
    MODE_IR_HEATING,
    MODE_ALL_SPECTRA
};

// --- PWM Configuration ---
const int PWM_FREQ        = 25000;  // 25 kHz switching frequency
const int PWM_RES_BITS    = 8;      // 8-bit resolution (0-255)

void setup() {
    Serial.begin(115200);
    
    pinMode(LED_UV_PIN, OUTPUT);
    pinMode(LED_VIS_PIN, OUTPUT);
    pinMode(LED_NIR_PIN, OUTPUT);
    pinMode(LED_IR_HEAT_PIN, OUTPUT);
    
    pinMode(LIDAR_SYNC_PIN, INPUT_PULLUP);
    pinMode(THERMISTOR_PIN, INPUT);
    
    // Ensure all spectra start completely off
    analogWrite(LED_UV_PIN, 0);
    analogWrite(LED_VIS_PIN, 0);
    analogWrite(LED_NIR_PIN, 0);
    analogWrite(LED_IR_HEAT_PIN, 0);
    
    Serial.println("Multi-Spectral Quad-Source Controller Initialized.");
}

float readBronzeCoreTemperature() {
    int rawADC = analogRead(THERMISTOR_PIN);
    if (rawADC == 0) return 0.0;
    
    float voltage = rawADC * (3.3 / 4095.0); 
    float resistance = (3.3 - voltage) * 10000.0 / voltage; 
    
    float beta = 3950.0;
    float nominalResistance = 10000.0;
    float nominalTempK = 298.15;
    
    float tempK = 1.0 / ((1.0 / nominalTempK) + (log(resistance / nominalResistance) / beta));
    return tempK - 273.15;
}

void allSpectraOff() {
    analogWrite(LED_UV_PIN, 0);
    analogWrite(LED_VIS_PIN, 0);
    analogWrite(LED_NIR_PIN, 0);
    analogWrite(LED_IR_HEAT_PIN, 0);
}

// --- Dynamic Spectrum Dispatcher ---
void executeSpectrum(SpectrumMode mode, uint8_t intensity, unsigned long durationMs) {
    float coreTemp = readBronzeCoreTemperature();
    
    // Safety cutoff if bronze core is saturated
    if (coreTemp >= CRITICAL_TEMP_C) {
        allSpectraOff();
        Serial.println("CRITICAL TEMP: Bronze core saturated. Halting spectrum output.");
        return;
    } 
    
    // Scale back intensity if entering warning zone (especially important for IR heating mode)
    if (coreTemp >= WARNING_TEMP_C) {
        intensity = (uint8_t)(intensity * 0.70); 
        Serial.println("WARNING TEMP: Throttling spectral intensity for thermal safety.");
    }
    
    // Route power to the selected spectral die
    switch (mode) {
        case MODE_UV:
            analogWrite(LED_UV_PIN, intensity);
            Serial.println("Active Spectrum: Ultraviolet (UV)");
            break;
            
        case MODE_VISIBLE:
            analogWrite(LED_VIS_PIN, intensity);
            Serial.println("Active Spectrum: Visible High-Lumen");
            break;
            
        case MODE_LIDAR_NIR:
            analogWrite(LED_NIR_PIN, intensity);
            Serial.println("Active Spectrum: Near-Infrared (LiDAR Pulse)");
            break;
            
        case MODE_IR_HEATING:
            analogWrite(LED_IR_HEAT_PIN, intensity);
            Serial.println("Active Spectrum: Infrared (Thermal Output)");
            break;
            
        case MODE_ALL_SPECTRA:
            analogWrite(LED_UV_PIN, intensity);
            analogWrite(LED_VIS_PIN, intensity);
            analogWrite(LED_NIR_PIN, intensity);
            analogWrite(LED_IR_HEAT_PIN, intensity);
            Serial.println("Active Spectrum: Full-Band Combination");
            break;
    }
    
    // Hold output for specified duration (milliseconds)
    delay(durationMs);
    
    // Shut down after duration expires
    allSpectraOff();
}

void loop() {
    // Example cycle: 
    // 1. Fire Near-IR for LiDAR ranging
    executeSpectrum(MODE_LIDAR_NIR, 240, 10); 
    delay(40);
    
    // 2. Switch to Infrared Thermal Output for localized heating
    executeSpectrum(MODE_IR_HEATING, 200, 500); 
    delay(100);
}
