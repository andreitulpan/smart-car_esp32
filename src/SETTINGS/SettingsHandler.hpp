#ifndef SETTINGS_HANDLER_HPP
#define SETTINGS_HANDLER_HPP

#include <Arduino.h>

class SettingsHandler {
public:
    static constexpr int DEFAULT_CAN_REQUEST_INTERVAL = 5000;
    static constexpr int DEFAULT_CAN_RESPONSE_THRESHOLD = 20000;
    static constexpr bool DEFAULT_ENABLE_LOGS = false;

    // Getters
    static int getCanRequestInterval();
    static int getCanResponseThreshold();
    static bool getEnableLogs();

    // Setters
    static void setCanRequestInterval(int value);
    static void setCanResponseThreshold(int value);
    static void setEnableLogs(bool value);

    // Persistence
    static void load();
    static void save();
    static void reset();

private:
    static int canRequestInterval;
    static int canResponseThreshold;
    static bool enableLogs;
};

#endif // SETTINGS_HANDLER_HPP
