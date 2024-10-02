#include <stdio.h>
#include <vga.h>
#include <string.h>
#include <spr.h>
#include <convert_utoa.h>

unsigned int utoa(unsigned int number, char *buf, unsigned int bufsz, unsigned int base, const char *digits) {
    unsigned int q = 0;
    unsigned int r = 0;
    int pos = bufsz - 2; 

    if (!(bufsz > 1 && base > 1)) {
        /* Failure */
        buf[0] = '\0';
        return 0;
    }

    buf[bufsz - 1] = '\0';  // Null terminate the buffer

    do {
        if (pos < 0) {
            // If the buffer is too small, prevent overflow
            buf[0] = '\0'; 
            return 0;
        }
        q = number / base;
        r = number % base;
        buf[pos--] = digits[r];  
        number = q;  
    } while (number > 0);

    // Shift the result if it doesn't start at the beginning of the buffer
    memmove(buf, &buf[pos + 1], bufsz - pos - 1);

    // Return the number of characters written (excluding null terminator)
    return bufsz - pos - 2;
}

int main() {
    int reg;

    // Assuming `vga_clear()` is part of your environment, if not, omit it
    // vga_clear(); // Uncomment if necessary

    printf("\nConverting from base 10 to base 20...\n");

    unsigned int base = 20;
    unsigned int bufsz = 6;  // Increased size to accommodate larger numbers
    char buf[bufsz];
    const char *vigesimal_digits = "0123456789ABCDEFGHIJ";

    for (unsigned int i = 0; i < 100; ++i) {
        utoa(i, buf, bufsz, base, vigesimal_digits);
        printf("%u = %s\n", i, buf);
    }

    asm volatile ("l.ori %[out1],r1,0" : [out1] "=r" (reg));
    printf("My stacktop = 0x%08X\n", reg);

    return 0;
}