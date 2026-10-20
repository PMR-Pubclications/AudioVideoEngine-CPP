#include <Arduino.h>
#include <math.h>

// --- Triangulation Engine for SAR Grid ---
// Calculates the intersection point (Latitude/Longitude) of two separate 
// directional bearing vectors reported by peer handheld units.

struct GeoCoordinate {
    float latitude;
    float longitude;
};

// Convert degrees to radians
float toRadians(float degrees) {
    return degrees * (PI / 180.0);
}

// Convert radians to degrees
float toDegrees(float radians) {
    return radians * (180.0 / PI);
}

/**
 * Calculates the intersection point of two lines defined by a starting point (lat/lon) 
 * and a bearing angle using a local planar approximation (ideal for local mesh ranges).
 */
bool calculateVectorIntersection(
    float lat1, float lon1, float bearing1Deg,
    float lat2, float lon2, float bearing2Deg,
    GeoCoordinate &result
) {
    // Convert bearings to math angles (0 degrees = East, counter-clockwise or standard azimuth conversion)
    // Standard navigation bearing: 0 = North, 90 = East. Convert to standard cartesian angle (radians):
    float theta1 = toRadians(90.0 - bearing1Deg);
    float theta2 = toRadians(90.0 - bearing2Deg);

    // Slope m of the lines
    float m1 = tan(theta1);
    float m2 = tan(theta2);

    // Check if lines are parallel
    if (fabs(m1 - m2) < 0.0001) {
        return false; // Parallel lines, no intersection
    }

    // Using local meter approximation around the midpoint or Unit 1 position
    // 1 degree latitude ~= 111,000 meters
    // 1 degree longitude ~= 111,000 * cos(latitude) meters
    float latScale = 111000.0;
    float lonScale = 111000.0 * cos(toRadians((lat1 + lat2) / 2.0));

    // Convert lat/lon points to local X, Y meters relative to Unit 1
    float x1 = 0.0;
    float y1 = 0.0;
    float x2 = (lon2 - lon1) * lonScale;
    float y2 = (lat2 - lat1) * latScale;

    // Line 1: y - y1 = m1 * (x - x1)  =>  y = m1*x + c1
    float c1 = y1 - m1 * x1;
    // Line 2: y - y2 = m2 * (x - x2)  =>  y = m2*x + c2
    float c2 = y2 - m2 * x2;

    // Solve for intersection X and Y in meters
    float x_intersect = (c2 - c1) / (m1 - m2);
    float y_intersect = m1 * x_intersect + c1;

    // Convert intersection meters back to global Latitude and Longitude
    result.latitude = lat1 + (y_intersect / latScale);
    result.longitude = lon1 + (x_intersect / lonScale);

    return true;
}

void setup() {
    Serial.begin(115200);
    Serial.println("SAR Triangulation Engine Initialized.");

    // Example Test Case: Two units detecting a sound source from different grid positions
    // Unit 1 Position & Bearing
    float u1_lat = 45.6387;
    float u1_lon = -122.6614;
    float u1_bearing = 45.0; // Pointing northeast-ish

    // Unit 2 Position & Bearing (located slightly east)
    float u2_lat = 45.6390;
    float u2_lon = -122.6590;
    float u2_bearing = 315.0; // Pointing northwest-ish

    GeoCoordinate targetLocation;

    if (calculateVectorIntersection(u1_lat, u1_lon, u1_bearing, u2_lat, u2_lon, u2_bearing, targetLocation)) {
        Serial.println("Target Triangulated Successfully!");
        Serial.print("Estimated Target Latitude: ");
        Serial.println(targetLocation.latitude, 6);
        Serial.print("Estimated Target Longitude: ");
        Serial.println(targetLocation.longitude, 6);
    } else {
        Serial.println("Triangulation failed: Vectors are parallel or invalid.");
    }
}

void loop() {
    // Real-time mesh triangulation processing runs here
    delay(1000);
}
