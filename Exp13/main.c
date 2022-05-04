#include <msp430.h> 

#define SAMPLE_SIZE 128

#define RED_LED_OFF (P1OUT &= ~BIT0)
#define RED_LED_ON (P1OUT |= BIT0)
#define GREEN_LED_OFF (P4OUT &= ~BIT7)
#define GREEN_LED_ON (P4OUT |= BIT7)

volatile unsigned int vector1[SAMPLE_SIZE];
volatile unsigned int vector2[SAMPLE_SIZE];

#define FLLN(x) ((x)-1)
void clockInit();
void pmmVCore (unsigned int level);


void configure_leds();
void configure_adc_trigger_clk();
void configure_adc();
void configure_dma1();
void configure_dma2();

void update_leds_according_to_vec(int* vector);

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    clockInit();

    configure_leds();
    configure_adc_trigger_clk();
    configure_adc();
    configure_dma1();
    configure_dma2();

    // Ativa transferência no DMA1
    DMA1CTL |= DMAEN;
    while(1) {
        // Aguarda fim da transferência no DMA1
        while (!(DMA1CTL & DMAIFG));
        // Ativa transferência no DMA2
        DMA2CTL &= ~DMAIFG;
        DMA2CTL |= DMAEN;
        update_leds_according_to_vec(vector1);


        // Aguarda fim da transferência no DMA2
        while (!(DMA2CTL & DMAIFG));
        // Ativa transferência no DMA1
        DMA1CTL &= ~DMAIFG;
        DMA1CTL |= DMAEN;
        update_leds_according_to_vec(vector2);
    }

    return 0;
}

void update_leds_according_to_vec(int* vector)
{
    volatile long int avg = 0;
    volatile int i = 0;
    for (i = 0; i < SAMPLE_SIZE; i++)
    {
        avg += vector[i];
    }

    avg = avg >> 7; // Divide por 128

    RED_LED_OFF;
    GREEN_LED_OFF;

    // Faz o led piscar para ajudar a conferir o tempo
    // para as amostras
//    volatile int x = 30000;
//    while(--x);

    if (avg <= 1000) {
        RED_LED_ON;
    } else if (avg <= 3000) {
        RED_LED_ON;
        GREEN_LED_ON;
    } else {
        GREEN_LED_ON;
    }
}

void configure_leds()
{
    P1SEL &= ~BIT0;
    P1DIR |= BIT0;

    P4SEL &= ~BIT7;
    P4DIR |= BIT7;
}

void configure_dma1()
{
    DMACTL0 |= DMA1TSEL_24; // Disparo baseado no ADC12

    // Operar com words
    DMA1CTL = DMADT_0 | // Modo Simples com repetição
             DMASRCINCR_0 | // Fonte fixa
             DMADSTINCR_3 ; // Destino incrementando

    // Fonte no ADC MEM0
    DMA1SA = &ADC12MEM0;

    // DMA1 -> vector1
    DMA1DA = vector1;

    DMA1SZ = SAMPLE_SIZE; // Quantidade de transferências
}

void configure_dma2()
{
    DMACTL1 |= DMA2TSEL_24; // Disparo baseado no ADC12

    // Operar com words
    DMA2CTL = DMADT_0 | // Modo Simples com repetição
             DMASRCINCR_0 | // Fonte fixa
             DMADSTINCR_3 ; // Destino incrementando

    // Fonte no ADC MEM0
    DMA2SA = &ADC12MEM0;

    // DMA2 -> vector2
    DMA2DA = vector2;

    DMA2SZ = SAMPLE_SIZE; // Quantidade de transferências
}

void configure_adc_trigger_clk()
{
    TA0CTL = TASSEL__ACLK | MC_1;
    TA0CCTL1 = OUTMOD_6;

    TA0CCR0 = 256; // ACLK / 128 ~ 128Hz
    TA0CCR1 = TA0CCR0 >> 1; // 50% duty cycle
}

void configure_adc()
{
    // Desabilita o módulo ADC para configurações
    ADC12CTL0 &= ~ADC12ENC;

    // Pinos 6.1 como entrada analógica
    P6SEL |= BIT1;

    // Cálculo do tempo de amostragem, considerando n = 8
    // Tamost > (Rs + Rin) * Cin * (n+1) * ln(2) + 800ns
    // Tamost > (10k + 1.8k) * 25p * (12+1) * ln(2) + 800ns
    // Tamost > 11.8 * 25 * 13 * ln(2) * 10^(-9) + 800 * 10^(-9)
    // Tamost > 3458,219 * 10^(-9)
    // Tamost > 3,46 us

    // Considerando um ADC12CLK de 5MHz, temos que o Tclk = 0,2us
    // Assim, tomando K como o número de ciclos de ADC12CLK necessários para o sampling time,
    // Tamost > 3,46us
    // K * 0,2us > 3,46us
    // K > 17
    // Portanto, como a quantidade de ciclos de clock deve ser um valor de bits no SHT do A/D válido,
    // K = 32

    ADC12CTL0 = ADC12SHT0_3 | // 32 ciclos de clock para o sampling time
                ADC12SHT1_3 | // 32 ciclos de clock para o sampling time
                ADC12ON; // Liga o ADC

    ADC12CTL1 = ADC12CSTARTADD_0 | // Start address 0
                ADC12SHS_1 | // Conversão via TA0.1
                ADC12DIV_0 | // Divisor de clock: 1
                ADC12SSEL_0 | // MODCLK (4.8MHz)
                ADC12CONSEQ_2; // Um canal com repetição

    // Usando o modo de um canal com repetição para manter o ADC12 sempre habilitado

    ADC12CTL2 = ADC12TCOFF | // Desliga o sensor de temperatura
                ADC12RES_2;  // Resolução de 12 bits


    //Configurações dos canais
    ADC12MCTL0 = ADC12SREF_0 | ADC12INCH_1 | ADC12EOS;

    // Habilita o módulo ADC após configurações
    ADC12CTL0 |= ADC12ENC;
}

// FUNÇÕES INALTERADAS ======================================================
void pmmVCore (unsigned int level)
{
#if defined (__MSP430F5529__)
    PMMCTL0_H = 0xA5;                       // Open PMM registers for write access

    SVSMHCTL =                              // Set SVS/SVM high side new level
            SVSHE            +
            SVSHRVL0 * level +
            SVMHE            +
            SVSMHRRL0 * level;

    SVSMLCTL =                              // Set SVM low side to new level
            SVSLE            +
//          SVSLRVL0 * level +              // but not SVS, not yet..
            SVMLE            +
            SVSMLRRL0 * level;

    while ((PMMIFG & SVSMLDLYIFG) == 0);    // Wait till SVM is settled

    PMMIFG &= ~(SVMLVLRIFG + SVMLIFG);      // Clear already set flags

    PMMCTL0_L = PMMCOREV0 * level;          // Set VCore to new level

    if ((PMMIFG & SVMLIFG))                 // Wait till new level reached
        while ((PMMIFG & SVMLVLRIFG) == 0);

    SVSMLCTL =                              // Set SVS/SVM low side to new level
            SVSLE            +
            SVSLRVL0 * level +
            SVMLE            +
            SVSMLRRL0 * level;

    PMMCTL0_H = 0x00;                       // Lock PMM registers for write access
#endif
}

void clockInit()
{
    pmmVCore(1);
    pmmVCore(2);
    pmmVCore(3);

    P5SEL |= BIT2 | BIT3 | BIT4 | BIT5;
    UCSCTL0 = 0x00;
    UCSCTL1 = DCORSEL_5;
    UCSCTL2 = FLLD__1 | FLLN(25);
    UCSCTL3 = SELREF__XT2CLK | FLLREFDIV__4;
    UCSCTL6 = XT2DRIVE_2 | XT1DRIVE_2 | XCAP_3;
    UCSCTL7 = 0;                            // Clear XT2,XT1,DCO fault flags

    do {                                    // Check if all clocks are oscillating
      UCSCTL7 &= ~(   XT2OFFG |             // Try to clear XT2,XT1,DCO fault flags,
                    XT1LFOFFG |             // system fault flags and check if
                       DCOFFG );            // oscillators are still faulty
      SFRIFG1 &= ~OFIFG;                    //
    } while (SFRIFG1 & OFIFG);              // Exit only when everything is ok

    UCSCTL5 = DIVPA_1 | DIVA_0 | DIVM_0; //Divide ACLK por 2.

    UCSCTL4 = SELA__XT1CLK    |             // ACLK  = XT1   =>      16.384 Hz
              SELS__XT2CLK    |             // SMCLK = XT2   =>   4.000.000 Hz
              SELM__DCOCLK;                 // MCLK  = DCO   =>  25.000.000 Hz

}
