#include "FirebaseHandler.hpp"
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

FirebaseHandler::FirebaseHandler(const String& apiKey, const String& userEmail, const String& userPassword, const String& databaseUrl) {
    config.api_key = apiKey;
    auth.user.email = userEmail;
    auth.user.password = userPassword;
    config.database_url = databaseUrl;
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
    Serial.println("Getting User UID");
    while ((auth.token.uid) == "") {
        Serial.print('.');
        delay(1000);
    }

    // Store the user UID
    String uid = auth.token.uid.c_str();
    Serial.print("User UID: ");
    Serial.println(auth.token.uid.c_str());

    // Update the database path
    databasePath = "/data/";
    databasePath += uid;
    databasePath += "/readings";
}

void FirebaseHandler::addData(const String& key, const String& value) {
    String formattedKey = "/" + key;
    dataMap[formattedKey] = value; // Add or update the key-value pair in the dictionary
}
void FirebaseHandler::sendData(unsigned long timestamp) {
    if (!dataMap.empty() && Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)) {
        sendDataPrevMillis = millis();

        // Construct the full path with the timestamp
        String fullPath = databasePath;
        fullPath += "/";
        fullPath += String(timestamp); // Add timestamp as part of the path

        // Set all key-value pairs in the JSON object
        for (const auto& entry : dataMap) {
            json.set(entry.first.c_str(), entry.second);
        }

        // Optionally include the timestamp in the data
        json.set("/timestamp", String(timestamp));

        // Send the JSON object to Firebase
        Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, fullPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());

        // Clear the dictionary after sending the data
        dataMap.clear();
    }
}