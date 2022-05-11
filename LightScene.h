
#include <stdint.h>

#define I2C_ADDR 0x10
#define CONTROL_REG 0x0
#define REGISTER2 0x2
#define WHITE_CHANNEL 0x4
#define ALS_CHANNEL 0x5
#define ID 0x9

enum LightSetting{
  dark,
  dim,
  light,
  bright
};

class RGB
{
  public:
  uint8_t red;
  uint8_t green;
  uint8_t blue;

  RGB(int, int, int);
}



class LightScene 
{    
  public:
  LightScene();
  LightScene(RGB dark, RGB light);
  LightScene(RGB dark, RGB dim, RGB light, RGB bright);
  
  RGB Dark;
  RGB Dim;
  RGB Light;
  RGB Bright;
};
