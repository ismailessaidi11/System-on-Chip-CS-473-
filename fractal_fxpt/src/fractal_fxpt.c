#include "fractal_fxpt.h"
#include <swap.h>
#include <stdint.h>

//! \brief  Mandelbrot fractal point calculation function
//! \param  cx    x-coordinate
//! \param  cy    y-coordinate
//! \param  n_max maximum number of iterations
//! \return       number of performed iterations at coordinate (cx, cy)
uint16_t calc_mandelbrot_point_soft(fixed cx, fixed cy, uint16_t n_max) {
  fixed x = cx;
  fixed y = cy;
  uint16_t n = 0;
  fixed xx, yy, two_xy;
  do {
    xx = x * x;
    yy = y * y;
    two_xy = 2 * x * y;

    x = xx - yy + cx;
    y = two_xy + cy;
    ++n;
  } while (((xx + yy) < float_to_q4_28(4.0)) && (n < n_max));
  return n;
}


//! \brief  Map number of performed iterations to black and white
//! \param  iter  performed number of iterations
//! \param  n_max maximum number of iterations
//! \return       colour
rgb565 iter_to_bw(uint16_t iter, uint16_t n_max) {
  if (iter == n_max) {
    return 0x0000;
  }
  return 0xffff;
}


//! \brief  Map number of performed iterations to grayscale
//! \param  iter  performed number of iterations
//! \param  n_max maximum number of iterations
//! \return       colour
rgb565 iter_to_grayscale(uint16_t iter, uint16_t n_max) {
  if (iter == n_max) {
    return 0x0000;
  }
  uint16_t brightness = iter & 0xf;
  return swap_u16(((brightness << 12) | ((brightness << 7) | brightness<<1)));
}


//! \brief Calculate binary logarithm for unsigned integer argument x
//! \note  For x equal 0, the function returns -1.
int ilog2(unsigned x) {
  if (x == 0) return -1;
  int n = 1;
  if ((x >> 16) == 0) { n += 16; x <<= 16; }
  if ((x >> 24) == 0) { n += 8; x <<= 8; }
  if ((x >> 28) == 0) { n += 4; x <<= 4; }
  if ((x >> 30) == 0) { n += 2; x <<= 2; }
  n -= x >> 31;
  return 31 - n;
}


//! \brief  Map number of performed iterations to a colour
//! \param  iter  performed number of iterations
//! \param  n_max maximum number of iterations
//! \return colour in rgb565 format little Endian (big Endian for openrisc)
rgb565 iter_to_colour(uint16_t iter, uint16_t n_max) {
  if (iter == n_max) {
    return 0x0000;
  }
  uint16_t brightness = (iter&1)<<4|0xF;
  uint16_t r = (iter & (1 << 3)) ? brightness : 0x0;
  uint16_t g = (iter & (1 << 2)) ? brightness : 0x0;
  uint16_t b = (iter & (1 << 1)) ? brightness : 0x0;
  return swap_u16(((r & 0x1f) << 11) | ((g & 0x1f) << 6) | ((b & 0x1f)));
}

rgb565 iter_to_colour1(uint16_t iter, uint16_t n_max) {
  if (iter == n_max) {
    return 0x0000;
  }
  uint16_t brightness = ((iter&0x78)>>2)^0x1F;
  uint16_t r = (iter & (1 << 2)) ? brightness : 0x0;
  uint16_t g = (iter & (1 << 1)) ? brightness : 0x0;
  uint16_t b = (iter & (1 << 0)) ? brightness : 0x0;
  return swap_u16(((r & 0xf) << 12) | ((g & 0xf) << 7) | ((b & 0xf)<<1));
}

//! \brief  Draw fractal into frame buffer
//! \param  width  width of frame buffer
//! \param  height height of frame buffer
//! \param  cfp_p  pointer to fractal function
//! \param  i2c_p  pointer to function mapping number of iterations to colour
//! \param  cx_0   start x-coordinate
//! \param  cy_0   start y-coordinate
//! \param  delta  increment for x- and y-coordinate
//! \param  n_max  maximum number of iterations
void draw_fractal(rgb565 *fbuf, int width, int height,
                  calc_frac_point_p cfp_p, iter_to_colour_p i2c_p,
                  fixed cx_0, fixed cy_0, fixed delta, uint16_t n_max) {
  rgb565 *pixel = fbuf;
  fixed cy = cy_0;
  for (int k = 0; k < height; ++k) {
    fixed cx = cx_0;
    for(int i = 0; i < width; ++i) {
      uint16_t n_iter = (*cfp_p)(cx, cy, n_max);
      rgb565 colour = (*i2c_p)(n_iter, n_max);
      *(pixel++) = colour;
      cx += delta;
    }
    cy += delta;
  }
}

fixed float_to_q4_28(float float_value) {
  union 
  {
    float f;
    uint32_t bits;
  } float_union;

  float_union.f = float_value;
  fixed fixed_value;
  uint32_t mantissa;
  uint8_t integer;
  int8_t exponent;
  uint8_t exponent_bits = ((float_union.bits >> 23) & 0xFF); // Shift right exponent (b30::b23 to b7::b0)

  if (exponent_bits <= 0) { // Denormalized representation (We will never reach the denormalized representation as min value of q4_28 is 2^-28)
    fixed_value = 0;
  } else { // Normalized representation
    exponent = exponent_bits - 127;
    mantissa = 0x00800000 | (float_union.bits & 0x007FFFFF); // implicit 1 of mantissa + 23 LSB
    if (exponent >= 0) {
      integer =  mantissa >> (23 - exponent); // 3 bits for integer
      mantissa = (mantissa << exponent) & 0x007FFFFF; // left shift (multiply by 2^exponent)
    } else {
      integer = 0;
      mantissa = (mantissa >> -exponent) & 0x007FFFFF; // right shift (divide by 2^|exponent|)
    }
    fixed_value = (float_union.bits & 0x80000000) | (integer << 28) | (mantissa << 5);  // sign + integer (28 = 32 - 4)

  }
  return fixed_value;
}
