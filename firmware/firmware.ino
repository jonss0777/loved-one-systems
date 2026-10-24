#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <secrets.h>

// Replace with your Wi-Fi credentials
const char* ssid = SSID;
const char* password = PASSWORD;

// Replace with your Raspberry Pi's IP address and WebSocket server port
const char* pi_ip = "172.20.10.2"; 
const uint16_t pi_port = 81;

// WebSocket Client instance
WebSocketsClient webSocket;

// Pin definitions
const int buttonPin = 4;      // Single Button Signal Pin

// Dynamic State Variables controlled by Server & Button
int currentSensorType = 1;    // 1: Temp/Ultrasonic (States: 0, 1, 2), 2: CO2 (States: 0, 1)
int buttonValue = 0;          // Current value being cycled

// Edge detection variable for debouncing
bool lastButtonState = LOW;

// WebSocket Event Handler (Receives commands from server)
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.println("[WSc] Disconnected!");
            break;
        case WStype_CONNECTED:
            Serial.printf("[WSc] Connected to server at url: %s\n", payload);
            
            // Send a valid JSON payload on connection instead of plain text
            {
                String initJson = "{\"status\": \"connected\", \"sensor_type\": " + String(currentSensorType) + "}";
                webSocket.sendTXT(initJson);
            }
            break;
        case WStype_TEXT:
            {
                Serial.printf("[WSc] Message from server: %s\n", payload);
                
                // Parse incoming JSON command from server (e.g., {"set_sensor": 2})
                StaticJsonDocument<200> doc;
                DeserializationError error = deserializeJson(doc, payload);
                
                if (!error) {
                    if (doc.containsKey("set_sensor")) {
                        currentSensorType = doc["set_sensor"];
                        buttonValue = 0; // Reset value when mode changes
                        Serial.printf("[Mode Change] Server updated Sensor Type to: %d\n", currentSensorType);
                    }
                } else {
                    Serial.println("Failed to parse incoming server JSON command.");
                }
            }
            break;
        default:
            break;
    }
}

// Task running on Core 1 (WebSocket Client Loop + Single Button Monitor)
void core1Task(void *pvParameters) {
    webSocket.begin(pi_ip, pi_port, "/");
    webSocket.onEvent(webSocketEvent);
    webSocket.setReconnectInterval(5000);
    
    Serial.println("Core 1 Task started: WebSocket Client & Single Button Monitor active.");

    while(true) {
        // 1. Maintain WebSocket client loop
        webSocket.loop();

        // 2. Read single button pin (Active HIGH)
        bool reading = digitalRead(buttonPin);

        // --- Single Button Logic with Dynamic Behavior ---
        if (reading == HIGH && lastButtonState == LOW) {
            vTaskDelay(pdMS_TO_TICKS(40)); // Simple software debounce
            if (digitalRead(buttonPin) == HIGH) {
                
                // Change cycling limits depending on what sensor type the server assigned
                if (currentSensorType == 1 || currentSensorType == 3) {
                    // 3 states: 0, 1, 2 (Temperature or Ultrasonic)
                    buttonValue = (buttonValue + 1) % 3;
                } else if (currentSensorType == 2) {
                    // 2 states: 0, 1 (CO2 Sensor)
                    buttonValue = (buttonValue + 1) % 2;
                }

                Serial.printf("Button Pressed -> Sensor Type: %d | Value: %d\n", currentSensorType, buttonValue);

                // Send structured JSON payload back to server
                String jsonPayload = "{\"sensor_type\": " + String(currentSensorType) + ", \"value\": " + String(buttonValue) + "}";
                webSocket.sendTXT(jsonPayload);
            }
        }
        lastButtonState = reading;

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void setup() {
    Serial.begin(115200);

    // Set GPIO 21 as an output and pull it HIGH to power the button
    pinMode(powerPin, OUTPUT);
    digitalWrite(powerPin, HIGH);

    // Initialize button signal pin as input
    pinMode(buttonPin, INPUT);

    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected!");
    Serial.print("ESP32 IP Address: ");
    Serial.println(WiFi.localIP());

    // Create FreeRTOS task pinned strictly to Core 1
    xTaskCreatePinnedToCore(
        core1Task,       /* Task function */
        "Core1_Worker",  /* Task name */
        10000,           /* Stack size (bytes) */
        NULL,            /* Parameters */
        1,               /* Priority */
        NULL,            /* Task handle */
        1                /* Core ID (1 = Core 1) */
    );
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}