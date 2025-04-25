#ifndef CAN_HANDLER_HPP
#define CAN_HANDLER_HPP

#include <SPI.h>
#include <mcp_can.h>

class CANHandler {
public:
    CANHandler(int csPin);
    bool begin();
    void requestAndReceive(byte pid, const String& label);
    void parseOBDResponse(byte* rxBuf, const String& label);

private:
    MCP_CAN can;
    const unsigned long obdRequestId = 0x7DF; // Standard OBD-II request ID
    const unsigned long ecuResponseId = 0x7E8; // Standard response ID from ECU
};

#endif // CAN_HANDLER_HPP