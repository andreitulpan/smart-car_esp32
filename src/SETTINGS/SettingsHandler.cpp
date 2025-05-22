#include "SettingsHandler.hpp"

// Define static member variables
int SettingsHandler::canRequestInterval = SettingsHandler::DEFAULT_CAN_REQUEST_INTERVAL;
int SettingsHandler::canResponseThreshold = SettingsHandler::DEFAULT_CAN_RESPONSE_THRESHOLD;
bool SettingsHandler::enableLogs = SettingsHandler::DEFAULT_ENABLE_LOGS;

int SettingsHandler::getCanRequestInterval() {
    return canRequestInterval;
}

int SettingsHandler::getCanResponseThreshold() {
    return canResponseThreshold;
}

bool SettingsHandler::getEnableLogs() {
    return enableLogs;
}

void SettingsHandler::setCanRequestInterval(int value) {
    canRequestInterval = value;
}

void SettingsHandler::setCanResponseThreshold(int value) {
    canResponseThreshold = value;
}

void SettingsHandler::setEnableLogs(bool value) {
    enableLogs = value;
}

void SettingsHandler::load() {
    // TODO: Implement loading from EEPROM, file, etc.
    // For now, just use defaults
    reset();
}

void SettingsHandler::save() {
    // TODO: Implement saving to EEPROM, file, etc.
}

void SettingsHandler::reset() {
    canRequestInterval = DEFAULT_CAN_REQUEST_INTERVAL;
    canResponseThreshold = DEFAULT_CAN_RESPONSE_THRESHOLD;
}