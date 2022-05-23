

#include "RGB.h"

RGB::RGB()
{
  red = (uint8_t)0;
  green = (uint8_t)0;
  blue = (uint8_t)0;
}

RGB::RGB(int r, int g, int b)
{
  red = (uint8_t)r;
  green = (uint8_t)g;
  blue = (uint8_t)b;
}

RGB::RGB(uint8_t r, uint8_t g, uint8_t b)
{
  red = r;
  green = g;
  blue = b;
}
