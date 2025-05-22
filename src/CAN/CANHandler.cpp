#include "CANHandler.hpp"

#include "../LOG/LogHandler.hpp"
#include "../SETTINGS/SettingsHandler.hpp"

CANHandler::CANHandler(int csPin) : can(csPin) {}

bool CANHandler::begin() {
    if (can.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
        LogHandler::writeMessage(LogHandler::DebugType::CAN, String("MCP2515 Initialized Successfully!"));
        can.setMode(MCP_NORMAL);
        canInitialized = true;
        return true;
    } else {
        LogHandler::writeMessage(LogHandler::DebugType::CAN, String("Error Initializing MCP2515..."));
        return false;
    }
}

void CANHandler::addPID(byte pid, const String& label) {
    pidMap[pid] = label;
}

void CANHandler::sendRequests() {
    if (!canInitialized || pidMap.empty()) return;

    unsigned long currentTime = millis();

    // If not waiting for a response, and enough time has passed since last response, send next PID
    if (!waitingForResponse && (currentTime - lastResponseTime >= 100) && (currentTime - lastIterationTime >= SettingsHandler::getCanRequestInterval())) {
        if (!pidQueue.empty()) {
            currentPid = pidQueue.front();
            pidQueue.pop();
            byte request[] = {0x02, 0x01, currentPid, 0x00, 0x00, 0x00, 0x00, 0x00};
            if (can.sendMsgBuf(obdRequestId, 0, 8, request) == CAN_OK) {
                LogHandler::writeMessage(LogHandler::DebugType::CAN, String("Request sent for PID: ") + String(currentPid, HEX) + " (" + pidMap[currentPid] + ")");
                waitingForResponse = true;
                lastRequestTime = currentTime;
            } else {
                LogHandler::writeMessage(LogHandler::DebugType::CAN, String("Error sending request for PID: ") + String(currentPid, HEX));
                waitingForResponse = false;
                lastResponseTime = currentTime; // Skip to next PID after error
            }
        }
    }
    // Timeout: if waiting for response and too much time has passed, skip to next PID
    if (waitingForResponse && (currentTime - lastRequestTime >= SettingsHandler::getCanResponseThreshold())) {
        LogHandler::writeMessage(LogHandler::DebugType::CAN, String("Timeout waiting for response for PID: ") + String(currentPid, HEX));
        waitingForResponse = false;
        lastResponseTime = currentTime;
    }
}

bool CANHandler::handleResponses(std::vector<CANResponse>& results) {
    unsigned long rxId;
    byte len;
    static byte rxBuf[8];
    while (can.checkReceive() == CAN_MSGAVAIL) {
        can.readMsgBuf(&rxId, &len, rxBuf);
        if (rxId == ecuResponseId && len >= 3 && rxBuf[1] == 0x41) {
            byte pid = rxBuf[2];
            if (pidMap.find(pid) != pidMap.end()) {
                if (waitingForResponse && pid == currentPid) {
                    waitingForResponse = false;
                    lastResponseTime = millis();
                }
                String pidLabel = getLabelForPID(pid);
                String humanReadable = convertToHumanReadable(pid, rxBuf);
                LogHandler::writeMessage(LogHandler::DebugType::CAN, String("Received Response: ") + pidLabel + " -> " + humanReadable, false);
                results.push_back({pidLabel, humanReadable});
                break;
            }
        }
    }

    if (pidQueue.empty() && !waitingForResponse) {
        lastIterationTime = millis();
        // If the queue is empty and not waiting for a response, refill the queue
        for (const auto& entry : pidMap) {
            pidQueue.push(entry.first);
        }
        return true;
    }

    return false;
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