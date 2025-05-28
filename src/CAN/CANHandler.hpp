#ifndef CAN_HANDLER_HPP
#define CAN_HANDLER_HPP

#include <SPI.h>
#include <mcp_can.h>
#include <map>
#include <Arduino.h>
#include <tuple>
#include <vector>
#include <queue>

#include "UTILS/CANResponse.hpp"
#include "UTILS/PIDConfig.hpp"

class CANHandler {
public:
    CANHandler(int csPin, std::map<byte, PIDConfig>& pidMapRef);
    bool begin();
    void sendRequests();
    // std::tuple<byte, byte*> handleResponse(); // Returns PID and raw message
    bool handleResponses(std::vector<CANResponse>& results);
    String convertToHumanReadable(byte pid, byte* rxBuf); // Converts raw data to human-readable
    String getLabelForPID(byte pid); // Returns the label for a given PID
private:
    MCP_CAN can;
    const unsigned long obdRequestId = 0x7DF; // Standard OBD-II request ID
    const unsigned long ecuResponseId = 0x7E8; // Standard response ID from ECU

    std::map<byte, PIDConfig>& pidMap;

    std::queue<byte> pidQueue;
    byte currentPid = 0;
    bool waitingForResponse = false;
    unsigned long lastResponseTime = 0;
    unsigned long lastRequestTime = 0;
    unsigned long lastIterationTime = 0;

    bool canInitialized = false; // Flag to check if CAN is initialized
};

#endif // CAN_HANDLER_HPP