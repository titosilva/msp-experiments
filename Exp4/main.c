#include <msp430.h> 

// FREQUENCIES (HZ)
#define FREQ_C 264
#define FREQ_D 297
#define FREQ_E 330
#define FREQ_F 352
#define FREQ_G 396
#define FREQ_A 440
#define FREQ_B 495

#define FREQ_ACLK 32768
#define FREQ_HALF_ACLK 16384

// GLOBAL VARS ------------------------------------------
volatile int waiting_falling_edge = 0;
volatile int rising_capture_ccr = -1;
volatile int falling_capture_ccr = -1;
// ------------------------------------------------------

// FUNCTION SIGNATURES ----------------------------------
// Configs
void config_timers(void);
void config_leds(void);

// Timer control
void buzzer_play(int frequency);

// Utils
int frequency_to_aclk_half_cycles(int frequency);
void countdown(int count);
// ------------------------------------------------------

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer

	config_timers();
	__enable_interrupt();

	while(1);

	return 0;
}

// FUNCTION IMPLEMENTATIONS -----------------------------
// Configs
void config_timers(void)
{
    // HC-SR04 Trigger
    TA0CTL = TASSEL__ACLK | MC__UP;
    // For about 20 measures / second;
    TA0CCR0 = 3276;
    TA0CCR4 = 1638;
    // Output (PWM)
    TA0CCTL4 = OUTMOD_6;
    P1SEL |= BIT5;
    P1DIR |= BIT5;

    // HC-SR04 Echo
    TA1CTL = TASSEL__SMCLK | MC__CONTINUOUS;
    // For about 20 measures / second;
    // Input (Capture - both edges)
    TA1CCTL1 = CM_3 | CCIS_0 | SCS | CAP | CCIE;
    P2SEL |= BIT0;

    // Buzzer
    TA2CTL = TASSEL__ACLK | MC__UP;
    // Output (PWM)
    TA2CCTL2 = OUTMOD_6;
    P2SEL |= BIT5;
    P2DIR |= BIT5;
}

// Timer control
void buzzer_play(int frequency)
{
    volatile int aclk_half_cycles = frequency_to_aclk_half_cycles(frequency);
    TA2CCR0 = aclk_half_cycles + aclk_half_cycles;
    TA2CCR2 = aclk_half_cycles;
}

// Utils
int frequency_to_aclk_half_cycles(int frequency)
{
    return FREQ_HALF_ACLK / frequency;
}

void countdown(int count)
{
    volatile int x, i;
    for (i = 0; i < 500; i++) {
        x = count;
        while(--x > 0);
    }
}
// ------------------------------------------------------

// INTERRUPTS -------------------------------------------
// Timers
#pragma vector = TIMER1_A1_VECTOR
__interrupt void timer1_a1_interrupt(void)
{
    if (TA1IV != TA1IV_TA1CCR1)
    {
        return;
    }

    volatile int rising_edge = P2IN & BIT0;
    if (rising_edge)
    {
        rising_capture_ccr = TA1CCR1;
        waiting_falling_edge = 1;
        return;
    }

    if (!rising_edge && waiting_falling_edge)
    {
        falling_capture_ccr = TA1CCR1;
        volatile int echo = falling_capture_ccr - rising_capture_ccr;
        echo += (TA1CTL & COV)? FREQ_ACLK : 0;
        waiting_falling_edge = 0;
    }


    TA1CCTL1 |= TACLR;
}
// ------------------------------------------------------
