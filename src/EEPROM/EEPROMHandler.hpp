#ifndef EEPROMHANDLER_HPP
#define EEPROMHANDLER_HPP

#include <EEPROM.h>
#include <Arduino.h>

const int SSID_ADDRESS = 0;
const int SSID_SIZE = 32; // Max length of SSID
const int PASSWORD_ADDRESS = SSID_ADDRESS + 33; // 32 bytes for SSID + 1 byte for null terminator
const int PASSWORD_SIZE = 32; // Max length of password
const int EEPROM_SIZE = (32 + 1) * 2; // Total EEPROM size

class EEPROMHandler {
public:
    EEPROMHandler(size_t size);
    void begin();
    void saveString(int address, const String& data);
    String loadString(int address, size_t length);
    void clear();

    void saveWiFiCredentials(const String& ssid, const String& password);
    void loadWiFiCredentials(String& ssid, String& password);
private:
    size_t _size;
};

#endif // EEPROMHANDLER_HPP