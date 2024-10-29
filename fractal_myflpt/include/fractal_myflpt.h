#ifndef FRACTAL_MYFLPT_H
#define FRACTAL_MYFLPT_H

#include <stdint.h>


typedef struct
{
  int8_t hours;
  int8_t minutes;
  int8_t seconds;
} Time;

//! Colour type (5-bit red, 6-bit green, 5-bit blue)
typedef uint16_t rgb565;

//! \brief Custom floating point representation
typedef int32_t myfloat;

//! \brief Pointer to fractal point calculation function
typedef uint16_t (*calc_frac_point_p)(myfloat cx, myfloat cy, uint16_t n_max);

uint16_t calc_mandelbrot_point_soft(myfloat cx, myfloat cy, uint16_t n_max);

//! Pointer to function mapping iteration to colour value
typedef rgb565 (*iter_to_colour_p)(uint16_t iter, uint16_t n_max);

rgb565 iter_to_bw(uint16_t iter, uint16_t n_max);
rgb565 iter_to_grayscale(uint16_t iter, uint16_t n_max);
rgb565 iter_to_colour(uint16_t iter, uint16_t n_max);

void draw_fractal(rgb565 *fbuf, int width, int height,
                  calc_frac_point_p cfp_p, iter_to_colour_p i2c_p,
                  myfloat cx_0, myfloat cy_0, myfloat delta, uint16_t n_max);

//! \brief  Convert a IEEE float value to myfloat custom representation 
//! \param  float_value  to be converted to myfloat
myfloat float_to_myfloat(float float_value);

//! \brief  add two myfloat-point numbers
//! \param  a myfloat operand of addition
//! \param  b myfloat operand of addition
myfloat myfloat_addition(myfloat a, myfloat b);

//! \brief  multiply two myfloat-point numbers
//! \param  a myfloat operand of multiplication
//! \param  b myfloat operand of multiplication
myfloat myfloat_multiply(myfloat a, myfloat b);

//! \brief  negate myfloat-point number
//! \param  a myfloat to negate
myfloat myfloat_negate(myfloat a);

//! \brief  compare two myfloat-point numbers
//! \param  a myfloat operand of comparison
//! \param  b myfloat operand of comparison
uint32_t myfloat_less_than(myfloat a, myfloat b);

//! \brief  Sets a Time struct with the time read from RTC
//! \param  time Pointer to Time struct
void readTime(Time* time);

void print_myfloat_bits(myfloat myfloat_value);
void print_bits(int32_t float_value);
void print_float_bits(float float_value);


#endif // FRACTAL_MYFLPT_H
