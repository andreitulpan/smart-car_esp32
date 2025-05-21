#include "BLEHandler.hpp"
#include "../LOG/LogHandler.hpp"

#define SERVICE_UUID "c19fb775-b69c-4ffb-9914-9cc0ea5549dd"
#define RX_CHARACTERISTIC_UUID "26baf8fc-9b45-48a1-8c51-d373281fc69d"
#define TX_CHARACTERISTIC_UUID "e360ea15-a6b1-43e1-bdff-465ad3eaf5d7"

BLEHandler::BLEHandler()
    : pServer(nullptr), pRxCharacteristic(nullptr), pTxCharacteristic(nullptr),
      receiveMessageCallback(nullptr), deviceConnectedCallback(nullptr), deviceDisconnectedCallback(nullptr) {}

void BLEHandler::begin(const std::string& deviceName) {
    // Initialize BLE with the provided device name
    BLEDevice::init(deviceName);

    // Create BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new CustomBLEServerCallbacks(this));

    // Create BLE Service
    BLEService* pService = pServer->createService(SERVICE_UUID);

    // Create RX Characteristic (to receive data from phone)
    pRxCharacteristic = pService->createCharacteristic(
        RX_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    pRxCharacteristic->setCallbacks(new CustomBLECharacteristicCallbacks(this));

    // Create TX Characteristic (to send data to phone)
    pTxCharacteristic = pService->createCharacteristic(
        TX_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pTxCharacteristic->addDescriptor(new BLE2902());

    // Start the service
    pService->start();
}

void BLEHandler::startListening() {
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->start();
    allowAdvertising = true;
    LogHandler::writeMessage(LogHandler::DebugType::BLE, String("BLE advertising started."));
}

void BLEHandler::stopListening() {
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->stop();
    allowAdvertising = false;
    disconnectDevice();
    LogHandler::writeMessage(LogHandler::DebugType::BLE, String("BLE is no longer discoverable."));
}

void BLEHandler::disconnectDevice() {
    if (deviceConnected) {
        pServer->disconnect(0); // Disconnect the first connected client
        LogHandler::writeMessage(LogHandler::DebugType::BLE, String("Device disconnected programmatically."));
    } else {
        LogHandler::writeMessage(LogHandler::DebugType::BLE, String("No device is currently connected."));
    }
}

void BLEHandler::setReceiveMessageCallback(std::function<void(const std::string&)> callback) {
    receiveMessageCallback = callback;
}

void BLEHandler::setDeviceConnectedCallback(std::function<void()> callback) {
    deviceConnectedCallback = callback;
}

void BLEHandler::setDeviceDisconnectedCallback(std::function<void()> callback) {
    deviceDisconnectedCallback = callback;
}

void BLEHandler::handle() {
    // Handle connection state changes
    if (!deviceConnected && oldDeviceConnected) {

        LogHandler::writeMessage(LogHandler::DebugType::BLE, String("Device disconnected."));
        if (deviceDisconnectedCallback) {
            deviceDisconnectedCallback();
        }
        delay(500);
        if (allowAdvertising) { // Only restart advertising if allowed
            LogHandler::writeMessage(LogHandler::DebugType::BLE, String("Restarting advertising..."));
            pServer->startAdvertising();
        }
        oldDeviceConnected = deviceConnected;
    }

    if (deviceConnected && !oldDeviceConnected) {
        LogHandler::writeMessage(LogHandler::DebugType::BLE, String("Device connected."));
        if (deviceConnectedCallback) {
            deviceConnectedCallback();
        }
        oldDeviceConnected = deviceConnected;
    }
}

void BLEHandler::sendMessage(const std::string& message) {
    if (deviceConnected && !message.empty()) {
        pTxCharacteristic->setValue(message);
        pTxCharacteristic->notify();
        LogHandler::writeMessage(LogHandler::DebugType::BLE, String("Message sent: ") + String(message.c_str()));
    } else {
        LogHandler::writeMessage(LogHandler::DebugType::BLE, String("Failed to send message: No device connected or message is empty."));
    }
}

// CustomBLEServerCallbacks Implementation
BLEHandler::CustomBLEServerCallbacks::CustomBLEServerCallbacks(BLEHandler* handler) : handler(handler) {}

void BLEHandler::CustomBLEServerCallbacks::onConnect(BLEServer* server) {
    handler->deviceConnected = true; // This should set the global flag
    LogHandler::writeMessage(LogHandler::DebugType::BLE, String("onConnect callback triggered."));
}

void BLEHandler::CustomBLEServerCallbacks::onDisconnect(BLEServer* server) {
    handler->deviceConnected = false; // This should reset the global flag
    LogHandler::writeMessage(LogHandler::DebugType::BLE, String("onDisconnect callback triggered."));
}

// CustomBLECharacteristicCallbacks Implementation
BLEHandler::CustomBLECharacteristicCallbacks::CustomBLECharacteristicCallbacks(BLEHandler* handler) : handler(handler) {}

void BLEHandler::CustomBLECharacteristicCallbacks::onWrite(BLECharacteristic* characteristic) {
    std::string value = characteristic->getValue();
    if (!value.empty() && handler->receiveMessageCallback) {
        handler->receiveMessageCallback(value);
    }
}