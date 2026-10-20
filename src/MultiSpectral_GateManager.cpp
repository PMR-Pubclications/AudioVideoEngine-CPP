#include <Arduino.h>

// --- Discrete MOSFET Gate Driver Pin Mapping ---
const int MOSFET_GATE_1 = 3;  // High-speed gate driver channel 1
const int MOSFET_GATE_2 = 4;  // High-speed gate driver channel 2
const int CURRENT_SENSE = A1; // Analog feedback from a low-side shunt resistor
const int FAULT_PIN     = 8;  // Hardware fault / over-current trip input

// --- Switching Parameters ---
const unsigned long DEAD_TIME_US = 2; // Microseconds of dead-time to prevent shoot-through

void setup() {
    Serial.begin(115200);
    
    pinMode(MOSFET_GATE_1, OUTPUT);
    pinMode(MOSFET_GATE_2, OUTPUT);
    pinMode(FAULT_PIN, INPUT_PULLUP);
    pinMode(CURRENT_SENSE, INPUT);
    
    // Ensure all gates start low (MOSFETs off)
    digitalWrite(MOSFET_GATE_1, LOW);
    digitalWrite(MOSFET_GATE_2, LOW);
    
    Serial.println("Discrete MOSFET Gate Controller Initialized.");
}

// Safe shutdown routine
void emergencyShutdown() {
    digitalWrite(MOSFET_GATE_1, LOW);
    digitalWrite(MOSFET_GATE_2, LOW);
    Serial.println("FAULT: Hardware interlock tripped. Gates disabled.");
}

// Safe complementary switching with dead-time insertion
void switchChannels(bool channel1Active) {
    if (digitalRead(FAULT_PIN) == LOW) {
        emergencyShutdown();
        return;
    }

    if (channel1Active) {
        digitalWrite(MOSFET_GATE_2, LOW);          // Turn off channel 2 first
        delayMicroseconds(DEAD_TIME_US);           // Enforce dead-time
        digitalWrite(MOSFET_GATE_1, HIGH);         // Turn on channel 1
    } else {
        digitalWrite(MOSFET_GATE_1, LOW);          // Turn off channel 1 first
        delayMicroseconds(DEAD_TIME_US);           // Enforce dead-time
        digitalWrite(MOSFET_GATE_2, HIGH);         // Turn on channel 2
    }
}

void loop() {
    // Example switching cycle monitoring current feedback
    int currentRaw = analogRead(CURRENT_SENSE);
    
    if (currentRaw > 900) { // Example raw ADC threshold for over-current protection
        emergencyShutdown();
        return;
    }
    
    // Toggle switching state
    switchChannels(true);
    delayMicroseconds(500);
    
    switchChannels(false);
    delayMicroseconds(500);
}
