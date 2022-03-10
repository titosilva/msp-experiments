#include <msp430.h> 

#define TOGGLE_GREEN_LED P4OUT ^= BIT7
#define DUTY_CYCLES_COUNT 5

unsigned int duty_cycles[DUTY_CYCLES_COUNT] = { 100, 300, 500, 700, 900 };
unsigned int current_duty_cycle = 0;
unsigned int debouncing = 0;

// SIGNATURES -----------------------------------
void config();
// ----------------------------------------------

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer

	config();
	__enable_interrupt();
	
	while(1);

	return 0;
}

// IMPLEMENTATIONS ------------------------------
void config() {
    // LEDS
    P1SEL |= BIT2; // Timer (PWM)
    P1DIR |= BIT2; // Out

    P4SEL &= ~BIT7; // GPIO
    P4DIR |= BIT7; // Out

    // BUTTONS
    P1SEL &= ~BIT1; // GPIO
    P1DIR &= ~BIT1; // In
    P1REN |= BIT1;
    P1OUT |= BIT1;
    P1IE |= BIT1;
    P1IES |= BIT1;

    P2SEL &= ~BIT1; // GPIO
    P2DIR &= ~BIT1; // In
    P2REN |= BIT1;
    P2OUT |= BIT1;
    P2IE |= BIT1;
    P2IES |= BIT1;

    // TIMERS
    // ACLK: 32768 Hz
    TA0CTL = TASSEL__SMCLK | MC__UP | TACLR;
    TA1CTL = TASSEL__ACLK | MC__UP | TACLR;

    // Green led timer
    TA1CCTL0 = CCIE;
    TA1CCR0 = 8192;

    TA0CCR0 = 1000;

    // Buttons timer
    TA0CCTL2 = CCIE;
    TA0CCR2 = 500;

    // P1.2 PWM
    TA0CCTL1 = CM_0 | OUTMOD_7 | OUT;
    TA0CCR1 = duty_cycles[current_duty_cycle];
}
// ----------------------------------------------

// INTERRUPTS -----------------------------------
#pragma vector = PORT1_VECTOR
__interrupt void port1_vector_interrupt(void) {
    switch(P1IV) {
    case P1IV_P1IFG1:
        if (debouncing > 0) {
            break;
        }

        current_duty_cycle += current_duty_cycle == DUTY_CYCLES_COUNT - 1? 0 : 1;
        TA0CCR1 = duty_cycles[current_duty_cycle];
        debouncing = 500;
    default:
        break;
    }
}

#pragma vector = PORT2_VECTOR
__interrupt void port2_vector_interrupt(void) {
    switch(P2IV) {
    case P2IV_P2IFG1:
        if (debouncing > 0) {
            break;
        }

        current_duty_cycle -= current_duty_cycle == 0? 0 : 1;
        TA0CCR1 = duty_cycles[current_duty_cycle];
        debouncing = 500;
    default:
        break;
    }
}

#pragma vector = TIMER1_A0_VECTOR
__interrupt void timer1_a0_vector_interrupt(void) {
    TOGGLE_GREEN_LED;
}

#pragma vector = TIMER0_A1_VECTOR
__interrupt void timer0_a1_vector_interrupt(void) {
    switch(TA0IV) {
    case TA0IV_TA0CCR2:
        if (debouncing > 0) {
            debouncing--;
        }
        break;
    default:
        break;
    }
}
// ----------------------------------------------
