#include "fractal_myflpt.h"
#include "swap.h"
#include "vga.h"
#include "cache.h"
#include <stddef.h>
#include <stdio.h>


// Constants describing the output device
const int SCREEN_WIDTH = 512;   //!< screen width
const int SCREEN_HEIGHT = 512;  //!< screen height

// Constants describing the initial view port on the fractal function
const float FRAC_WIDTH = 3.0; //!< default fractal width (3.0 in Q4.28)
const float CX_0 = -2.0;      //!< default start x-coordinate (-2.0 in Q4.28)
const float CY_0 = -1.5;      //!< default start y-coordinate (-1.5 in Q4.28)
const uint16_t N_MAX = 64;    //!< maximum number of iterations

int main() {
   myfloat CX_0_myfloat = float_to_myfloat(CX_0);
   myfloat CY_0_myfloat = float_to_myfloat(CY_0);

   volatile unsigned int *vga = (unsigned int *) 0x50000020;
   volatile unsigned int reg, hi;
   rgb565 frameBuffer[SCREEN_WIDTH*SCREEN_HEIGHT];
   float delta = FRAC_WIDTH / SCREEN_WIDTH;
   myfloat delta_myfloat = float_to_myfloat(delta);
   int i;
   vga_clear();
   printf("Starting drawing a fractal in myfloat representation\n");
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

   draw_fractal(frameBuffer,SCREEN_WIDTH,SCREEN_HEIGHT,&calc_mandelbrot_point_soft, &iter_to_colour,CX_0_myfloat,CY_0_myfloat,delta_myfloat,N_MAX);

#ifdef TEST_MODE   
   // Testing Addition
   printf("************* ADDITION BENCH TEST *************\n");
   // Case 2 : two positive 
   printf("Case 1 : \n 1.5 + 4.75 = 6.25\n");
   print_myfloat_bits(myfloat_addition(float_to_myfloat(1.5) ,float_to_myfloat(4.75)));

   // Case 2 : a < 0 and b > 0 and result > 0
   printf("Case 2 : \n -0.5 + 4.75 = 4.25\n");
   print_myfloat_bits(myfloat_addition(float_to_myfloat(-0.5) ,float_to_myfloat(4.75)));

   // Case 3 :  a > 0 and b < 0 and result > 0
   printf("Case 3 : \n 0.75 - 0.5 = 0.25\n");
   print_myfloat_bits(myfloat_addition(float_to_myfloat(0.75) ,float_to_myfloat(-0.5)));

   // Case 4 : a, b < 0
   printf("Case 4 : \n -0.5 - 4.75 = -5.25\n");
   print_myfloat_bits(myfloat_addition(float_to_myfloat(-0.5) ,float_to_myfloat(-4.75)));

   // Case 5 :  a < 0 and b > 0 and result < 0
   printf("Case 5 : \n -0.5  + 0.125 = -0.375\n");
   print_myfloat_bits(myfloat_addition(float_to_myfloat(-0.5) ,float_to_myfloat(0.125)));

   // Testing multiplication
   printf("************* MULTIPLICATION BENCH TEST *************\n");
   // Case 1 : two positive values > 1
   printf("Case 1 : \n 1.5 * 4.75 = 7.125\n");
   print_myfloat_bits(myfloat_multiply(float_to_myfloat(1.5) ,float_to_myfloat(4.75)));

   // Case 2 : two positive but a > 1 and b < 1
   printf("Case 2 : \n 0.5 * 4.75 = 2.375\n");
   print_myfloat_bits(myfloat_multiply(float_to_myfloat(0.5) ,float_to_myfloat(4.75)));

   // Case 3 : two positive values < 1
   printf("Case 3 : \n 0.5 * 0.75 = 0.375\n");
   print_myfloat_bits(myfloat_multiply(float_to_myfloat(0.5) ,float_to_myfloat(0.75)));

   // Case 4 : positive > 1 and negative < 1
   printf("Case 4 : \n -0.5 * 4.75 = -2.375\n");
   print_myfloat_bits(myfloat_multiply(float_to_myfloat(-0.5) ,float_to_myfloat(4.75)));

   // Case 5 : negative > 1 and negative < 1
   printf("Case 5 : \n -0.5 * (-4.75) = 2.375\n");
   print_myfloat_bits(myfloat_multiply(float_to_myfloat(-0.5) ,float_to_myfloat(-4.75)));

   // Case 6 : negative < 1 and negative < 1
   printf("Case 6 : \n -0.5 * (-0.75) = 0.375\n");
   print_myfloat_bits(myfloat_multiply(float_to_myfloat(-0.5) ,float_to_myfloat(-0.75)));
   
   // Less than test bench
   printf("************* LESS THAN BENCH TEST *************\n");
   printf("Case 1 : \n");
   printf(" 1.5 less than 2 = %d\n", myfloat_less_than(float_to_myfloat(1.5) ,float_to_myfloat(2.0)));

   printf("Case 2 : \n");
   printf(" -16.5 less than 2 = %d\n", myfloat_less_than(float_to_myfloat(-16.5) ,float_to_myfloat(2.0)));

   printf("Case 3 : \n");
   printf(" -15.785 less than -15.85 = %d\n", myfloat_less_than(float_to_myfloat(-15.785) ,float_to_myfloat(-15.85)));

   printf("Case 4 : \n");
   printf(" 12.9 less than 12.88 = %d\n", myfloat_less_than(float_to_myfloat(12.9) ,float_to_myfloat(12.88)));
#endif

#ifdef OR1300   
   dcache_flush();
#endif
   printf("Done\n");
}
