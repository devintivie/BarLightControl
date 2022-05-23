#ifndef VEML3235SL_H // include guard
#define VEML3235SL_H


#include <stdint.h>

#define I2C_ADDR 0x10
#define CONTROL_REG 0x0
#define REGISTER2 0x2
#define WHITE_CHANNEL 0x4
#define ALS_CHANNEL 0x5
#define ID 0x9

class VEML3235SL 
{
    public:
        VEML3235SL();
        ~VEML3235SL();

        bool begin();
        void writeRegister(uint8_t address, uint16_t value);
        uint16_t readRegister(uint8_t address);
        uint16_t getALS();
};



#endif
