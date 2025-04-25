#ifndef FIREBASE_HANDLER_HPP
#define FIREBASE_HANDLER_HPP

#include <Firebase_ESP_Client.h>
#include <FirebaseJson.h>
#include <Arduino.h>
#include "FirebaseConfig.hpp"

class FirebaseHandler {
public:
    FirebaseHandler(const String& apiKey, const String& userEmail, const String& userPassword, const String& databaseUrl);
    void begin();
    void sendData(float temperature, float humidity, float pressure, unsigned long timestamp);

private:
    FirebaseData fbdo;
    FirebaseAuth auth;
    FirebaseConfig config;
    FirebaseJson json;

    String uid;
    String databasePath;
    String tempPath = "/temperature";
    String humPath = "/humidity";
    String presPath = "/pressure";
    String timePath = "/timestamp";

    unsigned long sendDataPrevMillis = 0;
    unsigned long timerDelay = 60000;

    void initializeFirebase();
};

#endif // FIREBASE_HANDLER_HPP