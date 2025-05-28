#include "FirebaseHandler.hpp"
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include "../LOG/LogHandler.hpp"

FirebaseHandler* FirebaseHandler::instance = nullptr;

FirebaseHandler::FirebaseHandler(const String& apiKey, const String& userEmail, const String& userPassword, const String& databaseUrl, std::map<byte, PIDConfig>& pidMapRef) : pidMap(pidMapRef) {
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

    // Fetch CAN PIDs
    pidPath = userPath + "/config/sensors";
    // Firebase.RTDB.beginStream(&stream2, pidPath.c_str());
    // Firebase.RTDB.setStreamCallback(&stream2, streamCallback2, streamTimeoutCallback2);

    firebaseConfigured = true;
}

void FirebaseHandler::streamCallback(FirebaseStream data) {
    if (instance && instance->firebaseConfigured) {
        if (data.dataTypeEnum() == fb_esp_rtdb_data_type_json) {
            FirebaseJson* json = data.jsonObjectPtr();
            FirebaseJsonData result;
            if (json->get(result, "CAN_REQUEST_INTERVAL") && result.typeNum == FirebaseJson::JSON_INT) {
                SettingsHandler::setCanRequestInterval(result.intValue);
            }
            if (json->get(result, "CAN_RESPONSE_THRESHOLD") && result.typeNum == FirebaseJson::JSON_INT) {
               SettingsHandler::setCanResponseThreshold(result.intValue);
            }
            if (json->get(result, "ENABLE_LOGS") && result.typeNum == FirebaseJson::JSON_BOOL) {
                SettingsHandler::setEnableLogs(result.boolValue);
            }
            LogHandler::writeMessage(LogHandler::DebugType::INFO, "CAN_REQUEST_INTERVAL: " + String(SettingsHandler::getCanRequestInterval()) + ", CAN_RESPONSE_THRESHOLD: " + String(SettingsHandler::getCanResponseThreshold()) + ", ENABLE_LOGS: " + String(SettingsHandler::getEnableLogs()));
        } else if (data.dataPath() == "/CAN_REQUEST_INTERVAL") {
            SettingsHandler::setCanRequestInterval(data.intData());
            LogHandler::writeMessage(LogHandler::DebugType::INFO, "CAN_REQUEST_INTERVAL updated: " + String(SettingsHandler::getCanRequestInterval()));
        } else if (data.dataPath() == "/CAN_RESPONSE_THRESHOLD") {
            SettingsHandler::setCanResponseThreshold(data.intData());
            LogHandler::writeMessage(LogHandler::DebugType::INFO, "CAN_RESPONSE_THRESHOLD updated: " + String(SettingsHandler::getCanResponseThreshold()));
        } else if (data.dataPath() == "/ENABLE_LOGS") {
            SettingsHandler::setEnableLogs(data.boolData());
            LogHandler::writeMessage(LogHandler::DebugType::INFO, "ENABLE_LOGS updated: " + String(SettingsHandler::getEnableLogs()));
        } else {
            LogHandler::writeMessage(LogHandler::DebugType::INFO, "Stream data received: " + data.dataPath() + " -> " + data.stringData());
        }
    }
}

void FirebaseHandler::streamCallback2(FirebaseStream data)
{
    if (instance && instance->firebaseConfigured)
    {
        // instance->fetchCANPIDs();
    }
}

void FirebaseHandler::streamTimeoutCallback(bool timeout) {
    if (timeout)
        LogHandler::writeMessage(LogHandler::DebugType::INFO, "Stream timeout, resuming...");
}

void FirebaseHandler::streamTimeoutCallback2(bool timeout) {
    if (timeout)
        LogHandler::writeMessage(LogHandler::DebugType::INFO, "Stream timeout, resuming...");
}

void FirebaseHandler::addData(const String& key, const String& value) {
    String formattedKey = "/" + key;
    dataMap[formattedKey] = value;
}

void FirebaseHandler::addData(std::vector<CANResponse>& results) {
    for (const auto& response : results) {
        String formattedKey = "/" + response.PID;
        dataMap[formattedKey] = response.Value;
    }
}

void FirebaseHandler::readData() {
    Firebase.RTDB.readStream(&stream);
    // Firebase.RTDB.readStream(&stream2);
}

bool FirebaseHandler::setJSONWithRetry(FirebaseData* fbdo, const String& path, FirebaseJson* json, int maxRetries, int delayMs) {
    int attempt = 0;
    bool success = false;
    while (attempt < maxRetries && !success) {
        success = Firebase.RTDB.setJSON(fbdo, path.c_str(), json);
        if (!success) {
            String msg = "Set json failed (attempt " + String(attempt + 1) + "): " + fbdo->errorReason();
            LogHandler::writeMessage(LogHandler::DebugType::INFO, msg);
            delay(delayMs);
        }
        attempt++;
    }
    return success;
}

bool FirebaseHandler::sendData(bool dataWasReceived, unsigned long timestamp) {
    if (dataWasReceived && !dataMap.empty() && Firebase.ready() && (millis() - sendDataPrevMillis > SettingsHandler::getCanRequestInterval() || sendDataPrevMillis == 0)) {
        sendDataPrevMillis = millis();

        // Construct the full path with the timestamp
        String fullPath = sensorsPath;
        fullPath += "/";
        fullPath += String(timestamp); // Add timestamp as part of the path

        // Set all key-value pairs in the JSON object
        for (auto it = dataMap.begin(); it != dataMap.end(); ) {
            json.set(it->first.c_str(), it->second);
            LogHandler::writeMessage(LogHandler::DebugType::INFO, "Adding data to JSON: " + it->first + " -> " + it->second, false);
            it = dataMap.erase(it);
        }

        // Optionally include the timestamp in the data
        json.set("/timestamp", String(timestamp));

        // Send the JSON object to Firebase
        setJSONWithRetry(&fbdo, fullPath, &json, 3, 100);

        return true;
    }
    return false;
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

bool FirebaseHandler::fetchCANPIDs() {
    if (!firebaseConfigured) return false;

    if (Firebase.RTDB.getJSON(&fbdo, pidPath.c_str())) {
        String raw = fbdo.payload().c_str();
        // Serial.println("RAW JSON FROM FIREBASE:");
        // Serial.println(raw);

        FirebaseJsonArray arr;
        arr.setJsonArrayData(raw);

        pidMap.clear();
        FirebaseJsonData result;
        int maxSensors = 20;
        int len = arr.size();
        if (len > maxSensors) len = maxSensors;

        for (int i = 0; i < len; ++i) {
            FirebaseJson jsonObj;
            arr.get(result, i);
            if (!result.success) continue;
            jsonObj.setJsonData(result.stringValue);

            // Only add if enabled
            if (!jsonObj.get(result, "enabled") || !result.boolValue) continue;

            // Get PID as string, then convert to byte
            if (!jsonObj.get(result, "pid")) continue;
            String pidStr = result.stringValue;
            byte pid = (byte)strtol(pidStr.c_str(), nullptr, 0); // Handles "0x.." or decimal

            // Get label/id
            String label = "";
            if (jsonObj.get(result, "id")) label = result.stringValue;

            // Get formula
            String formula = "";
            if (jsonObj.get(result, "formula")) formula = result.stringValue;

            // Get unit
            String unit = "";
            if (jsonObj.get(result, "unit")) unit = result.stringValue;

            pidMap[pid] = PIDConfig{label, formula, unit};
        }

        LogHandler::writeMessage(LogHandler::DebugType::INFO, "Fetched " + String(pidMap.size()) + " active CAN PIDs from Firebase config.");
        return true;
    } else {
        LogHandler::writeMessage(LogHandler::DebugType::ERROR, "Failed to fetch CAN PID config from Firebase: " + fbdo.errorReason());
    }
    return false;
}
