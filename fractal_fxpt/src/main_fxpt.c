#include "fractal_fxpt.h"
#include "swap.h"
#include "vga.h"
#include "cache.h"
#include <stddef.h>
#include <stdio.h>
#include <rtc.h>

#define NUM_INT    5                             // Number of bits for the integer part
#define NUM_FRAC   (32 - NUM_INT - 1)            // Remaining bits for the fractional part (32-bit total, 1 bit for sign)
#define INT_MASK   ((1 << NUM_INT) - 1)          // Mask for the integer part
#define FRAC_MASK  ((1 << NUM_FRAC) - 1)         // Mask for the fractional part

#define SIGN_MASK   (1 << 31)  // Sign bit mask for 32-bit
#define VALUE_MASK  ((1 << NUM_FRAC) - 1)  // Mask for the value part (excluding the sign)
#define MANTISSA_MASK ((1 << 23) - 1) 
#define IMPLICIT_ONE (1 << 23)


// Constants describing the output device
const int SCREEN_WIDTH = 512;   //!< screen width
const int SCREEN_HEIGHT = 512;  //!< screen height

// Constants describing the initial view port on the fractal function
const float FRAC_WIDTH = 3.0; //!< default fractal width (3.0 in Q4.28)
const float CX_0 = -2.0;      //!< default start x-coordinate (-2.0 in Q4.28)
const float CY_0 = -1.5;        //!< default start y-coordinate (-1.5 in Q4.28)
const uint16_t N_MAX = 64;    //!< maximum number of iterations


int main() {    

   fixed CX_0_fixed = float_to_fixed(CX_0);
   fixed CY_0_fixed = float_to_fixed(CY_0);

   volatile unsigned int *vga = (unsigned int *) 0x50000020;
   volatile unsigned int reg, hi;
   rgb565 frameBuffer[SCREEN_WIDTH*SCREEN_HEIGHT];
   float delta = FRAC_WIDTH / SCREEN_WIDTH;
   // convert to fixed point 
   fixed delta_fixed = float_to_fixed(delta);

   int i;
   vga_clear();
   printf("Starting drawing a fractal\n");

#ifdef OR1300   
   /* enable the caches */
   icache_write_cfg( CACHE_DIRECT_MAPPED | CACHE_SIZE_8K | CACHE_REPLACE_FIFO );
   dcache_write_cfg( CACHE_FOUR_WAY | CACHE_SIZE_8K | CACHE_REPLACE_LRU | CACHE_WRITE_BACK );
   icache_enable(1);
   dcache_enable(1);
#endif

   /* Enable the vga-controller's graphic mode */

   vga[0] = swap_u32(SCREEN_WIDTH);
   vga[1] = swap_u32(SCREEN_HEIGHT);
   vga[2] = swap_u32(1);
   vga[3] = swap_u32((unsigned int)&frameBuffer[0]);

   
   /* Clear screen */
   for (i = 0 ; i < SCREEN_WIDTH*SCREEN_HEIGHT ; i++) frameBuffer[i]=0;
   draw_fractal(frameBuffer,SCREEN_WIDTH,SCREEN_HEIGHT,&calc_mandelbrot_point_soft, &iter_to_colour,CX_0_fixed,CY_0_fixed,delta_fixed,N_MAX);

#ifdef OR1300   
   dcache_flush();
#endif
   printf("Done\n");
}
