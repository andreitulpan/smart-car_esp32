#ifndef OTA_HANDLER_H
#define OTA_HANDLER_H

#include <ArduinoOTA.h>

class OTAHandler {
public:
    OTAHandler();
    void begin();
    void handle();

private:
    static void handleOTAStart();
    static void handleOTAProgress(unsigned int progress, unsigned int total);
    static void handleOTAEnd();
    static void handleOTAError(ota_error_t error);
};

#endif // OTA_HANDLER_H
