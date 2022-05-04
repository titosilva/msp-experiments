#include <msp430.h> 
#include <inttypes.h>

#define LCD_ADDRESS 0x27
#define MSP_ADDRESS 0x42

#define FLLN(x) ((x)-1)
void clockInit();
void pmmVCore (unsigned int level);

#define to_high_nibble(b) ((b & 0xf) << 4)
#define to_low_nibble(b) ((b & 0xf))

typedef uint8_t bool;
const bool true = 1;
const bool false = 0;

typedef uint8_t byte;

typedef struct {
    bool backlight_on;
    byte address;
    byte buffer[2][16];
} LCD;

#define LCD_ENABLE_BIT BIT2
#define LCD_BACKLIGHT_BIT BIT3
#define LCD_SET_DDRAM_ADDRESS_BIT BIT7

// Low level I2C
void initialize_I2C_UCB0_MasterTransmitter();
void initialize_I2C_UCB1_SlaveReceiver();
void master_TransmitOneByte(unsigned char address, unsigned char data);
void delay_us(unsigned int time_us);

// Low level LCD
LCD initialize_lcd(byte address);
byte assemble_byte_from_lcd_nibble(LCD* lcd, byte nibble, bool is_instruction);
void lcd_send_raw_byte(LCD* lcd, byte b);
void lcd_send_nibble(LCD* lcd, byte nibble, bool is_instruction);
void lcd_send_byte(LCD* lcd, byte b, bool is_instruction);

// High level LCD
void lcd_set_cursor_position(LCD* lcd, byte row, byte col);
void lcd_write_char(LCD* lcd, byte c);
void lcd_clear(LCD* lcd);

// LCD Buffer
#define LCD_EMPTY_CHAR (0x20)
void lcd_flush(LCD* lcd);


int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    clockInit();

    initialize_I2C_UCB0_MasterTransmitter();
    initialize_I2C_UCB1_SlaveReceiver();

    LCD lcd = initialize_lcd(LCD_ADDRESS);
    volatile int line = 0;
    volatile int col = 0;

    while(1) {
        // Esperar receber um byte
        while(!(UCB1IFG & UCRXIFG));

        // Escreve o byte no LCD
        lcd.buffer[line][col] = UCB1RXBUF;

        col = (col + 1) % 16;
        if (col == 0) {
            line = (line + 1) % 2;
        }

        lcd_flush(&lcd);
    }

    return 0;
}


/*
 *  P3.0 - SDA
 *  P3.1 - SCL
 */
void initialize_I2C_UCB0_MasterTransmitter()
{
    //Desliga o módulo
    UCB0CTL1 |= UCSWRST;

    //Configura os pinos
    P3SEL |= BIT0;     //Configuro os pinos para "from module"
    P3SEL |= BIT1;
    P3REN &= ~BIT0; //Resistores externos.
    P3REN &= ~BIT1;


    UCB0CTL0 = UCMST |           //Master Mode
               UCMODE_3 |    //I2C Mode
               UCSYNC;         //Synchronous Mode

    UCB0CTL1 =  UCSSEL__SMCLK |    //Clock Source
                UCTR |                      //Transmitter
                UCSWRST ;             //Mantém o módulo desligado

    //Divisor de clock para o BAUDRate
    UCB0BR0 = 2;
    UCB0BR1 = 0;

    //Liga o módulo.
    UCB0CTL1 &= ~UCSWRST;
}

void initialize_I2C_UCB1_SlaveReceiver()
{
    //Desliga o módulo
    UCB1CTL1 = UCSWRST;

    //Configura os pinos
    P4SEL |= BIT1 | BIT2;
    P4REN &= ~BIT1; // Resistores externos.
    P4REN &= ~BIT2;

    UCB1CTL0 = UCMODE_3 |    //I2C Mode
               UCSYNC;       //Synchronous Mode

    UCB1CTL1 = UCSSEL__ACLK | UCSWRST;

    //Divisor de clock para o BAUDRate
    UCB1BR0 = 2;
    UCB1BR1 = 0;

    // Configura o endereço
    UCB1I2COA = MSP_ADDRESS;

    //Liga o módulo.
    UCB1CTL1 &= ~UCSWRST;
}

LCD initialize_lcd(byte address)
{
    LCD lcd;
    lcd.address = LCD_ADDRESS;
    lcd.backlight_on = true;

    lcd_send_nibble(&lcd, 0x03, true);
    lcd_send_nibble(&lcd, 0x03, true);
    lcd_send_nibble(&lcd, 0x03, true);
    lcd_send_nibble(&lcd, 0x02, true);

    lcd_send_byte(&lcd, 0x28, true);
    lcd_send_byte(&lcd, 0x08, true);
    lcd_clear(&lcd);
    lcd_send_byte(&lcd, 0x06, true);
    lcd_send_byte(&lcd, 0x0F, true);

    return lcd;
}

byte assemble_byte_from_lcd_nibble(LCD* lcd, byte nibble, bool is_instruction)
{
    volatile byte result = is_instruction? 0x00 : 0x01;
    result |= to_high_nibble(nibble);
    result |= lcd->backlight_on? LCD_BACKLIGHT_BIT : 0;
    return result;
}

void lcd_send_nibble(LCD* lcd, byte nibble, bool is_instruction)
{
    volatile byte byte_to_send = assemble_byte_from_lcd_nibble(lcd, nibble, is_instruction);
    lcd_send_raw_byte(lcd, byte_to_send);
}

void lcd_send_byte(LCD* lcd, byte b, bool is_instruction)
{
    lcd_send_nibble(lcd, (b >> 4) & 0xf, is_instruction);
    lcd_send_nibble(lcd, b & 0xf,      is_instruction);
}

void lcd_send_raw_byte(LCD* lcd, byte b)
{
    master_TransmitOneByte(lcd->address, b);
    delay_us(1000);
    master_TransmitOneByte(lcd->address, b | LCD_ENABLE_BIT);
    delay_us(2000);
    master_TransmitOneByte(lcd->address, b);
    delay_us(1000);
}

void lcd_set_cursor_position(LCD* lcd, byte row, byte col)
{
    lcd_send_byte(lcd, (row * 0x40 + col) | LCD_SET_DDRAM_ADDRESS_BIT, true);
}

void lcd_write_char(LCD* lcd, byte c)
{
    lcd_send_byte(lcd, c, false);
}

void lcd_clear(LCD* lcd)
{
    volatile int i, j;
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 16; j++) {
            lcd->buffer[i][j] = LCD_EMPTY_CHAR;
        }
    }

    lcd_send_byte(lcd, 0x1, true);
}

void lcd_flush(LCD* lcd)
{
    volatile int i, j;
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 16; j++) {
            lcd_set_cursor_position(lcd, i, j);
            lcd_write_char(lcd, lcd->buffer[i][j]);
        }
    }
}

void master_TransmitOneByte(unsigned char address, unsigned char data)
{
    //Desligo todas as interrupções
    UCB0IE = 0;

    //Coloco o slave address
    UCB0I2CSA = address;

    //Espero a linha estar desocupada.
    if (UCB0STAT & UCBBUSY) return;

    //Peço um START
    UCB0CTL1 |= UCTXSTT;

    //Espero até o buffer de transmissão estar disponível
    while ((UCB0IFG & UCTXIFG) == 0);

    //Escrevo o dado
    UCB0TXBUF = data;

    //Aguardo o acknowledge
    while (UCB0CTL1 & UCTXSTT);

    UCB0CTL1 |= UCTXSTP;

    return;
}

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


// FUNÇÕES INALTERADAS ============================================
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
