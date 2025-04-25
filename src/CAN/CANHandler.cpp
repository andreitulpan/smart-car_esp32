#include "CANHandler.hpp"

CANHandler::CANHandler(int csPin) : can(csPin) {}

bool CANHandler::begin() {
    if (can.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
        Serial.println("MCP2515 Initialized Successfully!");
        can.setMode(MCP_NORMAL);
        return true;
    } else {
        Serial.println("Error Initializing MCP2515...");
        return false;
    }
}

void CANHandler::requestAndReceive(byte pid, const String& label) {
    byte request[] = {0x02, 0x01, pid, 0x00, 0x00, 0x00, 0x00, 0x00};
    can.sendMsgBuf(obdRequestId, 0, 8, request);
    delay(100); // Give ECU time to respond

    unsigned long rxId;
    byte len;
    byte rxBuf[8];

    unsigned long startTime = millis();
    while (millis() - startTime < 500) { // Wait for response for 500ms
        if (can.checkReceive() == CAN_MSGAVAIL) {
            can.readMsgBuf(&rxId, &len, rxBuf);

            if (rxId == ecuResponseId && rxBuf[1] == 0x41 && rxBuf[2] == pid) {
                parseOBDResponse(rxBuf, label);
                return;
            }
        }
    }

    Serial.println(label + ": No Response");
}

void CANHandler::parseOBDResponse(byte* rxBuf, const String& label) {
    int value = 0;

    if (label == "RPM") {
        value = ((rxBuf[3] * 256) + rxBuf[4]) / 4;
        Serial.println("RPM: " + String(value));
    } 
    else if (label == "Speed") {
        value = rxBuf[3];
        Serial.println("Speed: " + String(value) + " km/h");
    } 
    else if (label == "Coolant Temp") {
        value = rxBuf[3] - 40;
        Serial.println("Coolant Temp: " + String(value) + " °C");
    } 
    else if (label == "Oil Temp") {
        value = rxBuf[3] - 40;
        Serial.println("Oil Temp: " + String(value) + " °C");
    } 
    else if (label == "MAF") {
        float maf = ((rxBuf[3] * 256) + rxBuf[4]) / 100.0;
        Serial.println("MAF: " + String(maf) + " g/s");
    }
}
