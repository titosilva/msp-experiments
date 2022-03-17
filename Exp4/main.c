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

    volatile int i;
	while(1)
	{
        for (i = 100; i < 5000; i++)
        {
            buzzer_play(i);
            countdown(50000);
        }
	}

	return 0;
}

// FUNCTION IMPLEMENTATIONS -----------------------------
// Configs
void config_timers(void)
{
    // Buzzer (PWM)
    TA2CTL = TASSEL__ACLK | MC__UP;
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
//#pragma vector = TIMER2_A0_VECTOR
//__interrupt void timer2_a0_vector_interrupt(void)
//{
//    if (TA2R >= 40000 / scale[buzzer_note] - 100) {
//        ta2_counter++;
//    }
//
//    if (ta2_counter == 60000 / scale[buzzer_note]) {
//        ta2_counter = 0;
//        buzzer_note = buzzer_note + 1;
//        if (buzzer_note == 28) {
//            buzzer_note = 0;
//        }
//        TA2CCR0 = 40000 / scale[buzzer_note];
//        TA2CCR2 = 20000 / scale[buzzer_note];
//    }
//}
// ------------------------------------------------------
