#ifndef FIREBASE_HANDLER_HPP
#define FIREBASE_HANDLER_HPP

#include <Firebase_ESP_Client.h>
#include <FirebaseJson.h>
#include <map>
#include <Arduino.h>
#include <queue>
#include "FirebaseConfig.hpp"
#include "../SETTINGS/SettingsHandler.hpp"

struct LogEntry {
    String path;
    String message;
};

class FirebaseHandler {
public:
    FirebaseHandler(const String& apiKey, const String& userEmail, const String& userPassword, const String& databaseUrl);
    void begin();
    void addData(const String& key, const String& value); // Add key-value pair to the dictionary
    void sendData(unsigned long timestamp);              // Send all data in the dictionary
    static void streamCallback(FirebaseStream data);
    static void streamTimeoutCallback(bool timeout);
    void readData();
    void sendQueuedLogMessages();

private:
    static FirebaseHandler* instance;
    FirebaseData fbdo;
    FirebaseAuth auth;
    FirebaseConfig config;
    FirebaseJson json;
    FirebaseData stream;

    String sensorsPath;
    String logsPath;
    std::queue<LogEntry> logQueue;
    bool firebaseConfigured = false;

    unsigned long sendDataPrevMillis = 0;
    const unsigned long timerDelay = 5000; // Fixed interval (5 seconds)

    std::map<String, String> dataMap; // Dictionary to store key-value pairs
};

#endif // FIREBASE_HANDLER_HPP