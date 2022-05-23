#ifndef RGB_H // include guard
#define RGB_H

#include <stdint.h>

class RGB
{
  public:
  uint8_t red;
  uint8_t green;
  uint8_t blue;

  RGB();
  RGB(uint8_t r, uint8_t g, uint8_t b);
  RGB(int r, int g, int b);
};



#endif
