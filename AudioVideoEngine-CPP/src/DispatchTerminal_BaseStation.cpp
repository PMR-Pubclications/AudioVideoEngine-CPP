#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "USB.h"
#include "USBHost.h"

// --- Trust-Main: Dispatch Base Station Terminal ---
// Interfaces physical desktop screens and USB hardware keyboards with the 
// wilderness mesh network for command-and-control operations.

// Incoming telemetry packet structure from field units
typedef struct {
    uint8_t unitID;
    uint32_t sequenceNum;
    uint8_t alertType; // 0 = Normal, 1 = Man-Down, 2 = Status Check-in
    double latitude;
    double longitude;
    float altitude;
    float bearing;
    float distance;
    float confidence;
} FieldTelemetryPacket;

FieldTelemetryPacket lastReceivedPacket;
bool newPacketAvailable = false;

// ESP-NOW Receive Callback (Listens to all active field units)
void onMeshPacketReceived(const uint8_t *macAddr, const uint8_t *incomingData, int len) {
    if (len == sizeof(FieldTelemetryPacket)) {
        memcpy(&lastReceivedPacket, incomingData, sizeof(FieldTelemetryPacket));
        newPacketAvailable = true;
    }
}

// USB Host Keyboard Callback for Dispatcher Input
void keyboardKeyHandler(uint8_t mkey, uint8_t nkey) {
    // Handle physical keypresses from the dispatcher's desktop keyboard
    Serial.print("[DISPATCH KEYBOARD] Key Pressed Code: ");
    Serial.println(nkey);
    
    // Example dispatch commands based on key strokes
    if (nkey == 0x2C) { // Spacebar or specific hotkey
        Serial.println("[DISPATCH COMMAND] Broadcasting grid sector refresh request to all units.");
        // Code to send command packet back out via ESP-NOW mesh
    }
}

void initDispatchHardware() {
    Serial.begin(115200);
    
    // Initialize Wi-Fi in Station mode for ESP-NOW
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("[DISPATCH] ESP-NOW initialization failed.");
        return;
    }
    esp_now_register_recv_cb(esp_now_recv_cb_t(onMeshPacketReceived));

    // Initialize USB Host for Hardware Desktop Keyboard (ESP32-S3 specific feature)
    USBHost::begin();
    // USBHost.onKeyboard(keyboardKeyHandler); // Bind physical keyboard events

    Serial.println("Dispatch Base Station Terminal Initialized.");
}

// Update physical screen display with live telemetry and 3D triangulation grid
void updateDispatchConsoleScreen(const FieldTelemetryPacket &pkt) {
    Serial.println("==========================================");
    Serial.println("[DISPATCH CONSOLE SCREEN LIVE DISPLAY]");
    Serial.print("Active Unit ID: "); Serial.println(pkt.unitID);
    Serial.print("Status Alert:   "); 
    if (pkt.alertType == 1) Serial.println("CRITICAL [MAN-DOWN / DISTRESS]");
    else Serial.println("Normal Operation");
    
    Serial.print("3D Position:    Lat: "); Serial.print(pkt.latitude, 6);
    Serial.print(" | Lon: "); Serial.print(pkt.longitude, 6);
    Serial.print(" | Alt: "); Serial.print(pkt.altitude, 1); Serial.println("m MSL");
    
    Serial.print("Vector Target:  Bearing: "); Serial.print(pkt.bearing);
    Serial.print(" deg | Distance: "); Serial.print(pkt.distance); Serial.println("m");
    Serial.println("==========================================");
}

void setup() {
    initDispatchHardware();
}

void loop() {
    // Process incoming mesh packets in real time
    if (newPacketAvailable) {
        updateDispatchConsoleScreen(lastReceivedPacket);
        newPacketAvailable = false;
    }

    // Handle background dispatch logging and UI refresh cycles
    delay(100);
}
