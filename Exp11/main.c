#include <msp430.h> 

#define FLLN(x) ((x)-1)
void clockInit();
void pmmVCore (unsigned int level);

void configSPIUCB1();
void configButton();

volatile int actionFlag = 0;
volatile char password[5];

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    clockInit();

    //Código do aluno vem aqui.
    configSPIUCB1();
    configButton();
    __enable_interrupt();

    while(true) {
        // Esperar até poder realizar as ações
        while(!actionFlag);

        // A senha será introduzida por um 0
        while(!(UCB1CTL0 & UCRXIFG) && UCB1RXBUF != 0);

        // Fazer leitura da senha
        for (int i = 0; i < 5; i++) {
            while(!(UCB1CTL0 & UCRXIFG));
            password[i] = UCB1RXBUF;
        }

        // Enviar senha
        for (int i = 0; i < 5; i++) {
            while(!(UCB1CTL0 & UCTXIFG));
            UCB1TXBUF = password[i];
        }

        actionFlag = 0;
    }

    return 0;
}

void configSPIUCB1()
{
    UCB1CTL1 = UCSWRST; // Desliga o módulo
    UCB1CTL1 = UCMODE_0 | UCSYNC; // SPI 3 pinos, síncrono
    UCB1CTL0 |= UCMSB; // MSB First

    // Configurar como master
    UCB1CTL1 |= UCSSEL__SMCLK;
    UCB1CTL0 |= UCMST;

    // Pinos direcionados para o módulo
    P4SEL |= BIT1 | BIT2 | BIT3;

    UCB1CTL1 &= ~UCSWRST; // Liga o módulo novamente
    // Sem interrupções (será usado pooling)
}

void configButton()
{
    // Input com PullUp
    P2SEL &= ~BIT1;
    P2DIR &= ~BIT1;
    P2REN |= BIT1;
    P2OUT |= BIT1;
    // Habilitar interrupção
    P2IE |= BIT1;
}

// INTERRUPÇÕES ==================================================================
#pragma vector = PORT2_VECTOR
__interrupt void __p2_interrupt_handle(void)
{
    switch(P2IV) {
    case P2IV_P2IFG1:
        actionFlag = 1;

        // Debounce
        volatile int x = 50000;
        while(--x);

        break;
    default:
        break;
    }
}

// FUNÇÕES INALTERADAS ===========================================================
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
