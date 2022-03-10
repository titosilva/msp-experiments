#include <msp430.h> 
#include <inttypes.h>

#define SET_GREEN_LED P4OUT |= BIT7
#define RESET_GREEN_LED P4OUT &= ~BIT7
#define SET_RED_LED P1OUT |= BIT0
#define RESET_RED_LED P1OUT &= ~BIT0

// GLOBAL VARIABLES ------------------------------------------------
unsigned int sequence[10];
unsigned int currently_showing_backwards = 10;
unsigned int current_sequence_size = 10;
// -----------------------------------------------------------------

// FUNCTION SIGNATURES ---------------------------------------------
void config();
void start();
void createSequence(int n);
void showSequence(int n);
void countdown(int16_t value);
// -----------------------------------------------------------------

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    config();
    start();
    createSequence(current_sequence_size);
    showSequence(current_sequence_size);

    while(1);

    return 0;
}

// IMPLEMENTATIONS -------------------------------------------------
void config() {
    // RED LED
    P1SEL &= ~BIT0; // GPIO
    P1DIR |= BIT0; // Out

    // GREEN LED
    P4SEL &= ~BIT7; // GPIO
    P4DIR |= BIT7; // Out

    // Sequence timer
    TA0CTL = TASSEL__ACLK | MC__UP | TACLR;
    TA0CCTL0 = CCIE;
    TA0CCR0 = 32760;

    __enable_interrupt();
}

// Initialization ===========================
void start() {
    SET_RED_LED;
    SET_GREEN_LED;
    countdown(50000);

    RESET_RED_LED;
    countdown(50000);

    RESET_GREEN_LED;
    countdown(50000);
}

// Sequences =================================
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

void showSequence(int n) {
    currently_showing_backwards = current_sequence_size;

    while(currently_showing_backwards);
}

// Utils ================================
void countdown(int16_t value)
{
    volatile int16_t x = value;
    while (x--);
}
// -----------------------------------------------------------------

// INTERRUPTS ------------------------------------------------------
#pragma vector = TIMER0_A0_VECTOR
__interrupt void timer0_a0_vector_interrupt(void) {
    if (currently_showing_backwards >= 1) {
       uint16_t current_bit = sequence[current_sequence_size - currently_showing_backwards];

       if (current_bit) {
           SET_RED_LED;
           RESET_GREEN_LED;
       } else {
           RESET_RED_LED;
           SET_GREEN_LED;
       }

       currently_showing_backwards--;
    }

    if (!currently_showing_backwards) {
        RESET_GREEN_LED;
        RESET_RED_LED;
    }
}
// -----------------------------------------------------------------
