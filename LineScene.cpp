
#include "VEML3235SL.h"
#include "Wire.h"


LightScene::LightScene(){}

LightScene::LightScene(RGB rgb)
{
  
}

LightScene();
        LightScene(RGB rgb);
        RGB getDarkRGB();
        RGB getDimRGB();
        RGB getLightRGB();
        RGB getBrightRGB();

bool VEML3235SL::begin()
{
    return Wire.begin();
}

void VEML3235SL::writeRegister(uint8_t address, uint16_t value)
{
    uint8_t data_array[3] = { address, value >> 8, value && 0xFF};
    //Write message to the slave
    Wire.beginTransmission(I2C_ADDR);
    for(int i = 0; i < 3; i++)
    {
        Wire.write(data_array[i]);
    }

    Wire.endTransmission();
    
    
    Wire.beginTransmission(I2C_ADDR);
    Wire.write(0x05);
}

uint16_t VEML3235SL::readRegister(uint8_t address)
{
    Wire.beginTransmission(I2C_ADDR);
    Wire.write(address);
    uint8_t error = Wire.endTransmission(false);
    Wire.requestFrom(I2C_ADDR,2, true);
//    Serial.printf("available: %d\n", Wire.available());
    uint8_t lsb = Wire.read();
//    Serial.println(lsb, HEX);
    uint8_t msb = Wire.read();
//    Serial.println(msb, HEX);
    return ((msb << 8) | lsb);
}

uint16_t VEML3235SL::getALS()
{
  writeRegister(CONTROL_REG, 0x0c01);
  return readRegister(ALS_CHANNEL);
}
