#include <avr32/io.h>

int main(void)
{
    AVR32_GPIO.port[0].oders = 1;
    AVR32_GPIO.port[0].ovrs = 1;

    for (;;) {
    }

    return 0;
}
