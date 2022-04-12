#include <msp430.h>
#include <inttypes.h>

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

#define LCD_I2C_ADDRESS 0x27
#define LCD_ENABLE_BIT BIT2
#define LCD_BACKLIGHT_BIT BIT3
#define LCD_SET_DDRAM_ADDRESS_BIT BIT7

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

// - Esse código transmite 0x00 / 0xFF para o LCD/
// - UCB0 é configurado como MASTER TRANSMITTER
//Hardware setup:
// - P3.0 - SDA
// - P3.1 - SCL
// - Alimentar o LCD e conectar SDA / SCL
// - O código NÃO liga os resistores de pull-up internos, então se o LCD não tiver esses resistores eles tem que ser colocados na protoboard.


#define LED_RED_ON      (P1OUT |= BIT0)
#define LED_RED_OFF     (P1OUT &= ~BIT0)

#define LED_GREEN_ON      (P4OUT |= BIT7)
#define LED_GREEN_OFF     (P4OUT &= ~BIT7)

void initialize_I2C_UCB0_MasterTransmitter();
void master_TransmitOneByte(unsigned char address, unsigned char data);
void delay_us(unsigned int time_us);

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    initialize_I2C_UCB0_MasterTransmitter();

    //LED VERMELHO
    P1SEL &= ~BIT0;
    P1DIR |= BIT0;
    P1OUT &= ~BIT0;

    //LED VERDE
    P4SEL &= ~BIT7;
    P4DIR |= BIT7;
    P4OUT &= ~BIT7;

    LED_RED_OFF;
    LED_GREEN_OFF;

    LCD lcd = initialize_lcd(LCD_I2C_ADDRESS);

    // Write 'R'
    lcd.buffer[0][0] = 'B';
    lcd.buffer[0][2] = 'A';
    lcd.buffer[0][4] = 'T';
    lcd.buffer[0][6] = 'A';
    lcd.buffer[0][8] = 'T';
    lcd.buffer[0][10] = 'A';

    lcd.buffer[1][1] = 'B';
    lcd.buffer[1][3] = 'A';
    lcd.buffer[1][5] = 'T';
    lcd.buffer[1][7] = 'A';
    lcd.buffer[1][9] = 'T';
    lcd.buffer[1][11] = 'A';

    lcd_flush(&lcd);

    while(1);
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

    //Se eu quisesse ligar interrupções eu iria fazer isso aqui, depois de re-ligar o módulo..
}

LCD initialize_lcd(byte address)
{
    LCD lcd;
    lcd.address = LCD_I2C_ADDRESS;
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

    //Verifico se é um ACK ou um NACK
    if ((UCB0IFG & UCNACKIFG) != 0)
    {
        //Peço uma condição de parada
        LED_RED_ON;
        UCB0CTL1 |= UCTXSTP;
    } else
    {
        //Peço uma condição de parada
        LED_GREEN_ON;
        UCB0CTL1 |= UCTXSTP;
    }

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


