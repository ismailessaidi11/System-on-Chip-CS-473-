#ifndef FRACTAL_FXPT_H
#define FRACTAL_FXPT_H

#include <stdint.h>

//! Colour type (5-bit red, 6-bit green, 5-bit blue)
typedef uint16_t rgb565;

//! \brief Fixed point type in Q4.28 format
typedef int32_t fixed;

//! \brief Pointer to fractal point calculation function
typedef uint16_t (*calc_frac_point_p)(fixed cx, fixed cy, uint16_t n_max);

uint16_t calc_mandelbrot_point_soft(fixed cx, fixed cy, uint16_t n_max);

//! Pointer to function mapping iteration to colour value
typedef rgb565 (*iter_to_colour_p)(uint16_t iter, uint16_t n_max);

rgb565 iter_to_bw(uint16_t iter, uint16_t n_max);
rgb565 iter_to_grayscale(uint16_t iter, uint16_t n_max);
rgb565 iter_to_colour(uint16_t iter, uint16_t n_max);

void draw_fractal(rgb565 *fbuf, int width, int height,
                  calc_frac_point_p cfp_p, iter_to_colour_p i2c_p,
                  fixed cx_0, fixed cy_0, fixed delta, uint16_t n_max);

//! \brief Calculate binary logarithm for unsigned integer argument x
//! \note  For x equal 0, the function returns -1.
int ilog2(unsigned x);

fixed float_to_fixed(float value);
fixed fixed_point_multiply(fixed a, fixed b);
void print_fixed_point_bits(fixed fixed_point_value);
//float fixed_to_float(fixed fixed_value);

#endif // FRACTAL_FXPT_H
