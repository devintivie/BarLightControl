#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
//#include "BLEWifi.h"

#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>

#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

//EEPROM and EEPROM VARIABLES
#include <EEPROM.h>
#define EEPROM_SIZE 105
#define SSID_BASE 0
#define PW_BASE 34
#define SET_TEMP_BASE 100
#define TEMP_UNIT_BASE 101

#define LENGTH(x) (strlen(x)+1)

#define SERVICE_UUID "9e62a128-7ec7-4c72-a799-4f869de36642"
#define MESSAGE_UUID "60d73e30-02ad-460c-a5a3-b9ed26ec3ed4"
#define STATUS_UUID "b5594c77-cf4c-41ed-abfb-10c9679d3e66"
#define MAC_UUID "2078c59b-dde0-4b07-a35d-b170c314fe9a"

#define DEVINFO_UUID (uint16_t)0x180a
#define DEVINFO_MANUFACTURER_UUID (uint16_t)0x2a29
#define DEVINFO_NAME_UUID (uint16_t)0x2a24
#define DEVINFO_SERIAL_UUID (uint16_t)0x2a25

#define DEVICE_MANUFACTURER "Devin Ivie"
#define DEVICE_NAME "Ivie Bar LED Control "

#define RESET_HOLDDOWN_TIME 5000
#define WIFI_CONNECT_WAIT_TIME 8000

BLECharacteristic *characteristicMessage;
BLECharacteristic *characteristicStatus;
BLECharacteristic *characteristicMAC;

AsyncWebServer wifiServer(80);
unsigned long wifiStartTime = 0;
unsigned long wifiTryTime = 0;

bool isAdvertising = false;
int WifiStatus = 0;
int ALS = 0;

char StatusString[16];
char MAC[17];

//LED Control
#include <Adafruit_NeoPixel.h>
#define LED_DATA_PIN1 19
#define LED_DATA_PIN2 35
#define LED_COUNT1 70

#define LED_DATA_PIN3 4
#define LED_DATA_PIN4 19
#define LED_COUNT3 50

Adafruit_NeoPixel pixels1 = Adafruit_NeoPixel(LED_COUNT1, LED_DATA_PIN1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixels2 = Adafruit_NeoPixel(LED_COUNT1, LED_DATA_PIN2, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixels3 = Adafruit_NeoPixel(LED_COUNT3, LED_DATA_PIN3, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixels4 = Adafruit_NeoPixel(LED_COUNT3, LED_DATA_PIN4, NEO_GRB + NEO_KHZ800);


//Ambient Light Sensor Control
#include <Wire.h>
#define I2C_Freq 100000
#define SDA_0 22
#define SCL_0 21

#define SDA_1 32
#define SCL_1 33


#define I2C_DEV_ADDR 0x10
byte control_array[3] = { 0x0, 0x0c, 0x1};
byte dataArray[3] = {I2C_DEV_ADDR, 0x09, I2C_DEV_ADDR+1};

//TwoWire I2C_0 = TwoWire(16);
//TwoWire I2C_1 = TwoWire(16);

void StartAdvertising(BLEServer *btserver)
{
    BLEAdvertising *advertisement = btserver->getAdvertising();
    BLEAdvertisementData adv;
    //BLE MAC is reported in reverse byte order
    String chipId = String((uint32_t)(ESP.getEfuseMac() >> 24), HEX);
    String fullName = DEVICE_NAME + chipId;
    adv.setName(fullName.c_str());
    Serial.print("advertising name = ");
    Serial.println(fullName.c_str()); 
    adv.setCompleteServices(BLEUUID(SERVICE_UUID));
    advertisement->setAdvertisementData(adv);
    advertisement->start();
    isAdvertising = true;
}

class MyServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *btserver)
    {
        Serial.println("BLE Connected");
    };

    void onDisconnect(BLEServer *btserver)
    {
      isAdvertising = false;
      Serial.println("BLE Disconnected");
      if(WiFi.status() != WL_CONNECTED)
      {
        StartAdvertising(btserver);
      }
      else
      {
        
        ESP.restart();
      }
    }
};


char* GetWifiStatus()
{
  sprintf(StatusString, "%i", WifiStatus);
  return StatusString;
}

char* GetMAC()
{
  String mac = WiFi.macAddress();
  sprintf(MAC, "%s", mac.c_str());
  return MAC;
}

void WriteStringToFlash(const char *toStore, int startAddr)
{
  int i = 0;
  for(i = 0; i < LENGTH(toStore); i++)
  {
      EEPROM.write(startAddr + i, toStore[i]);
  }
  EEPROM.write(startAddr + i, '\0');
  EEPROM.commit();
}

String ReadStringFromFlash(int startAddr, int length)
{
  char in[length];
  char cursorIn;
  for(int i = 0; i< length; i++)
  {
    cursorIn = EEPROM.read(startAddr + i);
    in[i] = cursorIn;
  }

  return in;
}


int ReadIntFromFlash(int startAddr, int length)
{
  int value = EEPROM.read(startAddr);
  return value;
}

String LoadSSID()
{
  String ssid = ReadStringFromFlash(SSID_BASE, 33);
  return ssid;
}

String LoadPassword()
{
  String pw = ReadStringFromFlash(PW_BASE, 64);
  return pw;
}



void SaveCredentials(const char *ssid, const char *pw)
{
    WriteStringToFlash(ssid, SSID_BASE);
    WriteStringToFlash(pw, PW_BASE);
}

void ClearCredentials()
{
  for(int i = 0; i <SET_TEMP_BASE; i++ )
  {
    EEPROM.write(i, 0xFF);
  }
  EEPROM.commit();
}

//void i2cWriteRegister(uint8_t reg, uint16_t mask, uint16_t bits, uint8_t startPosition)
//{
//  uint16_t _i2cWrite; 
//
//  _i2cWrite = _readRegister(_wReg); // Get the current value of the register
//  _i2cWrite &= _mask; // Mask the position we want to write to.
//  _i2cWrite |= (_bits << _startPosition);  // Place the given bits to the variable
//  _i2cPort->beginTransmission(_address); // Start communication.
//  _i2cPort->write(_wReg); // at register....
//  _i2cPort->write(_i2cWrite); // Write LSB to register...
//  _i2cPort->write(_i2cWrite >> 8); // Write MSB to register...
//  _i2cPort->endTransmission(); // End communcation.
//}



void SetAllLEDs(int channel, int red, int green, int blue)
{

//    if(channel >= 1 && channel <= 4)
//    {
//      
//      Adafruit_NeoPixel thesePixels;
//      int led_count;
//      switch(channel)
//      {
//        case 1:
//          thesePixels = pixels1;
//          led_count = LED_COUNT1;
//        case 2:
//          thesePixels = pixels2;
//          led_count = LED_COUNT1;
//        case 3:
//          thesePixels = pixels3;
//          led_count = LED_COUNT3;
//        case 4:
//          thesePixels = pixels4;
//          led_count = LED_COUNT3;
//      }

      
      // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
      Serial.println("Send all LEDs a 255 and wait 2 seconds.");
     for(int l = 0; l < LED_COUNT1; l++) 
     {
       pixels1.setPixelColor(l, pixels1.Color(red,green,blue));
       
     }
     pixels1.show(); // This sends the updated pixel color to the hardware.
//    }
  
 
}


void UpdateFromALS()
{
  if(ALS >120)
  {
    SetAllLEDs(1, 20, 0, 0);
  }
  else
  {
    SetAllLEDs(1, 0, 20, 0);
  }
}


//////////////////////////////////////////////////
bool StartWifiServer(const char *ssid, const char *pw)
{
    Serial.printf("\n\rstart server \r\nssid = %s\r\npw = %s\r\n", ssid, pw);
    WiFi.begin(ssid, pw);
    WiFi.setHostname("BarLights");
    Serial.print("\r\nConnecting");
    wifiStartTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        wifiTryTime = millis();
        if(wifiTryTime - wifiStartTime > WIFI_CONNECT_WAIT_TIME)
        {
          Serial.println("\n Wifi Connection Timeout");
          bitSet(WifiStatus, 1);
          return false;
        }
    }

    if(!MDNS.begin("BarLights")) {
       Serial.println("Error starting mDNS");
    }else{
      Serial.println("mDNS Started");
    }

    MDNS.addService("iviebarlights", "tcp", 80);
    String mac = WiFi.macAddress();
    MDNS.addServiceTxt("iviebarlights", "tcp", "mac", mac);
    MDNS.addServiceTxt("iviebarlights", "tcp", "model", "custom");

    Serial.println(mac);
    bitSet(WifiStatus, 0);

    ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else //U_SPIFFS
        type = "filesystem";
      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());


    wifiServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        request->send(SPIFFS, "/index.html", "text/html");
    });

    wifiServer.on("/update", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        request->send(SPIFFS, "/update.html", "text/html");
    });

    wifiServer.on("/led/all", HTTP_PUT,
      [](AsyncWebServerRequest *request)
    {
//        Serial.println("1");
    },
    [](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final)
    {
      Serial.println("onupload");
    },
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
    {
      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, data);
      if (error) 
      {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.f_str());
          request->send(204, "text/plain", "error: parse error");
          return;
      }

      int channel = doc["channel"];
      int red = doc["red"];
      int green = doc["green"];
      int blue = doc["blue"];
      SetAllLEDs(channel, red, green, blue);
      request->send(200, "application/json", "unit set");
    });

    // Route to load style.css file
    wifiServer.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/style.css", "text/css");
    });
      
    wifiServer.begin();
    return true;
}



class MessageCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *characteristic)
    {
        std::string data = characteristic->getValue();
        Serial.println(data.c_str());

        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, data);
        if (error) 
        {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
            return;
        }

        const char* ssid = doc["ssid"];
        const char* pw = doc["password"];
        Serial.println(ssid);
        Serial.println(pw);

        SaveCredentials(ssid, pw);
        if(!StartWifiServer(ssid, pw))
        {
          ClearCredentials();
        }
    }

    void onRead(BLECharacteristic *characteristic)
    {
        BLEUUID uuid = characteristic->getUUID();
        std::string uuidString = uuid.toString();
        char buff[16];
        sprintf(buff, "%s", uuidString);
        Serial.println(buff);
        char *status = GetWifiStatus();
        characteristic->setValue(status);
    }
};

class StatusCallbacks : public BLECharacteristicCallbacks
{
    void onRead(BLECharacteristic *characteristic)
    {
        BLEUUID uuid = characteristic->getUUID();
        std::string uuidString = uuid.toString();
        char buff[16];
        sprintf(buff, "%s", uuidString);
        Serial.println(buff);
        char *status = GetWifiStatus();
        characteristic->setValue(status);
    }
};

class MACCallbacks : public BLECharacteristicCallbacks
{
    void onRead(BLECharacteristic *characteristic)
    {
        char *mac = GetMAC();
        characteristic->setValue(mac);
    }
};

void StartBluetoothServer()
{
  // Setup BLE Server
      BLEDevice::init(DEVICE_NAME);
      BLEServer *bleServer = BLEDevice::createServer();
      bleServer->setCallbacks(new MyServerCallbacks());
  
      // Register message service that can receive messages and reply with a static message.
      BLEService *service = bleServer->createService(SERVICE_UUID);
      characteristicMessage = service->createCharacteristic(MESSAGE_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_WRITE);
      characteristicMessage->setCallbacks(new MessageCallbacks());
      characteristicMessage->addDescriptor(new BLE2902());

      characteristicStatus = service->createCharacteristic(STATUS_UUID, BLECharacteristic::PROPERTY_READ);
      characteristicStatus->setCallbacks(new StatusCallbacks());
      characteristicMAC = service->createCharacteristic(MAC_UUID, BLECharacteristic::PROPERTY_READ);
      characteristicMAC->setCallbacks(new MACCallbacks());
      service->start();
      
      // Register device info service, that contains the device's UUID, manufacturer and name.
      service = bleServer->createService(DEVINFO_UUID);
      BLECharacteristic *characteristic = service->createCharacteristic(DEVINFO_MANUFACTURER_UUID, BLECharacteristic::PROPERTY_READ);
      characteristic->setValue(DEVICE_MANUFACTURER);
      characteristic = service->createCharacteristic(DEVINFO_NAME_UUID, BLECharacteristic::PROPERTY_READ);
      characteristic->setValue(DEVICE_NAME);
      characteristic = service->createCharacteristic(DEVINFO_SERIAL_UUID, BLECharacteristic::PROPERTY_READ);
      String chipId = String((uint32_t)(ESP.getEfuseMac() >> 24), HEX);
      characteristic->setValue(chipId.c_str());
      service->start();

//      currentTemperatureSetting = DEFAULT_THERMOSTAT_SETTING;
//      SaveTempSettings(currentTemperatureSetting);
//      SaveUnitSettings(isCelsius);
      
      // Advertise services
      StartAdvertising(bleServer);
      Serial.println("Advertising");
}


void setup()
{
    Serial.begin(115200);
    Wire.begin();
//    I2C_0.begin(SDA_0, SCL_0, I2C_Freq);
//    I2C_1.begin(SDA_1, SCL_1, I2C_Freq);
    //Setup EEPROM
    pixels1.begin(); // This initializes the NeoPixel library.
    pixels2.begin(); 
    pixels3.begin();
    pixels4.begin();

  SetAllLEDs(1, 1,0,1);
    EEPROM.begin(EEPROM_SIZE);
    if(EEPROM.read(0) == 0xff)
    {
//      StartBluetoothServer();
        const char* ssid = ssid;
        const char* pw = pw;
        Serial.println(ssid);
        Serial.println(pw);

        SaveCredentials(ssid, pw);
    }
    else
    {
        String ssid = LoadSSID();
        String pw = LoadPassword();
        StartWifiServer(ssid.c_str(), pw.c_str());
    }
    
    // Initialize SPIFFS
    if(!SPIFFS.begin(true)){
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }
    
}

void loop()
{
  delay(1000);
  //Write message to the slave
  Wire.beginTransmission(I2C_DEV_ADDR);
  for(int t = 0; t < 3; t++)
  {
    Wire.write(control_array[t]);
  }

  Wire.endTransmission();
  
  
  Wire.beginTransmission(I2C_DEV_ADDR);
  Wire.write(0x05);
//  for (int i=0; i< 3; i++)
//  {
//    Wire.write(dataArray[i]);
//  }
  uint8_t error = Wire.endTransmission(false);
  Wire.requestFrom(I2C_DEV_ADDR,2, true);
  Serial.printf("available: %d\n", Wire.available());
  uint8_t lsb = Wire.read();
    Serial.println(lsb, HEX);
    uint8_t msb = Wire.read();
    Serial.println(msb, HEX);

    ALS = ((msb << 8) | lsb);
    UpdateFromALS();
    
    Serial.println(ALS);
//  while(Wire.available() >= 1){
//    int c = Wire.read();
//    Serial.println(c, HEX);
//  }
  Serial.printf("endTransmission: %u\n", error);
  ArduinoOTA.handle();
//  delay(1000);
}
