#include "FirebaseHandler.hpp"
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include "../LOG/LogHandler.hpp"

FirebaseHandler* FirebaseHandler::instance = nullptr;

FirebaseHandler::FirebaseHandler(const String& apiKey, const String& userEmail, const String& userPassword, const String& databaseUrl) {
    config.api_key = apiKey;
    auth.user.email = userEmail;
    auth.user.password = userPassword;
    config.database_url = databaseUrl;
    instance = this;
}

void FirebaseHandler::begin() {
    Firebase.reconnectWiFi(true);
    fbdo.setResponseSize(4096);

    // Assign the callback function for the long-running token generation task
    config.token_status_callback = tokenStatusCallback;

    // Assign the maximum retry of token generation
    config.max_token_generation_retry = 5;

    // Initialize the library with the Firebase auth and config
    Firebase.begin(&config, &auth);

    // Wait for the user UID
    LogHandler::writeMessage(LogHandler::DebugType::INFO, "Waiting for user UID...", false);
    while ((auth.token.uid) == "") {
        LogHandler::writeMessage(LogHandler::DebugType::INFO, "...", false);
        delay(1000);
    }

    // Store the user UID
    String uid = auth.token.uid.c_str();
    LogHandler::writeMessage(LogHandler::DebugType::INFO, "User UID: " + uid);

    // Set paths
    String userPath = "/data/" + uid;
    sensorsPath = userPath + "/readings";
    logsPath = userPath + "/logs";

    // Update the reading path
    String readingPath = userPath + "/outputs";
    Firebase.RTDB.beginStream(&stream, readingPath.c_str());
    Firebase.RTDB.setStreamCallback(&stream, streamCallback, streamTimeoutCallback);
    firebaseConfigured = true;
}

void FirebaseHandler::streamCallback(FirebaseStream data) {
    if (instance) {
        if (data.dataTypeEnum() == fb_esp_rtdb_data_type_json) {
            FirebaseJson* json = data.jsonObjectPtr();
            FirebaseJsonData result;
            if (json->get(result, "CAN_REQUEST_INTERVAL") && result.typeNum == FirebaseJson::JSON_INT) {
                SettingsHandler::setCanRequestInterval(result.intValue);
            }
            if (json->get(result, "CAN_RESPONSE_THRESHOLD") && result.typeNum == FirebaseJson::JSON_INT) {
               SettingsHandler::setCanResponseThreshold(result.intValue);
            }
            LogHandler::writeMessage(LogHandler::DebugType::INFO, "CAN_REQUEST_INTERVAL: " + String(SettingsHandler::getCanRequestInterval()) + ", CAN_RESPONSE_THRESHOLD: " + String(SettingsHandler::getCanResponseThreshold()));
        } else if (data.dataPath() == "/CAN_REQUEST_INTERVAL") {
            SettingsHandler::setCanRequestInterval(data.intData());
            LogHandler::writeMessage(LogHandler::DebugType::INFO, "CAN_REQUEST_INTERVAL updated: " + String(SettingsHandler::getCanRequestInterval()));
        } else if (data.dataPath() == "/CAN_RESPONSE_THRESHOLD") {
            SettingsHandler::setCanResponseThreshold(data.intData());
            LogHandler::writeMessage(LogHandler::DebugType::INFO, "CAN_RESPONSE_THRESHOLD updated: " + String(SettingsHandler::getCanResponseThreshold()));
        }
    }
}

void FirebaseHandler::streamTimeoutCallback(bool timeout) {
    if (timeout)
        LogHandler::writeMessage(LogHandler::DebugType::INFO, "Stream timeout, resuming...");
}

void FirebaseHandler::addData(const String& key, const String& value) {
    String formattedKey = "/" + key;
    dataMap[formattedKey] = value;
}

void FirebaseHandler::readData() {
    Firebase.RTDB.readStream(&stream);
}

void FirebaseHandler::sendData(unsigned long timestamp) {
    if (!dataMap.empty() && Firebase.ready() && (millis() - sendDataPrevMillis > SettingsHandler::getCanRequestInterval() || sendDataPrevMillis == 0)) {
        sendDataPrevMillis = millis();

        // Construct the full path with the timestamp
        String fullPath = sensorsPath;
        fullPath += "/";
        fullPath += String(timestamp); // Add timestamp as part of the path

        // Set all key-value pairs in the JSON object
        for (auto it = dataMap.begin(); it != dataMap.end(); ) {
            json.set(it->first.c_str(), it->second);
            it = dataMap.erase(it); // erase returns the next iterator
        }

        // Optionally include the timestamp in the data
        json.set("/timestamp", String(timestamp));

        // Send the JSON object to Firebase
        LogHandler::writeMessage(LogHandler::DebugType::INFO, "Set json... " + String(Firebase.RTDB.setJSON(&fbdo, fullPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str()), false);

        // Clear the dictionary after sending the data
        // dataMap.clear();
    }
}

void FirebaseHandler::sendQueuedLogMessages() {
    if (firebaseConfigured) {
        static unsigned long lastSendTime = 0;
        unsigned long now = millis();
        if (now - lastSendTime < 10000) { // 10 seconds
            return;
        }
        lastSendTime = now;
        std::vector<LogHandler::LogEntry> logEntries = LogHandler::getAndClearLogs();
        for (auto entry : logEntries) { // Not const, makes a copy
            if (entry.timestamp == "0") {
                entry.timestamp = String(LogHandler::getTime() - 15); // Remove 15 for the messages when the internet wasn't available
            }
            String queuedFullPath = logsPath + "/" + entry.path + "/" + String(entry.timestamp);
            FirebaseJson json;
            json.set("timestamp", String(entry.timestamp));
            json.set("message", entry.message);
            Firebase.RTDB.setJSON(&fbdo, queuedFullPath.c_str(), &json);
            LogHandler::writeMessage(LogHandler::DebugType::INFO, "Queued log message sent to Firebase: " + entry.message, false);
        }
    }
}