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

void CANHandler::addPID(byte pid, const String& label) {
    pidMap[pid] = label;
    lastRequestTime[pid] = 0; // Initialize the last request time for the PID
    lastResponseTime[pid] = 0; // Initialize the last response time for the PID
    responseReceived[pid] = true; // Assume a response has been received initially
}

void CANHandler::sendRequests() {
    unsigned long currentTime = millis();

    for (const auto& entry : pidMap) {
        byte pid = entry.first;

        // Check if it's time to send a request for this PID
        if (currentTime - lastRequestTime[pid] >= requestInterval && responseReceived[pid]) {
            byte request[] = {0x02, 0x01, pid, 0x00, 0x00, 0x00, 0x00, 0x00};
            if (can.sendMsgBuf(obdRequestId, 0, 8, request) == CAN_OK) {
                Serial.println("Request sent for PID: " + String(pid, HEX) + " (" + pidMap[pid] + ")");
                lastRequestTime[pid] = currentTime; // Update the last request time
                responseReceived[pid] = false; // Mark as waiting for a response
            } else {
                Serial.println("Error sending request for PID: " + String(pid, HEX));
            }
        }

        // Check if the response threshold has been exceeded
        if (!responseReceived[pid] && currentTime - lastResponseTime[pid] >= responseThreshold) {
            Serial.println("No response received for PID: " + String(pid, HEX) + " (" + pidMap[pid] + ") within threshold time.");
            responseReceived[pid] = true; // Allow sending the request again
        }
    }
}

std::tuple<byte, byte*> CANHandler::handleResponse() {
    unsigned long rxId;
    byte len;
    static byte rxBuf[8]; // Static to persist after function returns

    // Check for incoming CAN messages
    if (can.checkReceive() == CAN_MSGAVAIL) {
        can.readMsgBuf(&rxId, &len, rxBuf);

        // Check if the message is a response from the ECU
        if (rxId == ecuResponseId && len >= 3 && rxBuf[1] == 0x41) {
            byte pid = rxBuf[2]; // Extract the PID from the response
            if (pidMap.find(pid) != pidMap.end()) {
                // Update the last response time for the PID
                lastResponseTime[pid] = millis();
                responseReceived[pid] = true; // Mark as response received
                return std::make_tuple(pid, rxBuf); // Return PID and raw message
            }
        }
    }
    return std::make_tuple(0xFF, nullptr); // Return invalid PID if no valid response
}

String CANHandler::convertToHumanReadable(byte pid, byte* rxBuf) {
    if (!rxBuf) return "No Data";

    int value = 0;
    if (pidMap.find(pid) == pidMap.end()) return "Unknown PID";

    String label = pidMap[pid];
    if (label == "RPM") {
        value = ((rxBuf[3] * 256) + rxBuf[4]) / 4;
        return String(value);
    } else if (label == "Speed") {
        value = rxBuf[3];
        return String(value);
    } else if (label == "Coolant_Temp") {
        value = rxBuf[3] - 40;
        return String(value);
    } else if (label == "Oil_Temp") {
        value = rxBuf[3] - 40;
        return String(value);
    } else if (label == "MAF") {
        float maf = ((rxBuf[3] * 256) + rxBuf[4]) / 100.0;
        return String(maf);
    }
    return "Unknown Data";
}

String CANHandler::getLabelForPID(byte pid) {
    if (pidMap.find(pid) != pidMap.end()) {
        return pidMap[pid];
    }
    return "Unknown PID";
}