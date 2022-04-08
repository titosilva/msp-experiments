#include <msp430.h> 
#include <inttypes.h>

#define FLLN(x) ((x)-1)
void clockInit();
void pmmVCore (unsigned int level);

volatile uint16_t debouncing = 0;
#define DEBOUNCE debouncing = 50000
#define IS_DEBOUNCING (debouncing > 0)
#define DO_DEBOUNCING_STEP debouncing--

#define RED_LED_ON P1OUT |= BIT0
#define GREEN_LED_ON P4OUT |= BIT7
#define RED_LED_OFF P1OUT &= ~BIT0
#define GREEN_LED_OFF P4OUT &= ~BIT7
#define send_byte_uca0_sync(byte) while(!(UCA0IFG & UCTXIFG));UCA0TXBUF = (byte)


void initialize_uart_uca0();
void initialize_uart_uca1();
void initialize_leds_and_buttons();

// Primeiro byte usado para contagem
volatile uint8_t bytes_received[4] = { 0, 0, 0, 0 };

/**
 * main.c
 */
int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	
	clockInit();
	initialize_uart_uca0();
	initialize_uart_uca1();
	initialize_leds_and_buttons();

	__enable_interrupt();

	while(1) {
	    if (IS_DEBOUNCING) {
	        DO_DEBOUNCING_STEP;
	    }
	}

	return 0;
}

void initialize_uart_uca0()
{
    P3DIR |= BIT3;
    P3SEL |= BIT3;

    //Desliga o m�dulo
    UCA0CTL1 |= UCSWRST;

    UCA0CTL0 = UCPEN |    //Parity enable: 1=ON, 0=OFF
               //UCPAR |    //Parity: 0:ODD, 1:EVEN
               //UCMSB |    //LSB First: 0, MSB First:1
               //UC7BIT |   //8bit Data: 0, 7bit Data:1
               UCSPB |    //StopBit: 0:1 Stop Bit, 1: 2 Stop Bits
               UCMODE_0 | //USCI Mode: 00:UART, 01:Idle-LineMultiprocessor, 10:AddressLine Multiprocessor, 11: UART with automatic baud rate
               //UCSYNC    //0:Assynchronous Mode, 1:Synchronous Mode
               0;

    UCA0CTL1 = UCSSEL__SMCLK | //00:UCAxCLK, 01:ACLK, 10:SMCLK, 11:SMCLK
               //UCRXEIE     | //Erroneous Character IE
               //UCBRKIE     | //Break Character IE
               //UCDORM      | //0:NotDormant, 1:Dormant
               //UCTXADDR    | //Next frame: 0:data, 1:address
               //UCTXBRK     | //TransmitBreak
               UCSWRST;        //Mant�m reset.

    //BaudRate: 57600
    UCA0BR0 = 4; //Low byte
    UCA0BR1 = 0; //High byte
    UCA0MCTL = UCBRS0 |
               UCBRF2 | UCBRF0 |
               UCOS16 |
               0;

    //Liga o m�dulo
    UCA0CTL1 &= ~UCSWRST;

    UCA0IE =   //UCTXIE | //Interrupt on transmission
               //UCRXIE | //Interrupt on Reception
               0;
}

void initialize_uart_uca1()
{
    P4SEL |= BIT2; //Disponibilizar P4.2
    P4DIR &= ~BIT2;
    P4REN |= BIT2;
    P4OUT &= ~BIT2;
    PMAPKEYID = 0X02D52; //Liberar mapeamento de P4
    P4MAP2 = PM_UCA1RXD; //P4.2 = RXD

    //Desliga o m�dulo
    UCA1CTL1 |= UCSWRST;

    UCA1CTL0 = UCPEN |    //Parity enable: 1=ON, 0=OFF
               //UCPAR |    //Parity: 0:ODD, 1:EVEN
               //UCMSB |    //LSB First: 0, MSB First:1
               //UC7BIT |   //8bit Data: 0, 7bit Data:1
               UCSPB |    //StopBit: 0:1 Stop Bit, 1: 2 Stop Bits
               UCMODE_0 | //USCI Mode: 00:UART, 01:Idle-LineMultiprocessor, 10:AddressLine Multiprocessor, 11: UART with automatic baud rate
               //UCSYNC    //0:Assynchronous Mode, 1:Synchronous Mode
               0;

    UCA1CTL1 = UCSSEL__SMCLK | //00:UCAxCLK, 01:ACLK, 10:SMCLK, 11:SMCLK
               //UCRXEIE     | //Erroneous Character IE
               //UCBRKIE     | //Break Character IE
               //UCDORM      | //0:NotDormant, 1:Dormant
               //UCTXADDR    | //Next frame: 0:data, 1:address
               //UCTXBRK     | //TransmitBreak
               UCSWRST;        //Mant�m reset.

    //BaudRate: 57600

    UCA1BR0 = 4; //Low byte
    UCA1BR1 = 0; //High byte
    UCA1MCTL = UCBRS0 |
               UCBRF2 | UCBRF0 |
               UCOS16 |
               0;

    //Liga o m�dulo
    UCA1CTL1 &= ~UCSWRST;

    UCA1IE =   UCTXIE | //Interrupt on transmission
               UCRXIE |   //Interrupt on Reception
               0;
}

void initialize_leds_and_buttons()
{
    P1SEL &= ~BIT0;
    P1DIR |= BIT0;
    RED_LED_OFF;

    P4SEL &= ~BIT7;
    P4DIR |= BIT7;
    GREEN_LED_OFF;

    P2SEL &= ~BIT1;
    P2DIR &= ~BIT1;
    P2REN |= BIT1;
    P2OUT |= BIT1;

    P2IE |= BIT1; // Local interrupt enabled
    P2IES |= BIT1; // High-to-low transition

    do {
        P2IFG = 0;
    } while (P2IFG != 0);

    P1SEL &= ~BIT1;
    P1DIR &= ~BIT1;
    P1REN |= BIT1;
    P1OUT |= BIT1;

    P1IE |= BIT1; // Local interrupt enabled
    P1IES |= BIT1; // High-to-low transition

    do {
        P1IFG = 0;
    } while (P1IFG != 0);
}

// INTERRUPTS ========================================================================
#pragma vector = USCI_A0_VECTOR
__interrupt void UART0_INTERRUPT(void)
{
    switch(__even_in_range(UCA0IV, 4))
    {
    default:
        break;
    }
}

#pragma vector = USCI_A1_VECTOR
__interrupt void UART1_INTERRUPT(void)
{
    switch(__even_in_range(UCA1IV, 4))
    {
        case 2:
            bytes_received[bytes_received[0] + 1] = UCA1RXBUF;
            if (++bytes_received[0] == 3)
            {
                if (bytes_received[1] == 0x11 &&
                    bytes_received[2] == 0x22 &&
                    bytes_received[3] == 0x33) {
                    RED_LED_ON;
                } else if (bytes_received[1] == 0xAA &&
                    bytes_received[2] == 0xBB &&
                    bytes_received[3] == 0xCC) {
                    GREEN_LED_ON;
                } else {
                    RED_LED_OFF;
                    GREEN_LED_OFF;
                }

                bytes_received[0] = 0;
            }

            break;
        default:
            break;
    }
}

#pragma vector = PORT1_VECTOR;
__interrupt void __p1_interrupt_handle(void)
{
    switch (__even_in_range(P1IV, 0x10))
    {
        case P1IV_P1IFG1:
            if (IS_DEBOUNCING) {
                P1IFG = 0;
                return;
            }

            send_byte_uca0_sync(0xAA);
            send_byte_uca0_sync(0xBB);
            send_byte_uca0_sync(0xCC);
            DEBOUNCE;
            break;
        default:
            break;
    }

    P1IFG = 0;
}

#pragma vector = PORT2_VECTOR;
__interrupt void __p2_interrupt_handle(void)
{
    switch (__even_in_range(P2IV, 0x10))
    {
        case P2IV_P2IFG1:
            if (IS_DEBOUNCING) {
                P2IFG = 0;
                return;
            }

            send_byte_uca0_sync(0x11);
            send_byte_uca0_sync(0x22);
            send_byte_uca0_sync(0x33);
            DEBOUNCE;
            break;
        default:
            break;
    }

    P2IFG = 0;
}

// CÓDIGO NÃO MODIFICADO =============================================================
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
