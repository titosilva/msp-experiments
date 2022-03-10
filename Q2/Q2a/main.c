#include <msp430.h> 
#include <inttypes.h>

#define SET_GREEN_LED P4OUT |= BIT7
#define RESET_GREEN_LED P4OUT &= ~BIT7
#define SET_RED_LED P1OUT |= BIT0
#define RESET_RED_LED P1OUT &= ~BIT0

// Function signatures ----------------------------------------
void start();
// ------------------------------------------------------------

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	
    start();

	return 0;
}

// IMPLEMENTATIONS -----------------------------------------------

// Initialization ===========================
void start() {
    // RED LED
    P1SEL &= ~BIT0; // GPIO
    P1DIR |= BIT0; // Out

    // GREEN LED
    P4SEL &= ~BIT7; // GPIO
    P4DIR |= BIT7; // Out

    SET_RED_LED;
    SET_GREEN_LED;
    countdown(50000);

    RESET_RED_LED;
    countdown(50000);

    RESET_GREEN_LED;
    countdown(50000);

    TA0CTL = TASSEL__SMCLK | ID__2 | MC__CONTINUOUSOUS | TACLR;
}

// Utils ===========================
void countdown(int16_t value)
{
    volatile unsigned int x = value;
    while (x--);
}

// ---------------------------------------------------------------

