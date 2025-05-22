#include <WiFi.h>
#include <time.h>
#include "OTA/OTAHandler.hpp"
#include "BLE/BLEHandler.hpp"
#include "EEPROM/EEPROMHandler.hpp"
#include "Firebase/FirebaseHandler.hpp"
#include "CAN/CANHandler.hpp"
#include "LOG/LogHandler.hpp"
#include "SETTINGS/SettingsHandler.hpp"
#include "UTILS/CANResponse.hpp"

#define BOOT_BUTTON_PIN 0 // GPIO pin for the boot button
#define LED_PIN 2         // GPIO pin for the onboard LED

#define CAN_CS 5 // Chip Select pin for MCP2515

const char* ntpServer = "pool.ntp.org"; // NTP server for time synchronization

bool isBLEActive = false; // Tracks if BLE is active
bool wifiStatus = false;
bool firebaseStatus = false;
bool canActive = false;
static unsigned long lastCanTryToActive = 0;

static bool receivedDataOverCan = false;

// Firebase handler instance
FirebaseHandler firebaseHandler(API_KEY, USER_EMAIL, USER_PASSWORD, DATABASE_URL);

// CAN Handler
CANHandler canHandler(CAN_CS);

// OTAHandler otaHandler;
BLEHandler bleHandler;
EEPROMHandler eepromHandler(EEPROM_SIZE);

std::vector<CANResponse> canResponses;

void bleReceiveCallback(const std::string& message) {
    if (!message.empty()) {
        LogHandler::writeMessage(LogHandler::LogHandler::DebugType::BLE, String("Received command: ") + String(message.c_str()));

        if (message == "CLEAR_EEPROM") {
            eepromHandler.clear();
            LogHandler::writeMessage(LogHandler::DebugType::BLE, String("EEPROM cleared."));
            bleHandler.sendMessage("EEPROM cleared.");
        } else if (message == "DISABLE_BLE") {
            LogHandler::writeMessage(LogHandler::DebugType::BLE, String("BLE disabled."));
            bleHandler.stopListening();
            isBLEActive = false;
            digitalWrite(LED_PIN, LOW);
        } else if (message.rfind("WIFI,", 0) == 0) { // Check if message starts with "WIFI,"
            size_t firstComma = message.find(',');
            size_t secondComma = message.find(',', firstComma + 1);

            if (secondComma != std::string::npos) {
                std::string ssid = message.substr(firstComma + 1, secondComma - firstComma - 1);
                std::string password = message.substr(secondComma + 1);
                
                // Validate SSID and password length
                if (ssid.length() < 1 || ssid.length() > 32 || password.length() < 1 || password.length() > 32) {
                    LogHandler::writeMessage(LogHandler::DebugType::INFO, String("Invalid SSID or Password length. Must be between 1 and 32 characters."));
                    bleHandler.sendMessage("Invalid SSID or Password length. Must be between 1 and 32 characters.");
                    return; // Exit the function if validation fails
                }

                eepromHandler.saveWiFiCredentials(ssid.c_str(), password.c_str());
                
                WiFi.disconnect();
                WiFi.begin(ssid.c_str(), password.c_str());
                wifiStatus = false; // Reset WiFi status
            } else {
                LogHandler::writeMessage(LogHandler::DebugType::INFO, String("Invalid WIFI command format."));
                bleHandler.sendMessage("Invalid WIFI command format.");
            }
        } else {
            LogHandler::writeMessage(LogHandler::DebugType::INFO, String("Invalid command: ") + String(message.c_str()));
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
    eepromHandler.loadWiFiCredentials(ssid, password);

    // Connect to WiFi
    WiFi.begin(ssid, password);

    // Initialize OTA
    // otaHandler.begin();

    canActive = canHandler.begin(); // Initialize CAN handler

    // Add PIDs to the CAN handler
    canHandler.addPID(0x0C, "RPM");
    canHandler.addPID(0x0D, "Speed");
    canHandler.addPID(0x10, "MAF");
    canHandler.addPID(0x05, "Coolant_Temp");
    canHandler.addPID(0x5C, "Oil_Temp");

    // Initialize BLE
    bleHandler.begin("SMARTCAR_BLE");
    // bleHandler.startListening();
    bleHandler.setReceiveMessageCallback(bleReceiveCallback);
    bleHandler.setDeviceConnectedCallback([]() {
        LogHandler::writeMessage(LogHandler::DebugType::BLE, String("Device connected!"));
    });
    bleHandler.setDeviceDisconnectedCallback([]() {
        LogHandler::writeMessage(LogHandler::DebugType::BLE, String("Device disconnected!"));
    });
}

void loop() {
    static unsigned long buttonPressStartTime = 0;
    static unsigned long lastLEDToggleTime = 0;
    static unsigned long lastCANRequestTime = 0;
    static bool ledState = false;

    if (WiFi.status() == WL_CONNECTED) {
        if (!wifiStatus) {
            wifiStatus = true;
            LogHandler::writeMessage(LogHandler::DebugType::INFO, String("WiFi connected: ") + WiFi.SSID() + ", IP: " + WiFi.localIP().toString());
            bleHandler.sendMessage(std::string("WiFi connected: ") + WiFi.SSID().c_str() + ", IP: " + WiFi.localIP().toString().c_str());
            
            // Configure time
            configTime(0, 0, ntpServer);
            struct tm timeinfo;
            while (!getLocalTime(&timeinfo) || timeinfo.tm_year < (2020 - 1900)) {
                LogHandler::writeMessage(LogHandler::DebugType::INFO, "Waiting for NTP time sync...", false);
                delay(1000);
            }
            LogHandler::writeMessage(LogHandler::DebugType::INFO, "NTP time set! Year: " + String(timeinfo.tm_year + 1900), false);
            LogHandler::writeMessage(LogHandler::DebugType::INFO, "Epoch time: " + String(LogHandler::getTime()), false);

        }

        if (!firebaseStatus) {
            firebaseHandler.begin();
            firebaseStatus = true;
        }
    }

    if (!canActive) {
        if (millis() - lastCanTryToActive >= 5000) { // Try to activate CAN every 5 seconds
            canHandler.begin();
            lastCanTryToActive = millis();
        }
    }

    // Check if the boot button is pressed
    if (digitalRead(BOOT_BUTTON_PIN) == LOW) {
        if (buttonPressStartTime == 0) {
            buttonPressStartTime = millis(); // Start timing the button press
        } else if (millis() - buttonPressStartTime >= 3000) { // Button held for 3 seconds
            if (!isBLEActive) {
                LogHandler::writeMessage(LogHandler::DebugType::BLE, String("Starting BLE..."));
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

    // Send CAN requests every X seconds
    canHandler.sendRequests();

    static unsigned long lastCANReadTime = 0;
    const unsigned long canReadInterval = 50; // ms

    if (millis() - lastCANReadTime >= canReadInterval) {
        lastCANReadTime = millis();
        if (canHandler.handleResponses(canResponses)) {
            firebaseHandler.addData(canResponses);
        }
    }

    // Send firebase messages
    receivedDataOverCan = !(firebaseHandler.sendData(receivedDataOverCan, LogHandler::getTime()));

    // Receive firebase messages
    firebaseHandler.readData();

    // Send queued log messages
    firebaseHandler.sendQueuedLogMessages();
}
