#include "OTAHandler.hpp"
#include <Arduino.h>

OTAHandler::OTAHandler() {}

void OTAHandler::begin() {
    // Initialize OTA
    ArduinoOTA.begin();

    // Set up OTA event handlers
    ArduinoOTA.onStart(handleOTAStart);
    ArduinoOTA.onProgress(handleOTAProgress);
    ArduinoOTA.onEnd(handleOTAEnd);
    ArduinoOTA.onError(handleOTAError);
}

void OTAHandler::handle() {
    ArduinoOTA.handle();
}

void OTAHandler::handleOTAStart() {
    Serial.println("OTA update starting...");
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    Serial.println("Start updating " + type);
}

void OTAHandler::handleOTAProgress(unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
}

void OTAHandler::handleOTAEnd() {
    Serial.println("\nOTA update complete!");
    ESP.restart();
}

void OTAHandler::handleOTAError(ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
        Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
        Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
        Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
        Serial.println("End Failed");
    }
}
