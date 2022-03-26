#include <msp430.h> 

#define bool int
#define true 1
#define false 0

// - UCB0 é configurado como MASTER TRANSMITTER/RECEIVER
// - UCB1 é configurado como SLAVE TRANSMITTER/RECEIVER
//Hardware setup:
// - P3.0 - P4.1 - SDA
// - P3.1 - P4.2 - SCL
//A UCB1 (disponibilizada) não liga os resistores de Pull-Up. Eles devem ser ligados na UCB0 ou devem ser adicionados externamente.

#define LED1_ON             (P1OUT |= BIT0)
#define LED1_OFF           (P1OUT &= ~BIT0)
#define LED1_TOGGLE   (P1OUT ^= BIT0)

#define LED2_ON              (P4OUT |= BIT7)
#define LED2_OFF            (P4OUT &= ~BIT7)
#define LED2_TOGGLE    (P4OUT ^= BIT7)

#define UCB1_ADDRESS 0x31
#define UCB1_BYTE 0x44
void initialize_I2C_UCB1_Slave();
unsigned char ucb1_byteReceived;
unsigned char ucb1_byteTransmitted;

volatile unsigned char ucb0_test_address;
volatile unsigned char ucb0_data_received;
bool ucb0_data_received_flag;
void reset_vars();
void configure_I2C_UCB0_Master(bool transmitter_mode);
bool master_TransmitOneByte(unsigned char address, unsigned char data);
void master_ReceiveOneByte(unsigned char address);
void set_address_and_send_start_UCB0(unsigned char address);

void delay_us(unsigned int time_us);

void configLED1()
{
    P1SEL &= ~BIT0;
    P1DIR |= BIT0;
    P1OUT &= ~BIT0;
}

void configLED2()
{
    P4SEL &= ~BIT7;
    P4DIR |= BIT7;
    LED2_OFF;
}

/**
 * main.c
 */
int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    configLED1();
    configLED2();

    initialize_I2C_UCB1_Slave();
    configure_I2C_UCB0_Master(true);
    reset_vars();

    volatile unsigned int status = 0;
    __enable_interrupt();

    status = 0;
    delay_us(50000);

    //Fase 1: Descobrir o endereço

    while(!master_TransmitOneByte(ucb0_test_address, 0x1))
    {
        ucb0_test_address++;
    }

    delay_us(50000);
    status = 1; //BREAKPOINT

    while(1)
    {
        ucb0_data_received_flag = false;
        configure_I2C_UCB0_Master(false);
        //Fase 2:Descobrir o byte certo
        //Peço para receber um byte
        master_ReceiveOneByte(ucb0_test_address);

        while (!ucb0_data_received_flag);

        delay_us(50000);
        status = 2; //BREAKPOINT

        //Fase 3: Envio o byte correto.
        delay_us(50000);
        status = 3; //BREAKPOINT
    }

    return 0;
}


void reset_vars()
{
    ucb0_test_address = 0x01;
    ucb0_data_received = 0x00;
    ucb0_data_received_flag = 0x00;
}

/*
 *  P3.0 - SDA
 *  P3.1 - SCL
 */
void configure_I2C_UCB0_Master(bool transmitter_mode) {
    // Turn off the module
       UCB0CTL1 = UCSWRST;

       // Configure the pins to use the module instead of GPIO
       P3SEL |= BIT0;
       P3SEL |= BIT1;
       // Enable resistors
       P3REN |= BIT0;
       P3REN |= BIT1;
       P3OUT |= BIT0;
       P3OUT |= BIT1;

       UCB0CTL0 |= UCMST // Master
                   | UCMODE_3 // Configure UCB0 to use I2C
                   | UCSYNC; // Synchronous mode

       // Use ACLK as clock source
       if (transmitter_mode) {
           UCB0CTL1 = UCSSEL__ACLK | UCTR | UCSWRST;
       } else {
           UCB0CTL1 = UCSSEL__ACLK | UCSWRST;
       }

       // Baud rate clock divisor
       UCB0BR0 = 2;
       UCB0BR1 = 0;

       // Turn on the module
       UCB0CTL1 &= ~UCSWRST;

       if (transmitter_mode) {
           UCB0IE = 0;
       } else {
           // Enable RX interrupts
           UCB0IE = UCNACKIE | UCRXIE;
       }
}

void set_address_and_send_start_UCB0(unsigned char address) {
    // Set the address
    UCB0I2CSA = address;

    // Waits for the line to be idle
    while (UCB0STAT & UCBBUSY);

    // Send a START
    UCB0CTL1 |= UCTXSTT;
}

bool master_TransmitOneByte(unsigned char address, unsigned char data)
{
        set_address_and_send_start_UCB0(address);

        // Wait until the TX buffer is available
        while ((UCB0IFG & UCTXIFG) == 0);

        // Send the data
        UCB0TXBUF = data;

        // Wait for ACK
        while (UCB0CTL1 & UCTXSTT);

        volatile bool is_ack = ((UCB0IFG & UCNACKIFG) == 0);
        UCB0CTL1 |= UCTXSTP;

        return is_ack;
}

void master_ReceiveOneByte(unsigned char address)
{
        set_address_and_send_start_UCB0(address);

        // The other actions will happen inside interrupts
        return;
}

#pragma vector = USCI_B0_VECTOR;
__interrupt void i2c_b0_isr() {
    switch (__even_in_range(UCB0IV,12)) {
    case USCI_NONE:
         break;
     case USCI_I2C_UCALIFG:
         break;
     case USCI_I2C_UCNACKIFG:
         break;
     case USCI_I2C_UCSTTIFG:
         break;
     case USCI_I2C_UCSTPIFG:
         break;
    case USCI_I2C_UCRXIFG:
        ucb0_data_received = (unsigned char) UCB0RXBUF;
        ucb0_data_received_flag = true;
        break;
    default:
        break;
    }
}

//*********************************************************
//AS FUNÇÕES ABAIXO NÃO DEVEM SER ALTERADAS!
//*********************************************************

/*
 * Delay microsseconds.
 */
void delay_us(unsigned int time_us)
{
    //Configure timer A0 and starts it.
    TA0CCR0 = time_us;
    TA0CTL = TASSEL__SMCLK | ID__1 | MC_1 | TACLR;

    //Locks, waiting for the timer.
    while((TA0CTL & TAIFG) == 0);

    //Stops the timer
    TA0CTL = MC_0 | TACLR;
}

/*
 *  P4.1 - SDA
 *  P4.2 - SCL
 */
void initialize_I2C_UCB1_Slave()
{
    //Desliga o módulo
    UCB1CTL1 |= UCSWRST;

    //Configura os pinos
    P4SEL |= BIT1;     //Configuro os pinos para "from module"
    P4SEL |= BIT2;
    P4REN &= ~BIT1; //Não configura resistores
    P4REN &= ~BIT2;


    UCB1CTL0 =  UCMODE_3 |    //I2C Mode
                           UCSYNC;         //Synchronous Mode

    UCB1CTL1 = UCSSEL__ACLK |    //Clock Source: ACLK
                           UCSWRST ;             //Mantém o módulo desligado

    //Divisor de clock para o BAUDRate
    UCB1BR0 = 2;
    UCB1BR1 = 0;

    //Prepara minhas variáveis.
    ucb1_byteTransmitted = UCB1_BYTE;
    ucb1_byteReceived = 0x00;

    UCB1I2COA = UCB1_ADDRESS;

    //Liga o módulo.
    UCB1CTL1 &= ~UCSWRST;

    //Liga as interrupções de TX e RX.
    UCB1IE = UCTXIE | UCRXIE;

}


#pragma vector = USCI_B1_VECTOR;
__interrupt void i2c_b1_isr()
{
    switch (__even_in_range(UCB1IV,12)) {
    case USCI_NONE:
        break;
    case USCI_I2C_UCALIFG:
        break;
    case USCI_I2C_UCNACKIFG:
        break;
    case USCI_I2C_UCSTTIFG:
        break;
    case USCI_I2C_UCSTPIFG:
        break;
    case USCI_I2C_UCRXIFG:
        ucb1_byteReceived = (unsigned char) UCB1RXBUF;
        if (ucb1_byteReceived == ucb1_byteTransmitted)
        {
            LED1_OFF;
            LED2_ON;
        } else
        {
            LED1_ON;
            LED2_OFF;
        }
        //Toda vez que recebe um byte ele muda a senha
        ucb1_byteTransmitted += 0x03;
        break;
    case USCI_I2C_UCTXIFG:
        UCB1TXBUF = ucb1_byteTransmitted;
        break;

    default:
        break;
    }

    UCB1IV = USCI_NONE;
}
