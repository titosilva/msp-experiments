#include <msp430.h> 

// LEDS
#define SET_RED_LED (P1OUT |= BIT0)
#define RESET_RED_LED (P1OUT &= ~BIT0)
#define SET_GREEN_LED (P4OUT |= BIT7)
#define RESET_GREEN_LED (P4OUT &= ~BIT7)

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
volatile unsigned int dist_last_measurements[MAX_MEASUREMENTS];
volatile int quantity_of_measurements = 1;
// ------------------------------------------------------

// FUNCTION SIGNATURES ----------------------------------
// Configs
void config_timers(void);
void config_leds(void);

// Buzzer
void buzzer_play(int frequency);
unsigned int dist_to_buzzer_freq(unsigned int dist);

// Distance
unsigned int compute_dist(unsigned int echo_microsec);
void display_distance_on_leds(unsigned int dist);
// ------------------------------------------------------

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer

	config_leds();
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

void config_leds(void)
{
    // Red led
    P1SEL &= ~BIT0;
    P1DIR |= BIT0;
    RESET_RED_LED;

    // Green led
    P4SEL &= ~BIT7;
    P4DIR |= BIT7;
    RESET_GREEN_LED;
}

// Buzzer
void buzzer_play(int frequency)
{
    if (frequency == 0) {
        // Turn buzzer off
        P2SEL &= ~BIT5;
        P2OUT &= ~BIT5;
        return;
    }

    // Turn buzzer on
    P2SEL |= BIT5;
    volatile unsigned int wave_half_period = 500 / (frequency / 1000);
    TA2CCR0 = wave_half_period + wave_half_period;
    TA2CCR2 = wave_half_period;
}

unsigned int dist_to_buzzer_freq(unsigned int dist)
{
    return (dist > 50000 || dist < 5)? 0 : 5000 - ((dist >> 10) - 5) * 125;
}

// Distance
unsigned int compute_dist(unsigned int echo_microsec)
{
    return 17 * echo_microsec;
}

void display_distance_on_leds(unsigned int dist)
{
    if (dist > 50000) {
        RESET_GREEN_LED;
        RESET_RED_LED;
    } else if (dist > 30000) {
        RESET_GREEN_LED;
        SET_RED_LED;
    } else if (dist > 5000) {
        SET_GREEN_LED;
        RESET_RED_LED;
    } else {
        SET_GREEN_LED;
        SET_RED_LED;
    }
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

    if (waiting_falling_edge) {
        falling_capture_ccr = TA1CCR1;

        volatile unsigned int dist = compute_dist(
                (falling_capture_ccr > rising_capture_ccr)?
                falling_capture_ccr - rising_capture_ccr :
                0xFFFF + falling_capture_ccr - rising_capture_ccr
        );

        if (dist > 60000) {
            waiting_falling_edge = 0;
            return;
        }

        if (++quantity_of_measurements > MAX_MEASUREMENTS)
        {
            quantity_of_measurements = 1;
        }

        dist_last_measurements[quantity_of_measurements - 1] = dist;

        volatile unsigned int i = 0;
        volatile long sum = 0;
        for (i = 0; i < MAX_MEASUREMENTS; i++)
        {
            sum += dist_last_measurements[i];
        }

        volatile unsigned int average_dist = sum >> 4;

        buzzer_play(dist_to_buzzer_freq(average_dist));
        display_distance_on_leds(average_dist);

        waiting_falling_edge = 0;
    }
}
// ------------------------------------------------------
