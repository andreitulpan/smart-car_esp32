#include <WiFi.h>
#include <time.h>
#include "OTA/OTAHandler.hpp"
#include "BLE/BLEHandler.hpp"
#include "EEPROM/EEPROMHandler.hpp"

#define BOOT_BUTTON_PIN 0 // GPIO pin for the boot button
#define LED_PIN 2         // GPIO pin for the onboard LED

bool isBLEActive = false; // Tracks if BLE is active
bool wifiStatus = false;

// OTAHandler otaHandler;
BLEHandler bleHandler;
EEPROMHandler eepromHandler(EEPROM_SIZE);

void bleReceiveCallback(const std::string& message) {
    if (!message.empty()) {
        Serial.println("Processing command: " + String(message.c_str()));

        if (message == "CLEAR_EEPROM") {
            eepromHandler.clear();
            Serial.println("EEPROM cleared via BLE.");
            bleHandler.sendMessage("EEPROM cleared.");
        } else if (message == "DISABLE_BLE") {
            bleHandler.sendMessage("BLE disabling...");
            bleHandler.stopListening();
            isBLEActive = false;
            digitalWrite(LED_PIN, LOW);
            Serial.println("BLE disabled.");
        } else if (message.rfind("WIFI,", 0) == 0) { // Check if message starts with "WIFI,"
            size_t firstComma = message.find(',');
            size_t secondComma = message.find(',', firstComma + 1);

            if (secondComma != std::string::npos) {
                std::string ssid = message.substr(firstComma + 1, secondComma - firstComma - 1);
                std::string password = message.substr(secondComma + 1);
                
                // Validate SSID and password length
                if (ssid.length() < 1 || ssid.length() > 32 || password.length() < 1 || password.length() > 32) {
                    Serial.println("Invalid SSID or Password length. Must be between 1 and 32 characters.");
                    bleHandler.sendMessage("Invalid SSID or Password length. Must be between 1 and 32 characters.");
                    return; // Exit the function if validation fails
                }

                eepromHandler.saveWiFiCredentials(ssid.c_str(), password.c_str());
                
                WiFi.disconnect();
                WiFi.begin(ssid.c_str(), password.c_str());
                wifiStatus = false; // Reset WiFi status
            } else {
                Serial.println("Invalid WIFI command format.");
                bleHandler.sendMessage("Invalid WIFI command format.");
            }
        } else {
            Serial.println("Invalid command: " + String(message.c_str()));
            bleHandler.sendMessage("Invalid command.");
        }
    }
}

void setup() {
    Serial.begin(115200);

    // Initialize GPIO
    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP); // Boot button
    pinMode(LED_PIN, OUTPUT);               // LED

    eepromHandler.begin(); // Initialize EEPROM

    String ssid, password;
    eepromHandler.loadWiFiCredentials(ssid, password); // Load WiFi credentials from EEPROM
    Serial.println("Loaded SSID: " + ssid);
    Serial.println("Loaded Password: " + password);

    // Connect to WiFi
    WiFi.begin(ssid, password);

    // Initialize OTA
    // otaHandler.begin();

    // Initialize BLE
    bleHandler.begin("SMARTCAR_BLE");
    // bleHandler.startListening();
    bleHandler.setReceiveMessageCallback(bleReceiveCallback);
    bleHandler.setDeviceConnectedCallback([]() {
        Serial.println("Device connected!");
    });
    bleHandler.setDeviceDisconnectedCallback([]() {
        Serial.println("Device disconnected!");
    });
}

void loop() {
    static unsigned long buttonPressStartTime = 0;
    static unsigned long lastLEDToggleTime = 0;
    static bool ledState = false;

    if (WiFi.status() == WL_CONNECTED && !wifiStatus) {
        wifiStatus = true;
        Serial.println("\nConnected to WiFi network " + String(WiFi.SSID()));
        Serial.println("IP Address: " + WiFi.localIP().toString());
        bleHandler.sendMessage(std::string("WiFi connected: ") + WiFi.SSID().c_str() + ", IP: " + WiFi.localIP().toString().c_str());
    }

    // Check if the boot button is pressed
    if (digitalRead(BOOT_BUTTON_PIN) == LOW) {
        if (buttonPressStartTime == 0) {
            buttonPressStartTime = millis(); // Start timing the button press
        } else if (millis() - buttonPressStartTime >= 3000) { // Button held for 3 seconds
            if (!isBLEActive) {
                Serial.println("Starting BLE...");
                bleHandler.startListening();
                isBLEActive = true; // Enable BLE
            }
        }
    } else {
        buttonPressStartTime = 0; // Reset timing if button is released
    }

    // Flicker the LED while BLE is active
    if (isBLEActive) {
        if (millis() - lastLEDToggleTime >= 500) { // Toggle LED every 500ms
            lastLEDToggleTime = millis();
            ledState = !ledState;
            digitalWrite(LED_PIN, ledState ? HIGH : LOW);
        }
    }

    // Handle OTA updates
    // otaHandler.handle();

    // Handle BLE communication
    bleHandler.handle();
}