#include "EEPROMHandler.hpp"

EEPROMHandler::EEPROMHandler(size_t size) : _size(size) {}

void EEPROMHandler::begin() {
    EEPROM.begin(_size);
}

void EEPROMHandler::saveString(int address, const String& data) {
    for (size_t i = 0; i < data.length(); i++) {
        EEPROM.write(address + i, data[i]);
    }
    EEPROM.write(address + data.length(), '\0'); // Null-terminate the string
    EEPROM.commit();
}

String EEPROMHandler::loadString(int address, size_t length) {
    char buffer[length + 1];
    for (size_t i = 0; i < length; i++) {
        buffer[i] = EEPROM.read(address + i);
    }
    buffer[length] = '\0'; // Null-terminate the string
    return String(buffer);
}

void EEPROMHandler::clear() {
    for (size_t i = 0; i < _size; i++) {
        EEPROM.write(i, 0);
    }
    EEPROM.commit();
}

void EEPROMHandler::saveWiFiCredentials(const String& ssid, const String& password) {
    saveString(SSID_ADDRESS, ssid);
    saveString(PASSWORD_ADDRESS, password);
    Serial.println("WiFi credentials saved to EEPROM.");
}

void EEPROMHandler::loadWiFiCredentials(String& ssid, String& password) {
    ssid = loadString(SSID_ADDRESS, SSID_SIZE); // Load SSID (max 32 characters)
    password = loadString(PASSWORD_ADDRESS, PASSWORD_SIZE); // Load password (max 32 characters)
    Serial.println("WiFi credentials loaded from EEPROM.");
}
