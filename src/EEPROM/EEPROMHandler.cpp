#include "EEPROMHandler.hpp"
#include "../LOG/LogHandler.hpp"

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
    LogHandler::writeMessage(LogHandler::DebugType::INFO, String("Saving WiFi credentials to EEPROM: ") + ssid + ", " + password);
}

void EEPROMHandler::loadWiFiCredentials(String& ssid, String& password) {
    ssid = loadString(SSID_ADDRESS, SSID_SIZE); // Load SSID (max 32 characters)
    password = loadString(PASSWORD_ADDRESS, PASSWORD_SIZE); // Load password (max 32 characters)
    if (ssid.length() > 0 && password.length() > 0) {
        LogHandler::writeMessage(LogHandler::DebugType::INFO, String("Loaded WiFi credentials from EEPROM: ") + ssid + ", " + password);
    } else {
        LogHandler::writeMessage(LogHandler::DebugType::INFO, String("No WiFi credentials found in EEPROM."));
    }
}
