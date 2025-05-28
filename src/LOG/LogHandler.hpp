#ifndef DEBUG_HANDLER_HPP
#define DEBUG_HANDLER_HPP

#include <Arduino.h>
#include <queue>
#include <vector>

class LogHandler {
public:
    enum class DebugType {
        INFO,
        BLE,
        WARNING,
        CAN,
        ERROR
    };

    struct LogEntry {
        String path;
        long timestamp;
        String message;
    };

    static unsigned long getTime();
    static void writeMessage(LogHandler::DebugType type, const String& message, bool sendToFirebase = true);
    static void sendLogMessage(const String& path, const String& message);
    static std::vector<LogHandler::LogEntry> getAndClearLogs();
    static std::queue<LogHandler::LogEntry> logQueue;
};

#endif // DEBUG_HANDLER_HPP
