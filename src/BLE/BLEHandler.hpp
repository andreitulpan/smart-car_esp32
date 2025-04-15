#ifndef BLE_HANDLER_H
#define BLE_HANDLER_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <functional>
#include <string>

class BLEHandler {
public:
    BLEHandler();
    void begin(const std::string& deviceName);
    void startListening();
    void stopListening();
    void disconnectDevice();
    void setReceiveMessageCallback(std::function<void(const std::string&)> callback);
    void setDeviceConnectedCallback(std::function<void()> callback);
    void setDeviceDisconnectedCallback(std::function<void()> callback);
    void handle();
    void sendMessage(const std::string& message); // New method to send messages

    bool deviceConnected;
    bool oldDeviceConnected;

private:
    BLEServer* pServer;
    BLECharacteristic* pRxCharacteristic;
    BLECharacteristic* pTxCharacteristic;
    bool allowAdvertising = false;

    std::function<void(const std::string&)> receiveMessageCallback;
    std::function<void()> deviceConnectedCallback;
    std::function<void()> deviceDisconnectedCallback;

    class CustomBLEServerCallbacks : public BLEServerCallbacks {
    public:
        CustomBLEServerCallbacks(BLEHandler* handler);
        void onConnect(BLEServer* server) override;
        void onDisconnect(BLEServer* server) override;

    private:
        BLEHandler* handler;
    };

    class CustomBLECharacteristicCallbacks : public BLECharacteristicCallbacks {
    public:
        CustomBLECharacteristicCallbacks(BLEHandler* handler);
        void onWrite(BLECharacteristic* characteristic) override;

    private:
        BLEHandler* handler;
    };
};

#endif // BLE_HANDLER_H