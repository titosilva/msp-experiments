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
#define MAX_MEASUREMENTS 16

// GLOBAL VARS ------------------------------------------
volatile int waiting_falling_edge = 0;
volatile unsigned int rising_capture_ccr = 0;
volatile unsigned int falling_capture_ccr = 0;
volatile unsigned int echo_last_measurements[MAX_MEASUREMENTS];
volatile int echo_quantity_of_measurements = 1;
// ------------------------------------------------------

// FUNCTION SIGNATURES ----------------------------------
// Configs
void config_timers(void);
void config_leds(void);

// Timer control
void buzzer_play(int frequency);

// Utils
int frequency_to_aclk_half_cycles(int frequency);
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
    TA2CTL = TASSEL__SMCLK | MC__UP;
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
    return frequency >> 1;
}
// ------------------------------------------------------

// INTERRUPTS -------------------------------------------
// Timers
#pragma vector = TIMER1_A1_VECTOR
__interrupt void timer1_a1_interrupt(void)
{
    if (P2IN & BIT0)
    {
        rising_capture_ccr = TA1CCR1;
        waiting_falling_edge = 1;
        return;
    }

    falling_capture_ccr = TA1CCR1;

    if (++echo_quantity_of_measurements > MAX_MEASUREMENTS)
    {
        echo_quantity_of_measurements = 1;
    }
    echo_last_measurements[echo_quantity_of_measurements - 1] =
            (falling_capture_ccr < rising_capture_ccr)?
            falling_capture_ccr - rising_capture_ccr :
            0xFFFF + falling_capture_ccr - rising_capture_ccr;

    volatile unsigned int i = 0, sum = 0;
    for (i = 0; i < MAX_MEASUREMENTS; i++)
    {
        sum += echo_last_measurements[i];
    }
    buzzer_play(sum);
}
// ------------------------------------------------------
