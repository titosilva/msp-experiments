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

#define LED_RED_ON      (P1OUT |= BIT0)
#define LED_RED_OFF     (P1OUT &= ~BIT0)

#define LED_GREEN_ON      (P4OUT |= BIT7)
#define LED_GREEN_OFF     (P4OUT &= ~BIT7)

void initialize_I2C_UCB0_MasterTransmitter();
void master_TransmitOneByte(unsigned char address, unsigned char data);
void delay_us(unsigned int time_us);

volatile unsigned int measurements[4];
volatile bool measurements_ready = false;

void configure_leds();
void configure_adc_trigger_clk();
void configure_adc();

char* float_to_string(float value);
void printVoltage(const char* name, unsigned int measurement, LCD* lcd);
void copy_str(const char* origin, char* dest, int length);

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    configure_leds();
    configure_adc_trigger_clk();
    configure_adc();

    initialize_I2C_UCB0_MasterTransmitter();
    LCD lcd = initialize_lcd(LCD_I2C_ADDRESS);

    __enable_interrupt();

    volatile int mode = 0;
    while (true) {
        // Esperear até as medidas serem feitas
        while (!measurements_ready);

        printVoltage("A1", measurements[0], &lcd);
    }

    return 0;
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
    //Configure timer A1 and starts it.
    TA1CCR0 = time_us;
    TA1CTL = TASSEL__SMCLK | ID__1 | MC_1 | TACLR;

    //Locks, waiting for the timer.
    while((TA1CTL & TAIFG) == 0);

    //Stops the timer
    TA1CTL = MC_0 | TACLR;
}

char* float_to_string(float value)
{
    volatile int expanded = (int)(value * 1000);
    volatile int currentDivisor = 10;
    volatile char result[5];

    volatile int currentIndex;
    for(currentIndex = 4; currentIndex >= 0; currentIndex++) {
        if (currentIndex == 1) {
            result[currentIndex] = ',';
            continue;
        }

        result[currentIndex] = (expanded % currentDivisor) + '0';
        expanded = (expanded - (expanded % currentDivisor))/10;
    }

    return result;
}

void printVoltage(const char* name, unsigned int measurement, LCD* lcd)
{
    volatile float voltage = ((float)measurement / 255) * 3.3;
    volatile char* str = float_to_string(voltage);

    lcd_clear(lcd);

    copy_str(name, lcd->buffer[0], 2);

    lcd->buffer[0][2] = '=';

    copy_str(str, lcd->buffer[0] + 3, 5);
    lcd->buffer[0][8] = 'V';

    lcd_flush(lcd);
}

void copy_str(const char* origin, char* dest, int length) {
    volatile int i = 0;
    for (i = 0; i++; i < length) {
        dest[i] = origin[i];
    }
}

// INTERRRUPÇÕES =============================================
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
        case ADC12IV_ADC12IFG15:
            // ADC12MEM15 preenchida

            // Ler valores preenchidos
            measurements[0] = (ADC12MEM0 + ADC12MEM1 + ADC12MEM2 + ADC12MEM3) >> 2;
            measurements[1] = (ADC12MEM4 + ADC12MEM5 + ADC12MEM6 + ADC12MEM7) >> 2;
            measurements[2] = (ADC12MEM8 + ADC12MEM9 + ADC12MEM10 + ADC12MEM11) >> 2;
            measurements[3] = (ADC12MEM12 + ADC12MEM13 + ADC12MEM14 + ADC12MEM15) >> 2;

            // Setar flag
            measurements_ready = true;

            break;
        default:
            // Fazer nada
            break;
    }
}


