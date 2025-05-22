#include "LogHandler.hpp"

#include "../SETTINGS/SettingsHandler.hpp"

std::queue<LogHandler::LogEntry> LogHandler::logQueue;

unsigned long LogHandler::getTime() {
    time_t now;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo) || timeinfo.tm_year < (2020 - 1900)) {
        // Time not set or invalid
        return 0;
    }
    time(&now);
    return now;
}

void LogHandler::writeMessage(LogHandler::DebugType type, const String& message, bool sendToFirebase) {
    String typeStr;
    switch (type) {
        case DebugType::INFO: typeStr = "INFO"; break;
        case DebugType::WARNING: typeStr = "WARNING"; break;
        case DebugType::ERROR: typeStr = "ERROR"; break;
        case DebugType::BLE: typeStr = "BLE"; break;
        case DebugType::CAN: typeStr = "CAN"; break;
        default: typeStr = "INFO"; break;
    }
    if (sendToFirebase && SettingsHandler::getEnableLogs()) {
        logQueue.push({typeStr, String(getTime()), message});
    }
    // Print timestamp in human-readable format
    char timeStr[32] = {0};
    time_t now = getTime();
    if (now != 0) {
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
        Serial.println("[" + typeStr + "] [" + String(timeStr) + "] " + message);
    } else {
        Serial.println("[" + typeStr + "] [NO TIME] " + message);
    }
}

std::vector<LogHandler::LogEntry> LogHandler::getAndClearLogs() {
    std::vector<LogEntry> logs;
    while (!logQueue.empty()) {
        logs.push_back(logQueue.front());
        logQueue.pop();
    }
    return logs;
}
