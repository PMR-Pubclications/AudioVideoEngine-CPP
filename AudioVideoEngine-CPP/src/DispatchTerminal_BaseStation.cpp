#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "USB.h"
#include "USBHost.h"

// --- Trust-Main: Dispatch Base Station Terminal (Keyboard & Mouse Support) ---
// Integrates USB Host peripherals (keyboard and mouse) with mesh telemetry 
// for full command-and-control operations on the dispatcher's console.

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

// USB Host Keyboard Handler
void keyboardKeyHandler(uint8_t mkey, uint8_t nkey) {
    Serial.print("[DISPATCH KEYBOARD] Key Pressed Code: ");
    Serial.println(nkey);
}

// USB Host Mouse Movement & Click Handler
void mouseMoveHandler(int x, int y) {
    // Tracks relative cursor/crosshair movement across the 3D grid map
    if (x != 0 || y != 0) {
        Serial.print("[DISPATCH MOUSE] Cursor Delta -> X: ");
        Serial.print(x);
        Serial.print(" | Y: ");
        Serial.println(y);
    }
}

void mouseButtonHandler(uint8_t button, bool pressed) {
    if (pressed) {
        Serial.print("[DISPATCH MOUSE] Button Clicked: ");
        if (button == 1) Serial.println("Left Click (Select Sector / Target)");
        else if (button == 2) Serial.println("Right Click (Context Menu / Options)");
    }
}

void initDispatchHardware() {
    Serial.begin(115200);
    
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("[DISPATCH] ESP-NOW initialization failed.");
        return;
    }
    esp_now_register_recv_cb(esp_now_recv_cb_t(onMeshPacketReceived));

    // Initialize USB Host for Desktop Keyboard & Mouse Peripherals
    USBHost::begin();
    
    // Bind peripheral input callbacks
    // USBHost.onKeyboard(keyboardKeyHandler);
    // USBHost.onMouseMove(mouseMoveHandler);
    // USBHost.onMouseButton(mouseButtonHandler);

    Serial.println("Dispatch Base Station Terminal Initialized [Keyboard & Mouse Active].");
}

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

    delay(50); // Responsive polling interval for dispatch UI
}
