#ifndef FIREBASE_HANDLER_HPP
#define FIREBASE_HANDLER_HPP

#include <Firebase_ESP_Client.h>
#include <FirebaseJson.h>
#include <map>
#include <Arduino.h>
#include "FirebaseConfig.hpp"

class FirebaseHandler {
public:
    FirebaseHandler(const String& apiKey, const String& userEmail, const String& userPassword, const String& databaseUrl);
    void begin();
    void addData(const String& key, const String& value); // Add key-value pair to the dictionary
    void sendData(unsigned long timestamp);              // Send all data in the dictionary

private:
    FirebaseData fbdo;
    FirebaseAuth auth;
    FirebaseConfig config;
    FirebaseJson json;

    String databasePath;
    unsigned long sendDataPrevMillis = 0;
    const unsigned long timerDelay = 5000; // Fixed interval (5 seconds)

    std::map<String, String> dataMap; // Dictionary to store key-value pairs
};

#endif // FIREBASE_HANDLER_HPP