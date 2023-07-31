#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <WiFi.h>

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
    Serial.println("WiFi scan started");
    pCharacteristic->setValue("WiFi scan started");
    pCharacteristic->notify();

    short n = WiFi.scanNetworks();
    delay(100);

    if (n == 0)
    {
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

        pCharacteristic->setValue(str);
    }

    pCharacteristic->notify();
    Serial.println();
}

void connectWiFi(std::string inputValue)
{
    Serial.println("WiFi connect started");
    pCharacteristic->setValue("WiFi connect started");
    pCharacteristic->notify();

    int ssidNumber = std::stoi(inputValue.substr(9, 2)) - 1;
    std::string password = inputValue.substr(12);
    password.pop_back();

    Serial.print("SSID: *");
    Serial.print(WiFi.SSID(ssidNumber).c_str());
    Serial.print("*");
    Serial.println();
    Serial.print("Password: *");
    Serial.print(password.c_str());
    Serial.print("*");
    Serial.println();

    WiFi.begin(WiFi.SSID(ssidNumber).c_str(), password.c_str());

    Serial.println("Connecting Wifi...");
    short timer = 0;
    while (WiFi.status() != WL_CONNECTED && timer < 20)
    {
        timer++;
        delay(500);
        Serial.print(".");
        Serial.print(WiFi.status());
    }

    if (timer == 20)
    {
        Serial.println("Connection failed");
        pCharacteristic->setValue("Connection failed");
    }
    else
    {
        Serial.println("");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        pCharacteristic->setValue("Connection success");
    }

    pCharacteristic->notify();
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
            connectWiFi(value);
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
