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

void FirebaseHandler::sendData(float temperature, float humidity, float pressure, unsigned long timestamp) {
    if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)) {
        sendDataPrevMillis = millis();

        String parentPath = databasePath;
        parentPath += "/";
        parentPath += String(timestamp);

        json.set(tempPath.c_str(), String(temperature));
        json.set(humPath.c_str(), String(humidity));
        json.set(presPath.c_str(), String(pressure));
        json.set(timePath, String(timestamp));

        Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
    }
}
