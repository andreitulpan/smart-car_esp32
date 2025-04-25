#ifndef CAN_HANDLER_HPP
#define CAN_HANDLER_HPP

#include <SPI.h>
#include <mcp_can.h>
#include <map>
#include <Arduino.h>
#include <tuple>

class CANHandler {
public:
    CANHandler(int csPin);
    bool begin();
    void addPID(byte pid, const String& label);
    void sendRequests();
    std::tuple<byte, byte*> handleResponse(); // Returns PID and raw message
    String convertToHumanReadable(byte pid, byte* rxBuf); // Converts raw data to human-readable
    String getLabelForPID(byte pid); // Returns the label for a given PID

private:
    MCP_CAN can;
    const unsigned long obdRequestId = 0x7DF; // Standard OBD-II request ID
    const unsigned long ecuResponseId = 0x7E8; // Standard response ID from ECU

    std::map<byte, String> pidMap; // Dictionary to store PID and label
    std::map<byte, unsigned long> lastRequestTime; // Tracks the last request time for each PID
    std::map<byte, unsigned long> lastResponseTime; // Tracks the last response time for each PID
    std::map<byte, bool> responseReceived; // Tracks whether a response has been received for each PID

    const unsigned long requestInterval = 5000; // 5 seconds between requests
    const unsigned long responseThreshold = 30000; // 30 seconds threshold for missing responses
};

#endif // CAN_HANDLER_HPP