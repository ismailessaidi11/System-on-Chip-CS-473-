#include "fractal_myflpt.h"
#include <swap.h>
#include <rtc.h>
#include <stdio.h>


// IEEE Floating point representation 
/*   ___________________________________________________
    |1 sign bit|  8 exponent bits  |  23 mantissa bits  |
    |__________|___________________|____________________|
*/
#define IEEE_EXPONENT_NUM_BIT  8                                 // Number of bits for exponent
#define IEEE_EXPONENT_MASK    ((1 << IEEE_EXPONENT_NUM_BIT) - 1) // Mask for exponent = 11111111b 
#define IEEE_BIAS              127                               // exponent_value = IEEE_exponent_bits - IEEE_BIAS
#define IEEE_MANTISSA_NUM_BIT  23                                // Number of bits for the mantissa
#define IEEE_MANTISSA_MASK    ((1 << IEEE_MANTISSA_NUM_BIT) - 1) // Mask for mantissa 
#define IEEE_IMPLICIT_ONE     (1 << IEEE_MANTISSA_NUM_BIT)       // 1.mantissa

// My-float representation 
/*   ___________________________________________________
    |1 sign bit|  23 mantissa bits  |  8 exponent bits  |
    |__________|____________________|___________________|
*/
#define MYFLOAT_EXPONENT_NUM_BIT  8                                    // Number of bits for exponent
#define MYFLOAT_EXPONENT_MASK    ((1 << IEEE_EXPONENT_NUM_BIT) - 1)    // Mask for exponent = 11111111b
#define MYFLOAT_BIAS             (uint32_t) 250                     // MYFLOAT_exponent_bits = exponent_value + MYFLOAT_BIAS = IEEE_exponent_bits - IEEE_BIAS + MYFLOAT_BIAS
#define BIAS_DIFFERENCE          (uint32_t) (MYFLOAT_BIAS - IEEE_BIAS)             // MYFLOAT_exponent_bits = IEEE_exponent_bits + BIAS_DIFFERENCE (123)
#define MYFLOAT_MANTISSA_NUM_BIT  23                                   // Number of bits for the mantissa
#define MYFLOAT_MANTISSA_MASK    (((1 << MYFLOAT_MANTISSA_NUM_BIT) - 1) << MYFLOAT_EXPONENT_NUM_BIT) // Mask for mantissa shifter to the level of the mantissa (bit 30 :: bit 8)
#define MYFLOAT_IMPLICIT_ONE     (1 << MYFLOAT_MANTISSA_NUM_BIT)       // 1.mantissa
#define SIGN_MASK                (1 << 31)                             // Sign bit mask for 32-bit


//! \brief  Mandelbrot fractal point calculation function
//! \param  cx    x-coordinate
//! \param  cy    y-coordinate
//! \param  n_max maximum number of iterations
//! \return       number of performed iterations at coordinate (cx, cy)
uint16_t calc_mandelbrot_point_soft(myfloat cx, myfloat cy, uint16_t n_max) {
  myfloat x = cx;
  myfloat y = cy;
  uint16_t n = 0;
  myfloat xx, yy, two_xy, minus_yy;
  myfloat two = float_to_myfloat(2.0);
  myfloat four = float_to_myfloat(4.0);
  do {
    xx = myfloat_multiply(x, x);
    yy = myfloat_multiply(y, y);
    two_xy = myfloat_multiply(myfloat_multiply(two, x), y);
    minus_yy = myfloat_negate(yy);

    x = myfloat_addition(myfloat_addition(xx, minus_yy), cx);
    y = myfloat_addition(two_xy, cy);
    ++n;
  } while ((myfloat_less_than(myfloat_addition(xx, yy), four)) && (n < n_max));
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
                  myfloat cx_0, myfloat cy_0, myfloat delta, uint16_t n_max) {
  
  Time start, end;
  readTime(&start);
  rgb565 *pixel = fbuf;
  myfloat cy = cy_0;
  for (int k = 0; k < height; ++k) {
    myfloat cx = cx_0;
    for(int i = 0; i < width; ++i) {
      uint16_t n_iter = (*cfp_p)(cx, cy, n_max);
      rgb565 colour = (*i2c_p)(n_iter, n_max);
      *(pixel++) = colour;
      cx = myfloat_addition(cx, delta);
    }
    cy = myfloat_addition(cy, delta);
  }
  readTime(&end);
  printf("run time : %02X:%02X:%02X\n", end.hours - start.hours, end.minutes - start.minutes, end.seconds - start.seconds);
}

//! \brief  Convert a IEEE float value to myfloat custom represention 
//! \param  float_value  to be converted to myfloat
myfloat float_to_myfloat(float float_value) {
  // Union to easily access the bits of the float value
  union {
      float f;
      uint32_t bits;
  } float_union;

  float_union.f = float_value;

  myfloat myfloat_value = 0;
  
  uint8_t IEEE_exponent = ((float_union.bits >> IEEE_MANTISSA_NUM_BIT) & IEEE_EXPONENT_MASK); // Exponent bits
  uint32_t mantissa = float_union.bits & IEEE_MANTISSA_MASK; 
  uint32_t sign = float_union.bits & SIGN_MASK; // Sign bit
  uint32_t exponent = IEEE_exponent + BIAS_DIFFERENCE; // exponent_value + MYFLOAT_BIAS (250) = IEEE_exponent + BIAS_DIFFERENCE (250 - 127 = 123)

  // Assemble the myfloat value
  myfloat_value |= sign;  // sign in bit 31
  myfloat_value |= (mantissa << MYFLOAT_EXPONENT_NUM_BIT);  // Mantissa in bit 8 :: bit 30
  myfloat_value |= (exponent & MYFLOAT_EXPONENT_MASK);  // Exponent in bit 0 :: bit 7

  return myfloat_value;
}

//! \brief  add two fixed-point numbers
//! \param  a myfloat operand of addition
//! \param  b myfloat operand of addition
myfloat myfloat_addition(myfloat a, myfloat b) {
  uint8_t exponent_a = a & MYFLOAT_EXPONENT_MASK;
  uint8_t exponent_b = b & MYFLOAT_EXPONENT_MASK;
  uint32_t mantissa_a = (a & MYFLOAT_MANTISSA_MASK) >> MYFLOAT_EXPONENT_NUM_BIT; // Handle them in 0 :: 22 form  instead of 8 :: 30
  uint32_t mantissa_b = (b & MYFLOAT_MANTISSA_MASK) >> MYFLOAT_EXPONENT_NUM_BIT; // Handle them in 0 :: 22 form  instead of 8 :: 30
  uint32_t sign_a = a & SIGN_MASK;
  uint32_t sign_b = b & SIGN_MASK;

  // Add the implicit 1 to the mantissa for normalized numbers
  mantissa_a |= (1 << MYFLOAT_MANTISSA_NUM_BIT);  // Adding implicit 1
  mantissa_b |= (1 << MYFLOAT_MANTISSA_NUM_BIT);  // Adding implicit 1

  // Alignement of the exponents and mantissas
  if (exponent_a > exponent_b) {
    mantissa_b >>= exponent_a - exponent_b;
    exponent_b =  exponent_a;
  } else {
    mantissa_a >>= exponent_b - exponent_a;
    exponent_a = exponent_b;
  }

  // Perform addition or substraction operation
  int32_t result_mantissa;
  int32_t result_sign;
  if (sign_a == sign_b) { // Same signs 
    result_mantissa = mantissa_a + mantissa_b;
    result_sign = sign_a;
  } else {                // Different signs
    if (mantissa_a > mantissa_b) {  // a is bigger that b
      result_mantissa = mantissa_a - mantissa_b;
      result_sign = sign_a;   
    } else {                        // b is bigger than a
      result_mantissa = mantissa_b - mantissa_a;
      result_sign = sign_b; 
    }
  }

  // Normalize the result
  uint8_t result_exponent = exponent_a > exponent_b ? exponent_a : exponent_b; // maximum exponent
  if (result_mantissa & (1 << (MYFLOAT_MANTISSA_NUM_BIT + 1))) {// Check for mantissa overflow 
    result_mantissa >>= 1;
    result_exponent++;
  }
  while ((result_mantissa & (1 << MYFLOAT_MANTISSA_NUM_BIT)) == 0 && result_mantissa != 0) { // while implicit one of result (bit23) is 0 and stop if result = 0 (in case of equal substraction)
    result_mantissa <<= 1;
    result_exponent--;
  }
  // Remove implicit one 
  result_mantissa &= IEEE_MANTISSA_MASK;

  myfloat result = result_sign | (result_mantissa << MYFLOAT_EXPONENT_NUM_BIT)  | result_exponent;
  return result;
}

myfloat myfloat_multiply(myfloat a, myfloat b) {
  // Extract the sign, exponent, and mantissa from a and b
  uint32_t sign_a = a & SIGN_MASK;
  uint32_t sign_b = b & SIGN_MASK;
  uint32_t sign_result = (sign_a ^ sign_b);  // XOR the signs to get the result's sign
  uint8_t exponent_a = a & MYFLOAT_EXPONENT_MASK;
  uint8_t exponent_b = b & MYFLOAT_EXPONENT_MASK;
  uint32_t mantissa_a = (a & MYFLOAT_MANTISSA_MASK) >> MYFLOAT_EXPONENT_NUM_BIT; // Handle them in 0 :: 22 form  instead of 8 :: 30
  uint32_t mantissa_b = (b & MYFLOAT_MANTISSA_MASK) >> MYFLOAT_EXPONENT_NUM_BIT; // Handle them in 0 :: 22 form  instead of 8 :: 30

  // Add the implicit 1 to the mantissa for normalized numbers
  mantissa_a |= (1 << MYFLOAT_MANTISSA_NUM_BIT);  
  mantissa_b |= (1 << MYFLOAT_MANTISSA_NUM_BIT); 

  // Multiply the mantissas
  uint64_t mantissa_product = (uint64_t)mantissa_a * (uint64_t)mantissa_b;

  // Normalize the mantissa result if needed
  if (mantissa_product  >= (1ULL << 2*MYFLOAT_MANTISSA_NUM_BIT + 1)) {  // The mantissa product is greater than 2.0 in 64 bit format (becaue 2*23 = 46 bits)
    mantissa_product >>= (MYFLOAT_MANTISSA_NUM_BIT + 1);  
    exponent_a += 1;  // Adjust exponent for normalization
  } else {
    mantissa_product >>= MYFLOAT_MANTISSA_NUM_BIT;  // Normalize the product
  }
   // restore mantissa in its place (bit8::bit30)
  mantissa_product <<= MYFLOAT_EXPONENT_NUM_BIT;
  mantissa_product &= MYFLOAT_MANTISSA_MASK;

  // Add the exponents 
  int32_t exponent_result = (int32_t)(exponent_a + exponent_b - MYFLOAT_BIAS); // because by simply adding we have 2*MYFLOAT_BIAS and want 1*MYFLOAT_BIAS

  // Handle overflow/underflow
  if (exponent_result >= MYFLOAT_EXPONENT_MASK) {
    // Overflow: Set to max or min float value depending on sign
    return sign_result | MYFLOAT_EXPONENT_MASK;
  } else if (exponent_result <= 0) {
    // Underflow: Return zero
    return sign_result;
  }

  myfloat result = sign_result  | mantissa_product | exponent_result;

  return result;
}

myfloat myfloat_negate(myfloat a) {
  return a ^ SIGN_MASK;
}

uint32_t myfloat_less_than(myfloat a, myfloat b) {
  // Extract the sign, exponent, and mantissa
  uint32_t sign_a = a & SIGN_MASK;
  uint32_t sign_b = b & SIGN_MASK;
  uint8_t exponent_a = a & MYFLOAT_EXPONENT_MASK;
  uint8_t exponent_b = b & MYFLOAT_EXPONENT_MASK;
  uint32_t mantissa_a = (a & MYFLOAT_MANTISSA_MASK);
  uint32_t mantissa_b = (b & MYFLOAT_MANTISSA_MASK);

  // If the signs are different
  if (sign_a != sign_b) {
    return (sign_a > sign_b) ? 1 : 0;  // If a is negative and b is positive, return 1 (a < b)
  }

  // Same signs: compare based on exponent and mantissa
  if (exponent_a != exponent_b) {
    if (sign_a == 0) {  // Both are positive
      return (exponent_a < exponent_b) ? 1 : 0;  
    } else {  // Both are negative
      return (exponent_a > exponent_b) ? 1 : 0; 
    }
  }

  // Same exponents: compare mantissa
  if (mantissa_a != mantissa_b) {
    if (sign_a == 0) {  // Both are positive
      return (mantissa_a < mantissa_b) ? 1 : 0;
    } else {  // Both are negative
      return (mantissa_a > mantissa_b) ? 1 : 0;
    }
  }
  return 0;  // If all are equal so false 
}

//! \brief  Sets a Time struct with the time read from RTC
//! \param  time Pointer to Time struct
void readTime(Time* time) {
  time->hours = readRtcRegister(2);   
  time->minutes = readRtcRegister(1); 
  time->seconds = readRtcRegister(0); 
}

void print_myfloat_bits(myfloat myfloat_value) {
  int32_t mantissa = (myfloat_value & MYFLOAT_MANTISSA_MASK) >> MYFLOAT_EXPONENT_NUM_BIT;
  int32_t exponent = myfloat_value & MYFLOAT_EXPONENT_MASK;

  // print sign bit
  int sign_bit = (myfloat_value >> 31) & 1;
  printf("%d | ", sign_bit);

  // print mantissa
  for (int i = MYFLOAT_MANTISSA_NUM_BIT - 1; i >= 0; i--) {  // Exclude the sign bit for printing
      printf("%d", (mantissa >> i) & 1);
  }
  
  printf(" | ");

  // print exponent
  for (int i = MYFLOAT_EXPONENT_NUM_BIT- 1; i >= 0; i--) {
      printf("%d", (exponent >> i) & 1);
  }
  printf("\n\n");
}

void print_bits(int32_t float_value) {
  for (int i = 31; i >= 0; i--) {
        // Use bitwise AND to check each bit
        unsigned int bit = (float_value >> i) & 1;
        printf("%d", bit);
    }
    printf("\n\n");
}
void print_float_bits(float float_value) {
     union {
      float f;
      uint32_t bits;
  } float_union;

  float_union.f = float_value;
  for (int i = 31; i >= 0; i--) {
        unsigned int bit = (float_union.bits >> i) & 1;
        printf("%d", bit);
    }
    printf("\n\n");
}

