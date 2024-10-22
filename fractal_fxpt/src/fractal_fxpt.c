#include "fractal_fxpt.h"
#include <swap.h>
#include <stdint.h>
#include <stdio.h>


#define NUM_INT    6                             // Number of bits for the integer part
#define NUM_FRAC   (32 - NUM_INT - 1)            // Remaining bits for the fractional part (32-bit total, 1 bit for sign)
#define INT_MASK   ((1 << NUM_INT) - 1)          // Mask for the integer part
#define FRAC_MASK  ((1 << NUM_FRAC) - 1)         // Mask for the fractional part
#define FIXED_SCALE (1 << NUM_FRAC)               // Scaling factor for fixed-point representation

#define SIGN_MASK   (1 << 31)  // Sign bit mask for 32-bit
#define MANTISSA_MASK ((1 << 23) - 1) 
#define EXPONENT_MASK 0xFF
#define IMPLICIT_ONE (1 << 23)

// Floating point representation
#define EXPONENT_NUM_BIT 8
#define EXPONENT_MAX ((1 << EXPONENT_NUM_BIT) - 1)  // 2^EXPONENT_NUM_BIT - 1 = 255 (for 8 expo)
#define FLOAT_BIAS 127
#define MANTISSA_NUM_BIT 23

//! \brief  Mandelbrot fractal point calculation function
//! \param  cx    x-coordinate
//! \param  cy    y-coordinate
//! \param  n_max maximum number of iterations
//! \return       number of performed iterations at coordinate (cx, cy)
uint16_t calc_mandelbrot_point_soft(fixed cx, fixed cy, uint16_t n_max) {
    
  fixed x = cx;
  fixed y = cy;
  uint16_t n = 0;
  fixed xx, yy, two_xy, minus_yy;
  fixed two = float_to_fixed(2.0);
  do {
    xx = fixed_point_multiply(x, x);
    yy = fixed_point_multiply(y, y);
    two_xy = fixed_point_multiply(fixed_point_multiply(two, x), y);
    
    x = xx - yy + cx;
    y = two_xy + cy;

    ++n;
  } while (((xx + yy) < float_to_fixed(4.0)) && (n < n_max));
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


// Convert a fixed-point value back to a float
float fixed_to_float(fixed fixed_value) {
    // Convert fixed-point back to float by dividing by scale factor
    return (float)fixed_value / FIXED_SCALE;
}

// Multiply two fixed-point numbers
fixed fixed_point_multiply(fixed a, fixed b) {
    int64_t temp = (int64_t)a * (int64_t)b;
    fixed result = (fixed)(temp >> NUM_FRAC);  // Shift right to scale back
    return result;
}


fixed float_to_fixed(float float_value) {
    // Union to easily access the bits of the float value
    union {
        float f;
        uint32_t bits;
    } float_union;

    float_union.f = float_value;
    fixed fixed_value;
    
    // Extracting the parts of the float
    uint32_t sign = float_union.bits & SIGN_MASK; // Sign bit
    uint8_t exponent_bits = ((float_union.bits >> MANTISSA_NUM_BIT) & EXPONENT_MASK); // Exponent bits
    uint32_t mantissa = IMPLICIT_ONE | (float_union.bits & MANTISSA_MASK); // Add implicit 1
    
    int8_t exponent = exponent_bits - FLOAT_BIAS; // Bias of 127 for the exponent
    
    if (exponent_bits == 0) {
        // Denormalized number, treat as 0 for simplicity in this range
        fixed_value = 0;
    } else if (exponent_bits == EXPONENT_MAX) {
        // Inf/NaN are not representable in fixed-point
        fixed_value = 0; 
    } else {
        // Normalized number
        if (exponent >= 0) {
            // Left shift mantissa if exponent is positive
            mantissa <<= exponent;
        } else {
            // Right shift mantissa if exponent is negative
            mantissa >>= -exponent;
        }
        
        // Adjust mantissa to fit into the fixed-point format
        fixed_value = (mantissa << (NUM_FRAC - MANTISSA_NUM_BIT)); // Scale the mantissa to NUM_FRAC
        
        // Apply the sign to the fixed-point result
        if (sign) {
            fixed_value = -fixed_value;
        }
    }
    
    return fixed_value;
}

void print_fixed_point_bits(fixed fixed_point_value) {  
  int32_t int_part = (fixed_point_value & (INT_MASK<<NUM_FRAC)) >> NUM_FRAC;
  int32_t frac_part = fixed_point_value & FRAC_MASK;

  //sign bit 
  int sign_bit = (fixed_point_value >> 31) & 1;
  printf("%d | ", sign_bit);

  // integer part 
  for (int i = NUM_INT - 1; i >= 0; i--) {  // Exclude the sign bit for printing
      printf("%d", (int_part >> i) & 1);
  }
  printf(" | ");
  
  // fractional part 
  for (int i = NUM_FRAC - 1; i >= 0; i--) {
      printf("%d", (frac_part >> i) & 1);
  }
  printf("\n");
}