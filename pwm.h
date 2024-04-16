#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/delay.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "lcd.h"
#include "avrlcd.h"
#define __ASSERT_USE_STDERR
#include <assert.h>


#define DEBUG_BAUD  9600


#define PINV PA0
#define PINI PA1
#define WINDI PA2
#define PVI   PA3


#define DDRIN DDRA
#define PORTIN PORTA
#define PININ PINA
#define RLOAD1 PA4
#define RLOAD2 PA5
#define RLOAD3 PA6

#define DDROUT DDRD
#define PORTOUT PORTD 
#define PINOUT PIND 
#define CHARGEPIN PD0
#define DISCHARGEPIN PD1
#define SLOAD1 PD2
#define SLOAD2 PD3
#define SLOAD3 PD4
#define ILOAD1 1.2
#define ILOAD2 2
#define ILOAD3 0.8
#define THRESHOLD_V 240
#define THRESHOLD_I 2.9
#define BATTERYI 1
#define XBORDER 20;
#define ROW1 30;
#define ROW2 60;
#define ROW3 90;
#define ROW4 120;
#define ROW5 150;
#define ROW6 180;


void read_inputs(double* Iwind, double* Isolar, double* busbar_v, double* busbar_i, uint8_t* load1_req, uint8_t* load2_req, uint8_t* load3_req);
void init_lcd();
void init_pwm();
void init_adc();
void init_digital();
void init_timer();
void init_loadsPwm();
void set_pwm(double val);
uint16_t adc(int channel);
uint8_t get_digital(uint8_t pin);
double getV_amplitude();
double getI_amplitude();
void set_switch(uint8_t pin, uint8_t state);
void battery_control(uint8_t charge_c, uint8_t discharge_c);
void begin_charge (uint8_t mode, uint32_t* start_t, uint8_t* battery_charge, uint8_t* battery_discharge);
void end_charge (uint8_t mode, uint32_t* start_t,uint32_t* total_t, uint8_t* battery_charge, uint8_t* battery_discharge);
void set_loads(uint8_t* load1_set,uint8_t* load2_set, uint8_t* load3_set, uint8_t* load1_req,uint8_t* load2_req, uint8_t* load3_req);
void load_control(uint8_t* load1_set,uint8_t* load2_set, uint8_t* load3_set);
void controller(uint8_t mode, double* Imains, double* Irenewable, uint8_t* status_mains, uint8_t* load1_req, uint8_t* load2_req, uint8_t* load3_req, uint8_t* load1_set, uint8_t* load2_set, uint8_t* load3_set, uint8_t* control);
void status_bar(uint16_t length, uint16_t colour);
void draw_screen(int state);
void print_numbers(double val, int row);
void display_time();




//UART debugging


int uputchar0(char c, FILE *stream)
{
	if (c == '\n') uputchar0('\r', stream);
	while (!(UCSR0A & _BV(UDRE0)));
	UDR0 = c;
	return c;
}

int ugetchar0(FILE *stream)
{
	while(!(UCSR0A & _BV(RXC0)));
	return UDR0;
}

void init_debug_uart0(void)
{
	/* Configure UART0 baud rate, one start bit, 8-bit, no parity and one stop bit */
	UBRR0H = (F_CPU/(DEBUG_BAUD*16L)-1) >> 8;
	UBRR0L = (F_CPU/(DEBUG_BAUD*16L)-1);
	UCSR0B = _BV(RXEN0) | _BV(TXEN0);
	UCSR0C = _BV(UCSZ00) | _BV(UCSZ01);

	/* Setup new streams for input and output */
	static FILE uout = FDEV_SETUP_STREAM(uputchar0, NULL, _FDEV_SETUP_WRITE);
	static FILE uin = FDEV_SETUP_STREAM(NULL, ugetchar0, _FDEV_SETUP_READ);

	/* Redirect all standard streams to UART0 */
	stdout = &uout;
	stderr = &uout;
	stdin = &uin;
}


