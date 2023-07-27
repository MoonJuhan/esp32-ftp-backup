#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "WiFi.h"

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
bool deviceConnected = false;

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

class MyServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
        Serial.println("connecting");
        deviceConnected = true;
    };

    void onDisconnect(BLEServer *pServer)
    {
        Serial.println("disconnecting");
        deviceConnected = false;
    }
};

void scanWiFi()
{
    Serial.println("scanWiFi");

    short n = WiFi.scanNetworks();

    if (n == 0)
    {
        Serial.println(F("no networks found"));
        pCharacteristic->setValue("no networks found");
    }
    else
    {
        std::string str = "";
        str += std::to_string(n);
        str += " networks found\n";

        for (short i = 0; i < n; ++i)
        {
            str += std::to_string(i + 1);
            str += " | ";
            str += WiFi.SSID(i).c_str();
            str += "\n";
            delay(10);
        }

        for (short i = 0; i < str.length(); i++)
        {
            Serial.print(str[i]);
        }

        pCharacteristic->setValue(str);
    }

    pCharacteristic->notify();
    Serial.println();
}

class MyCharacteristicCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        std::string value = pCharacteristic->getValue();

        if (value.compare("/scan") == 1)
        {
            scanWiFi();
        }

        if (value.length() > 9 && value.compare(0, 9, "/connect"))
        {
            Serial.println("connect command");
            Serial.println(WiFi.SSID(0).c_str());
            Serial.println();

            Serial.print("SSID No: ");
            Serial.print(value[9]);
            Serial.print(value[10]);
            Serial.println();

            Serial.print("Password: ");
            for (short i = 0; i < value.substr(12).length(); i++)
            {
                Serial.print(value.substr(12)[i]);
            }

            Serial.println();
        }
    }
};

void setup()
{
    Serial.begin(115200);

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    BLEDevice::init("ESP32");

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);

    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);

    pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());
    pCharacteristic->addDescriptor(new BLE2902());
    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinPreferred(0x0);

    BLEDevice::startAdvertising();

    Serial.println("start");
}

void loop()
{
    if (!deviceConnected)
    {
        pServer->startAdvertising();
    }

    delay(1000);
}
