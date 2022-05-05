#include <msp430.h> 

#define TOGGLE_RED_LED (P1OUT ^= BIT0)
#define TOGGLE_GREEN_LED (P4OUT ^= BIT7)

#define PASSWORD_LEN 5

#define FLLN(x) ((x)-1)
void clockInit();
void pmmVCore (unsigned int level);

void configSPIUCB1();
void configSPIUCB0ForTest();
void configButton();
void configLeds();

volatile int actionFlag = 0;
volatile char password[PASSWORD_LEN];
volatile char password_idx = -1;
volatile char sending_password = 0;

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    clockInit();

    //Código do aluno vem aqui.
    configSPIUCB1();
    configSPIUCB0ForTest();

    configButton();
    configLeds();
    __enable_interrupt();


    volatile char received_byte;
    while (1) {
        // Iniciar comunicação
        UCB1TXBUF = 0x01;

        while(!(UCB1IFG & UCRXIFG));

        received_byte = UCB1RXBUF;

        if (sending_password) {

            UCB1TXBUF = password[password_idx];

            if (received_byte == 0x02) {
                password_idx++;
            }

            if(password_idx == PASSWORD_LEN) {
                sending_password = 0;
                password_idx = -1;
            }
        }

        if (password_idx >= 0) {
            password[password_idx] = received_byte;
        }
        password_idx++;
        UCB1TXBUF = 0x01;

        if (password_idx == PASSWORD_LEN) {
            sending_password = 1;
            password_idx = 0;
        }
    }

    while(1);

    return 0;
}

void configSPIUCB1()
{
    UCB1CTL1 = UCSWRST; // Desliga o módulo
    UCB1CTL1 |= UCMODE_0 | UCSYNC; // SPI 3 pinos, síncrono
    UCB1CTL0 |= UCMSB; // MSB First

    // Configurar como master
    UCB1CTL1 |= UCSSEL__ACLK;
    UCB1CTL0 |= UCMST;
    UCB1BR0 = 0;
    UCB1BR1 = 0;

    // Pinos direcionados para o módulo
    P4SEL |= BIT1 | BIT2 | BIT3;

    UCB1CTL1 &= ~UCSWRST; // Liga o módulo novamente
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

void configLeds()
{
    P4SEL &= ~BIT7;
    P4DIR |= BIT7;
    P4OUT &= ~BIT7;

    P1SEL &= ~BIT0;
    P1DIR |= BIT0;
    P1OUT |= BIT0;
}

// INTERRUPÇÕES ==================================================================
volatile int debouncing = 0;
#pragma vector = PORT2_VECTOR
__interrupt void __p2_interrupt_handle(void)
{
    switch(P2IV) {
    case P2IV_P2IFG1:
        actionFlag = 1;
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

// FUNÇÕES DE TESTE
volatile char __test_password[PASSWORD_LEN] = "12345";
volatile int __test_password_idx = 0;
volatile char __recv_password[PASSWORD_LEN];
volatile int __recv_password_idx = 0;
volatile char __recv_byte;

void configSPIUCB0ForTest()
{
    UCB0CTL1 = UCSWRST; // Desliga o módulo
    UCB0CTL1 |= UCMODE_0 | UCSYNC; // SPI 3 pinos, síncrono
    UCB0CTL0 |= UCMSB; // MSB First

    // Pinos direcionados para o módulo
    P3SEL |= BIT0 | BIT1 | BIT2;

    UCB0CTL1 &= ~UCSWRST; // Liga o módulo novamente

    UCB0IE = UCRXIE;
}

#pragma vector = USCI_B0_VECTOR
__interrupt void __ucb0_interrupt_handle()
{
    switch (_even_in_range(UCB0IV, 0x04))
    {
    case USCI_NONE:
        break;
    case USCI_UCRXIFG:
        __recv_byte = UCB0RXBUF;
        if (__recv_password_idx > 0)
        {
            __recv_password[__recv_password_idx] = __recv_byte;
            __recv_password_idx++;

            if (__recv_password_idx == PASSWORD_LEN)
            {
                volatile int i = 0;
                volatile int correct_password = 1;
                for (i = 0; i < PASSWORD_LEN; i++)
                {
                    if (__recv_password[i] != __test_password[i])
                    {
                        correct_password = 0;
                        break;
                    }
                }

                if (correct_password)
                {
                    TOGGLE_GREEN_LED;
                    TOGGLE_RED_LED;
                }
                __recv_password_idx = 0;
            } else {
                UCB0TXBUF = 0x02;
            }

            break;
        }

        if (__recv_byte == __test_password[0])
        {
            __recv_password_idx = 1;
            __recv_password[0] = __test_password[0];
            UCB0TXBUF = 0x02;
            break;
        }

        volatile char to_send = __test_password[__test_password_idx];
        UCB0TXBUF = to_send;
        __test_password_idx++;
        if(__test_password_idx == PASSWORD_LEN) {
            __test_password_idx = 0;
        }
        break;
    case USCI_UCTXIFG:
        break;
    default:
        break;
    }
}
