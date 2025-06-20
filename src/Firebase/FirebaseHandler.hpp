#ifndef FIREBASE_HANDLER_HPP
#define FIREBASE_HANDLER_HPP

#include <Firebase_ESP_Client.h>
#include <FirebaseJson.h>
#include <map>
#include <Arduino.h>
#include <queue>
#include <vector>

#include "FirebaseConfig.hpp"
#include "../SETTINGS/SettingsHandler.hpp"
#include "../LOG/LogHandler.hpp"
#include "../UTILS/CANResponse.hpp"
#include "../UTILS/PIDConfig.hpp"

struct LogEntry {
    String path;
    String message;
};

class FirebaseHandler {
public:
    FirebaseHandler(const String& apiKey, const String& userEmail, const String& userPassword, const String& databaseUrl, std::map<byte, PIDConfig>& pidMapRef);
    void begin();
    void addData(const String& key, const String& value); // Add key-value pair to the dictionary
    void addData(std::vector<CANResponse>& results);
    bool sendData(bool dataWasReceived, unsigned long timestamp);              // Send all data in the dictionary
    static void streamCallback(FirebaseStream data);
    static void streamCallback2(FirebaseStream data);
    static void streamTimeoutCallback(bool timeout);
    static void streamTimeoutCallback2(bool timeout);
    void readData();
    void sendQueuedLogMessages();
    bool setJSONWithRetry(FirebaseData* fbdo, const String& path, FirebaseJson* json, int maxRetries, int delayMs);
    bool fetchCANPIDs();
    bool firebaseConfigured = false;

private:
    static FirebaseHandler* instance;
    FirebaseData fbdo;
    FirebaseAuth auth;
    FirebaseConfig config;
    FirebaseJson json;
    FirebaseData stream;
    FirebaseData stream2;

    String sensorsPath;
    String logsPath;
    String pidPath;
    std::queue<LogEntry> logQueue;

    std::map<byte, PIDConfig>& pidMap;

    unsigned long sendDataPrevMillis = 0;
    const unsigned long timerDelay = 5000; // Fixed interval (5 seconds)

    std::map<String, String> dataMap; // Dictionary to store key-value pairs
};

#endif // FIREBASE_HANDLER_HPP