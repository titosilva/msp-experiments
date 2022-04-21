#include <msp430.h> 

int diff;

#define RED_LED_ON P1OUT |= BIT0
#define RED_LED_OFF P1OUT &= ~BIT0
#define GREEN_LED_ON P4OUT |= BIT7
#define GREEN_LED_OFF P4OUT &= ~BIT7

void configure_leds();
void configure_adc_trigger_clk();
void configure_adc();

int main(void)
{
    // Stop watchdog timer
    WDTCTL = WDTPW | WDTHOLD;

    configure_leds();
    configure_adc_trigger_clk();
    configure_adc();

    __enable_interrupt();

    while(1);
}

void configure_leds()
{
    P1SEL &= ~BIT0;
    P1DIR |= BIT0;

    P4SEL &= ~BIT7;
    P4DIR |= BIT7;
}

void configure_adc_trigger_clk()
{
    TA0CTL = TASSEL__ACLK | MC_1;
    TA0CCTL1 = OUTMOD_6;

    TA0CCR0 = 3276; // ACLK / 10 ~ 10Hz
    TA0CCR1 = TA0CCR0 >> 1; // 50% duty cycle
}

void configure_adc()
{
    // Desabilita o módulo ADC para configurações
    ADC12CTL0 &= ~ADC12ENC;

    // Pinos 6.1 e 6.0 como entradas analógicas
    P6SEL |= BIT1 | BIT0;

    // Cálculo do tempo de amostragem, considerando n = 8
    // Tamost > (Rs + Rin) * Cin * (n+1) * ln(2) + 800ns
    // Tamost > (10k + 1.8k) * 25p * (8+1) * ln(2) + 800ns
    // Tamost > 11.8 * 25 * 9 * ln(2) * 10^(-9) + 800 * 10^(-9)
    // Tamost > 2640,306 * 10^(-9)
    // Tamost > 2,64 us

    // Considerando um ADC12CLK de 5MHz, temos que o Tclk = 0,2us
    // Assim, tomando K como o número de ciclos de ADC12CLK necessários para o sampling time,
    // Tamost > 2,64us
    // K * 0,2us > 2,64us
    // K > 13,2
    // Portanto, como a quantidade de ciclos de clock deve ser um valor de bits no SHT do A/D válido,
    // K = 16

    // Como será utilizado o modo de sequência de canais sem repetição para 2 canais, o tempo utilizado
    // a cada leitura das portas analógicas será de 16 * 0,2us * 2 = 6,4us
    // Assim, o máximo de leituras que poderiam ser feitas por segundo é dado por
    // f = 1s / 6,4us = 156250 leituras por segundo

    ADC12CTL0 = ADC12SHT0_2 | // 16 ciclos de clock para o sampling time
                ADC12SHT1_2 | // 16 ciclos de clock para o sampling time
                ADC12ON; // Liga o ADC

    ADC12CTL1 = ADC12CSTARTADD_0 | // Start address 0
                ADC12SHS_1 | // Conversão via TA0.1
                ADC12DIV_0 | // Divisor de clock: 1
                ADC12SSEL_0 | // MODCLK (4.8MHz)
                ADC12CONSEQ_1; // Sequência de canais sem repetição

    ADC12CTL2 = ADC12TCOFF | // Desliga o sensor de temperatura
                ADC12RES_0;  // Resolução de 8 bits


    //Configurações dos canais
    ADC12MCTL0 = ADC12SREF_0 | ADC12INCH_0 ;
    ADC12MCTL1 = ADC12SREF_0 | ADC12INCH_1 | ADC12EOS;

    // Habilita interrupção no ADC12MEM1
    ADC12IE = ADC12IE1;

    // Habilita o módulo ADC após configurações
    ADC12CTL0 |= ADC12ENC;
}

// INTERRUPÇÕES
#pragma vector = ADC12_VECTOR
__interrupt void __adc12_interrupt(void)
{
    switch(__even_in_range(ADC12IV,0x24)) {
        case 0x8:
            // ADC12MEM1 preenchida

            // Ler valores preenchidos
            diff = ADC12MEM0 - ADC12MEM1;

            if (diff > 2) {
                RED_LED_ON;
                GREEN_LED_OFF;
            } else if (diff < -2) {
                RED_LED_OFF;
                GREEN_LED_ON;
            } else {
                RED_LED_ON;
                GREEN_LED_ON;
            }

            // Reabilita módulo para próxima leitura
            ADC12CTL0 |= ADC12ENC;
            break;
        default:
            // Fazer nada
            break;
    }
}
