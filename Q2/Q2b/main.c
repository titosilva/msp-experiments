#include <msp430.h> 
#include <inttypes.h>

// GLOBAL VARIABLES ------------------------------------------------
unsigned int sequence[10];
// -----------------------------------------------------------------

// FUNCTION SIGNATURES ---------------------------------------------
void createSequence(int n);
// -----------------------------------------------------------------

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	
	createSequence(10);
	createSequence(10);
	createSequence(10);
	createSequence(10);

	return 0;
}

// IMPLEMENTATIONS -------------------------------------------------
void createSequence(int n) {
    // Implementação utilizando Fibonacci LFSR
    // https://en.wikipedia.org/wiki/Linear-feedback_shift_register
    // Utilizando taps 16, 14, 12, 11
    volatile uint16_t state = 0xCAFEu + (uint16_t)(
            (sequence[0] << 0) +
            (sequence[1] << 1) +
            (sequence[2] << 2) +
            (sequence[4] << 4) +
            (sequence[5] << 5) +
            (sequence[6] << 6) +
            (sequence[7] << 7) +
            (sequence[8] << 8) +
            (sequence[9] << 9) );
    volatile uint16_t next_bit = 0;
    volatile int counter = 0;

    while (counter < n) {
        next_bit = ((state >> 0) ^ (state >> 2) ^ (state >> 4) ^ (state >> 5)) & 0x1;
        state = (next_bit << 15) | (state >> 1);
        sequence[counter] = next_bit;
        counter++;
    }
}
// -----------------------------------------------------------------
