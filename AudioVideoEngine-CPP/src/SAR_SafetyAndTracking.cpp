#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

// --- Trust-Main: Responder Safety, Man-Down & Breadcrumb Tracking ---
// Handles automated IMU fall/immobility detection, emergency mesh beacons, 
// and secure breadcrumb drop routines for wilderness navigation.

const uint8_t UNIT_ID = 1;
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Breadcrumb structure for offline route history
struct Breadcrumb {
    double latitude;
    double longitude;
    float altitude;
    unsigned long timestamp;
};

// Emergency Telemetry Structure
typedef struct {
    uint8_t unitID;
    uint8_t alertType; // 0 = Normal, 1 = Man-Down / Distress, 2 = Status Check-in
    double latitude;
    double longitude;
    float altitude;
    unsigned long timestamp;
} SafetyPacket;

SafetyPacket localSafetyReport;

// State tracking for Man-Down detection
unsigned long lastMovementTimestamp = 0;
const unsigned long IMMOBILITY_TIMEOUT_MS = 30000; // 30 seconds without movement triggers alert
bool manDownTriggered = false;

// Breadcrumb buffer configuration
const int MAX_BREADCRUMBS = 100;
Breadcrumb breadcrumbTrail[MAX_BREADCRUMBS];
int breadcrumbIndex = 0;
unsigned long lastBreadcrumbTime = 0;
const unsigned long BREADCRUMB_INTERVAL_MS = 10000; // Drop point every 10 seconds

void initSafetyAndTracking() {
    Serial.println("Safety, Man-Down & Breadcrumb Tracking Initialized.");
    lastMovementTimestamp = millis();
}

// Drop a spatial breadcrumb into local memory for return-path tracking
void recordBreadcrumb(double lat, double lon, float alt) {
    if (breadcrumbIndex < MAX_BREADCRUMBS) {
        breadcrumbTrail[breadcrumbIndex] = {lat, lon, alt, millis()};
        Serial.print("[BREADCRUMB] Dropped point #");
        Serial.print(breadcrumbIndex);
        Serial.print(" at Lat: ");
        Serial.print(lat, 6);
        Serial.print(" | Lon: ");
        Serial.println(lon, 6);
        breadcrumbIndex++;
    } else {
        // Implement circular buffer overwrite if trail exceeds max capacity
        for (int i = 1; i < MAX_BREADCRUMBS; i++) {
            breadcrumbTrail[i - 1] = breadcrumbTrail[i];
        }
        breadcrumbTrail[MAX_BREADCRUMBS - 1] = {lat, lon, alt, millis()};
        Serial.println("[BREADCRUMB] Trail buffer cycled. Oldest point overwritten.");
    }
}

// Broadcast emergency distress beacon across the mesh network
void broadcastManDownAlert(double lat, double lon, float alt) {
    localSafetyReport.unitID = UNIT_ID;
    localSafetyReport.alertType = 1; // Distress / Man-Down
    localSafetyReport.latitude = lat;
    localSafetyReport.longitude = lon;
    localSafetyReport.altitude = alt;
    localSafetyReport.timestamp = millis();

    esp_now_send(broadcastAddress, (uint8_t *)&localSafetyReport, sizeof(localSafetyReport));
    Serial.println("[CRITICAL ALERT] MAN-DOWN DISTRESS BEACON BROADCASTED ACROSS MESH!");
}

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW initialization failed.");
        return;
    }

    initSafetyAndTracking();
}

void loop() {
    unsigned long currentMillis = millis();

    // Simulated live GPS/IMU telemetry from the hardware bus
    double currentLat = 45.6387;
    double currentLon = -122.6614;
    double currentAlt = 120.5;
    bool imuMotionDetected = true; // Set to false if accelerometer detects zero movement

    // 1. Man-Down / Immobility Monitoring Loop
    if (imuMotionDetected) {
        lastMovementTimestamp = currentMillis;
        if (manDownTriggered) {
            manDownTriggered = false;
            Serial.println("[SAFETY] Motion re-detected. Man-down alert cleared.");
        }
    } else {
        if (!manDownTriggered && (currentMillis - lastMovementTimestamp > IMMOBILITY_TIMEOUT_MS)) {
            manDownTriggered = true;
            broadcastManDownAlert(currentLat, currentLon, currentAlt);
        }
    }

    // 2. Automated Breadcrumb Tracking Loop
    if (currentMillis - lastBreadcrumbTime > BREADCRUMB_INTERVAL_MS) {
        recordBreadcrumb(currentLat, currentLon, currentAlt);
        lastBreadcrumbTime = currentMillis;
    }

    delay(1000);
}
