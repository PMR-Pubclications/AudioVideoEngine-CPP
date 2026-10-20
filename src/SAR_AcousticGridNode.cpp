#include <WiFi.h>
#include <esp_now.h>

// --- Hardware Pin Mapping ---
const int MIC_LEFT_PIN     = A2;  
const int MIC_RIGHT_PIN    = A3;  
const int LED_SOURCE_1     = 3;   // Left directional indicator
const int LED_SOURCE_2     = 4;   // Forward / Center indicator
const int LED_SOURCE_3     = 5;   // Right directional indicator

// --- SAR Node Configuration ---
const uint8_t UNIT_ID      = 1;   // Unique identifier for this handheld unit
float currentLatitude      = 45.6387; // Placeholder for live GPS integration
float currentLongitude     = -122.6614;

// --- Mesh Packet Structure for SAR ---
typedef struct {
    uint8_t unitID;
    float latitude;
    float longitude;
    float bearingDegrees;
    float signalConfidence; // Amplitude/Volume metric to weight accuracy
    unsigned long timestamp;
} SAR_Packet;

SAR_Packet localReport;
SAR_Packet peerReport;
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len) {
    memcpy(&peerReport, incomingData, sizeof(peerReport));
    
    // Process incoming peer data for team coordination
    Serial.print("SAR Peer [Unit ");
    Serial.print(peerReport.unitID);
    Serial.print("] Bearing: ");
    Serial.print(peerReport.bearingDegrees);
    Serial.println(" deg.");
}

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    
    if (esp_now_init() != ESP_OK) {
        Serial.println("Mesh initialization failed.");
        return;
    }
    
    esp_now_register_recv_cb(OnDataRecv);
    
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
    
    pinMode(LED_SOURCE_1, OUTPUT);
    pinMode(LED_SOURCE_2, OUTPUT);
    pinMode(LED_SOURCE_3, OUTPUT);
    
    Serial.println("SAR Grid Node Initialized.");
}

// Drive the LED array as a physical compass pointing toward the target
void updateVisualGuidance(float targetBearing) {
    // Reset indicators
    digitalWrite(LED_SOURCE_1, LOW);
    digitalWrite(LED_SOURCE_2, LOW);
    digitalWrite(LED_SOURCE_3, LOW);
    
    if (targetBearing < -15.0) {
        digitalWrite(LED_SOURCE_1, HIGH); // Steer Left
        Serial.println("Guidance: VEER LEFT");
    } else if (targetBearing > 15.0) {
        digitalWrite(LED_SOURCE_3, HIGH); // Steer Right
        Serial.println("Guidance: VEER RIGHT");
    } else {
        digitalWrite(LED_SOURCE_2, HIGH); // On Target / Straight Ahead
        Serial.println("Guidance: HEAD STRAIGHT");
    }
}

void broadcastSARData(float bearing, float confidence) {
    localReport.unitID = UNIT_ID;
    localReport.latitude = currentLatitude;
    localReport.longitude = currentLongitude;
    localReport.bearingDegrees = bearing;
    localReport.signalConfidence = confidence;
    localReport.timestamp = millis();
    
    esp_now_send(broadcastAddress, (uint8_t *)&localReport, sizeof(localReport));
}

void loop() {
    // Simulated live acoustic calculation
    float detectedBearing = 22.5; 
    float signalAmplitude = 0.85; // High confidence hit
    
    // Update physical LED indicators for the operator
    updateVisualGuidance(detectedBearing);
    
    // Share telemetry across the search grid network
    broadcastSARData(detectedBearing, signalAmplitude);
    
    delay(2000); // Grid sync interval
}
