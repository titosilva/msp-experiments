#include <msp430.h> 

volatile unsigned int measurements[4];

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

    TA0CCR0 = 8192; // ACLK / 4 ~ 4Hz
    TA0CCR1 = TA0CCR0 >> 1; // 50% duty cycle
}

void configure_adc()
{
    // Desabilita o módulo ADC para configurações
    ADC12CTL0 &= ~ADC12ENC;

    // Pinos 6.3, 6.2, 6.1 e 6.0 como entradas analógicas
    P6SEL |= BIT3 | BIT2 | BIT1 | BIT0;

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
                ADC12CONSEQ_3; // Sequência de canais com repetição

    ADC12CTL2 = ADC12TCOFF | // Desliga o sensor de temperatura
                ADC12RES_0;  // Resolução de 8 bits


    //Configurações dos canais
    ADC12MCTL0 = ADC12SREF_0 | ADC12INCH_0;
    ADC12MCTL1 = ADC12SREF_0 | ADC12INCH_0;
    ADC12MCTL2 = ADC12SREF_0 | ADC12INCH_0;
    ADC12MCTL3 = ADC12SREF_0 | ADC12INCH_0;
    ADC12MCTL4 = ADC12SREF_0 | ADC12INCH_1;
    ADC12MCTL5 = ADC12SREF_0 | ADC12INCH_1;
    ADC12MCTL6 = ADC12SREF_0 | ADC12INCH_1;
    ADC12MCTL7 = ADC12SREF_0 | ADC12INCH_1;
    ADC12MCTL8 = ADC12SREF_0 | ADC12INCH_2;
    ADC12MCTL9 = ADC12SREF_0 | ADC12INCH_2;
    ADC12MCTL10 = ADC12SREF_0 | ADC12INCH_2;
    ADC12MCTL11 = ADC12SREF_0 | ADC12INCH_2;
    ADC12MCTL12 = ADC12SREF_0 | ADC12INCH_3;
    ADC12MCTL13 = ADC12SREF_0 | ADC12INCH_3;
    ADC12MCTL14 = ADC12SREF_0 | ADC12INCH_3;
    ADC12MCTL15 = ADC12SREF_0 | ADC12INCH_3 | ADC12EOS;

    // Habilita interrupção na última conversão
    ADC12IE = ADC12IE15;

    // Habilita o módulo ADC após configurações
    ADC12CTL0 |= ADC12ENC;
}

// INTERRUPÇÕES
#pragma vector = ADC12_VECTOR
__interrupt void __adc12_interrupt(void)
{
    switch(__even_in_range(ADC12IV,0x24)) {
        case ADC12IV_NONE:
                break;
        case ADC12IV_ADC12OVIFG:    //MEMx overflow
            break;
        case ADC12IV_ADC12TOVIFG:  //Conversion Time overflow
            break;
        case ADC12IV_ADC12IFG0:       //MEM0 Ready
            break;
        case ADC12IV_ADC12IFG1:      //MEM1 Ready
            break;
        case ADC12IV_ADC12IFG2:      //MEM2 Ready
            break;
        case ADC12IV_ADC12IFG3:      //MEM3 Ready
            break;
        case ADC12IV_ADC12IFG4:      //MEM4 Ready
            break;
        case ADC12IV_ADC12IFG5:      //MEM5 Ready
            break;
        case ADC12IV_ADC12IFG6:      //MEM6 Ready
            break;
        case ADC12IV_ADC12IFG7:      //MEM7 Ready
            break;
        case ADC12IV_ADC12IFG8:      //MEM8 Ready
            break;
        case ADC12IV_ADC12IFG9:      //MEM9 Ready
            break;
        case ADC12IV_ADC12IFG10:      //MEM10 Ready
            break;
        case ADC12IV_ADC12IFG11:      //MEM11 Ready
            break;
        case ADC12IV_ADC12IFG12:      //MEM12 Ready
            break;
        case ADC12IV_ADC12IFG13:      //MEM13 Ready
            break;
        case ADC12IV_ADC12IFG14:      //MEM14 Ready
            break;
        case ADC12IV_ADC12IFG15:
            // ADC12MEM15 preenchida

            // Ler valores preenchidos
            measurements[0] = (ADC12MEM0 + ADC12MEM1 + ADC12MEM2 + ADC12MEM3) >> 2;
            measurements[1] = (ADC12MEM4 + ADC12MEM5 + ADC12MEM6 + ADC12MEM7) >> 2;
            measurements[2] = (ADC12MEM8 + ADC12MEM9 + ADC12MEM10 + ADC12MEM11) >> 2;
            measurements[3] = (ADC12MEM12 + ADC12MEM13 + ADC12MEM14 + ADC12MEM15) >> 2;
            break;
        default:
            // Fazer nada
            break;
    }
}
