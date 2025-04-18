#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WebServer.h>
#include <MPU6050.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// MPU6050 object
MPU6050 mpu;

// WiFi Credentials
const char* ssid = "Dialog 4G 434";
const char* password = "7Fd1B4d1";

WebServer server(80);  // For ESP32

const float gyroThreshold = 1.0;  
const float accelThreshold = 1.3; 
const float fallThreshold = 2.5;  // Adjust this threshold for fall detection
const float alpha = 0.8;  
int stepCount = 0;
bool stepDetected = false;
const int stepsGoal = 10000;  // Define a step goal

// Emergency notification variables
bool fallDetected = false;
unsigned long fallDetectionTime = 0;
const unsigned long fallBlinkInterval = 1000; // Blink interval in milliseconds
const int blinkCount = 5;

// Debounce delay for step detection
const unsigned long debounceDelay = 800;
unsigned long lastStepTime = 0;

float filteredGyroZ = 0.0;  
float temperature = 0.0;  

// NTP Client to get time
WiFiUDP ntpUDP; 
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800, 60000); 

// Heart Rate Sensor Pin
const int heartRatePin = 34;  
int heartRateValue = 0;
int heartRateSum = 0; 
int heartRateCount = 0; 
unsigned long healthCheckStartTime = 0;

// Timing for heart rate readings
unsigned long lastHeartRateTime = 0; 
const unsigned long heartRateInterval = 5000; 
const unsigned long healthCheckInterval = 300000; // 5 minutes in milliseconds

// Button pin
const int buttonPin = 18;  
int screenIndex = 0; 
unsigned long lastButtonPressTime = 0; 
const unsigned long buttonDebounceDelay = 200; 

// Notification center flag
bool notificationCenterActive = false;
unsigned long buttonPressStartTime = 0;
const unsigned long longPressDuration = 1500;  // 1.5 seconds for long press

void handleRoot() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    String response = "{\"stepCount\": " + String(stepCount) + 
                      ", \"temperature\": " + String(temperature, 1) + 
                      ", \"heartRate\": " + String(heartRateValue) + 
                      ", \"time\": \"" + timeClient.getFormattedTime() + 
                      "\", \"fallDetected\": " + String(fallDetected ? "true" : "false") + 
                      ", \"warning\": " + String(fallDetected ? "true" : "false") + "}";
    server.send(200, "application/json", response);
}

void updateOledDisplay() {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);

    if (notificationCenterActive) {
        // Notification Center Display
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.print("5 Min Health Check");
        display.setCursor(0, 10);
        
        // BPM condition display
        if (heartRateValue < 60) {
            display.print("BPM: Bradycardia");
        } else if (heartRateValue <= 100) {
            display.print("BPM: Normal");
        } else if (heartRateValue <= 120) {
            display.print("BPM: Elevated");
        } else if (heartRateValue <= 150) {
            display.print("BPM: High Tachycardia");
        } else {
            display.print("BPM: Very High");
        }

        // Steps converted to kilometers
        float km = stepCount * 0.0008; // Assume 1 step = 0.8 meters
        display.setCursor(0, 25);
        display.print("You walked ");
        display.print(km, 2);
        display.print(" KM");

        // Step goal progress
        display.setCursor(0, 40);
        display.print("Goal: ");
        display.print(stepCount);
        display.print("/");
        display.print(stepsGoal);
    } else {
        // Normal screen based on screenIndex
        if (fallDetected) {
            display.setCursor(0, 0);
            display.setTextSize(1);
            display.print("Fall Detected");
            display.setCursor(0, 30);
            display.setTextSize(2);
            display.print("Dialing..");
        } else {
            // Display regular info based on the current screen index
            if (screenIndex == 1) {
                display.setCursor(0, 0);
                display.print("Steps ");
                display.setCursor(0, 30);
                display.print(stepCount);
            } else if (screenIndex == 2) {
                display.setCursor(0, 0);
                display.print("Temp");
                display.setCursor(0, 30);
                display.print(temperature, 1);
                display.print(" C");
            } else if (screenIndex == 3) {
                display.setCursor(0, 0);
                display.print("HeartRate");
                display.setCursor(0, 30);
                display.print(heartRateValue);
                display.print(" BPM");
            } else if (screenIndex == 0) {
                display.setCursor(0, 0);
                display.print("Time");
                display.setCursor(0, 30);
                display.print(timeClient.getFormattedTime());
            }
        }
    }

    display.display();
}

void setup() {
    Serial.begin(115200);
    Wire.begin(); // Initialize I2C communication for ESP32

    // Initialize MPU6050
    mpu.initialize();
    if (!mpu.testConnection()) {
        Serial.println("MPU6050 connection failed!");
        while (1);
    }

    // Initialize OLED display
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("OLED initialization failed!");
        while (1);
    }
    display.display();
    delay(2000);
    display.clearDisplay();

    // Initialize WiFi
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi ");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Initialize NTP Client
    timeClient.begin();

    // Start the web server
    server.on("/", HTTP_GET, handleRoot);
    server.begin();

    // Setup button pin
    pinMode(buttonPin, INPUT_PULLUP); // Set button pin as input with pull-up resistor

    delay(2000); // Pause for 2 seconds
}

void loop() {
    server.handleClient();
    timeClient.update();  // Update the NTP time

    unsigned long currentMillis = millis();

    // Read gyroscope and accelerometer values from the MPU6050
    int16_t gx, gy, gz, ax, ay, az;
    mpu.getRotation(&gx, &gy, &gz);
    mpu.getAcceleration(&ax, &ay, &az);

    float gyroZ = gz / 131.0;  
    float accelMagnitude = sqrt(ax * ax + ay * ay + az * az) / 16384.0;  

    // Get temperature from the MPU6050
    temperature = (mpu.getTemperature() / 340.00) + 36.53; // Convert to degrees Celsius

    // Update heart rate value every 5 seconds
    if (currentMillis - lastHeartRateTime >= heartRateInterval) {
        lastHeartRateTime = currentMillis; 

        // Read heart rate sensor value
        heartRateValue = analogRead(heartRatePin);
        heartRateValue = map(heartRateValue, 0, 4095, 40, 180); // Adjust according to your sensor

        heartRateSum += heartRateValue;
        heartRateCount++;
    }

    // Handle 5-minute health check
    if (currentMillis - healthCheckStartTime >= healthCheckInterval) {
        heartRateValue = heartRateSum / heartRateCount;  // Calculate average BPM
        heartRateSum = 0; 
        heartRateCount = 0; 
        healthCheckStartTime = currentMillis;  // Reset health check start time
    }

    // Button press handling for long press and cycling through screens
    if (digitalRead(buttonPin) == LOW) {  // Button is pressed
        if (buttonPressStartTime == 0) {  // First press
            buttonPressStartTime = currentMillis;
        } else if (currentMillis - buttonPressStartTime > longPressDuration) {
            notificationCenterActive = true;  // Activate notification center on long press
            updateOledDisplay();
        }
    } else {  // Button released
        if (buttonPressStartTime > 0 && (currentMillis - buttonPressStartTime < longPressDuration)) {
            // Short press: cycle through screens
            screenIndex = (screenIndex + 1) % 4;
        }
        buttonPressStartTime = 0;
        notificationCenterActive = false;  // Deactivate notification center after release
        updateOledDisplay();
    }

    // Only proceed with step detection if there is significant motion
    if (accelMagnitude > accelThreshold) {
        filteredGyroZ = alpha * filteredGyroZ + (1.0 - alpha) * gyroZ;

        if (abs(filteredGyroZ) > gyroThreshold) {
            if (!stepDetected && (currentMillis - lastStepTime) > debounceDelay) {
                stepCount++;
                stepDetected = true;
                lastStepTime = currentMillis;  

                Serial.println("Step detected!");
                Serial.print("Step count: ");
                Serial.println(stepCount);
            }
        } else {
            stepDetected = false;
        }

        // Fall detection logic
        if (accelMagnitude > fallThreshold) {
            fallDetected = true;
            fallDetectionTime = currentMillis; // Record time of fall detection
            Serial.println("Fall detected!");
            updateOledDisplay();
        }
    } else {
        stepDetected = false;  
    }

    // Blink warning on the web interface if a fall is detected
    if (fallDetected) {
        if (currentMillis - fallDetectionTime <= blinkCount * fallBlinkInterval) {
            server.send(200, "application/json", "{\"fallDetected\": true}"); // Notify web
        } else {
            fallDetected = false; // Reset fall detection after blink duration
            server.send(200, "application/json", "{\"fallDetected\": false}"); // Clear warning
        }
    }

    // Update OLED display if not in notification center mode
    if (!notificationCenterActive && millis() - lastButtonPressTime > 200) {
        updateOledDisplay();
    }
}
