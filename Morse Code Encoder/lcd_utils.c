#include <stdint.h>
#include <stdlib.h>
#include <fsl_device_registers.h>
#include "utils.h"
#include "lcd_utils.h"

#define CLEAR 0
#define SET 1
#define INSTR 0
#define DATA 1
#define READ 1
#define WRITE 0


/* ----------------------------------------------------------------------------
  GPIO Utilities for LCD screen

  	PORTD->PCR[3]  = (1<<8);  // D7
	PORTD->PCR[2]  = (1<<8);  // D6
	PORTD->PCR[0]  = (1<<8);  // D5
	PORTC->PCR[4]  = (1<<8);  // D4
	
	PORTC->PCR[12] = (1<<8);  // D3
	PORTC->PCR[3]  = (1<<8);  // D2
	PORTC->PCR[2]  = (1<<8);  // D1
	PORTA->PCR[2]  = (1<<8);  // D0

	PORTB->PCR[23] = (1<<8);  // EN
	PORTA->PCR[1]  = (1<<8);  // R/W
	PORTB->PCR[9]  = (1<<8);  // RS

------------------------------------------------------------------------------- */

static void set_data_dir(uint8_t read){
	// Sets 8 data pins as inputs or outputs based on 'read' argument
	if (read){
		PTD->PDDR &= ~(1<<3 | 1<<2 | 1<<0);
		PTC->PDDR &= ~(1<<4 | 1<<12 | 1<<3 | 1<<2);
		PTA->PDDR &= ~(1<<2);
	}
	else{
		PTD->PDDR |= (1<<3 | 1<<2 | 1<<0);
		PTC->PDDR |= (1<<4 | 1<<12 | 1<<3 | 1<<2);
		PTA->PDDR |= (1<<2);
	}
}

void lcd_util_pin_rs(uint8_t is_set){
	// Set or clear Register Select pin
    if (is_set) PTB->PSOR = (1<<9);
    else 		PTB->PCOR = (1<<9);
}
void lcd_util_pin_rw(uint8_t is_set){
	// Set or clear Read/Write pin
	if (is_set) PTA->PSOR = (1<<1);
	else 		PTA->PCOR = (1<<1);
	set_data_dir(is_set); // Direction of data pins is tied to R/W pin
}
void lcd_util_pin_e(uint8_t is_set){
	// Set or clear Enable pin
	if (is_set) PTB->PSOR = (1<<23);
	else 		PTB->PCOR = (1<<23);
}

uint8_t lcd_util_data_get(void){
	// Reads data from 8 input pins
	uint8_t data = 0;
	if (0x0008 & PTD->PDIR) data += 0x80;	// D7
	if (0x0004 & PTD->PDIR) data += 0x40;	// D6
	if (0x0001 & PTD->PDIR) data += 0x20;	// D5
	if (0x0010 & PTC->PDIR) data += 0x10;	// D4

	if (0x1000 & PTC->PDIR) data += 0x08;	// D3
	if (0x0008 & PTC->PDIR) data += 0x04;	// D2
	if (0x0004 & PTC->PDIR) data += 0x02;	// D1
	if (0x0004 & PTA->PDIR) data += 0x01;	// D0

	return data;
}

void lcd_util_data_set(uint8_t data){
	// Write 8 bits of data to the 8 data pins
	if (0x80 & data) PTD->PSOR = (1<<3);
	else 			 PTD->PCOR = (1<<3);
	if (0x40 & data) PTD->PSOR = (1<<2);
	else 			 PTD->PCOR = (1<<2);
	if (0x20 & data) PTD->PSOR = (1<<0);
	else 			 PTD->PCOR = (1<<0);
	if (0x10 & data) PTC->PSOR = (1<<4);
	else 			 PTC->PCOR = (1<<4);

	if (0x08 & data) PTC->PSOR = (1<<12);
	else 			 PTC->PCOR = (1<<12);
	if (0x04 & data) PTC->PSOR = (1<<3);
	else 			 PTC->PCOR = (1<<3);
	if (0x02 & data) PTC->PSOR = (1<<2);
	else 			 PTC->PCOR = (1<<2);
	if (0x01 & data) PTA->PSOR = (1<<2);
	else 			 PTA->PCOR = (1<<2);
}

// -----------------------------------------------------------------------
//	LCD General Utility Functions
// -----------------------------------------------------------------------

static int8_t lcd_util_check_busy(void) {
	// Checks busy flag from LCD
	// If LCD is busy, LED lights up blue until no longer busy
	LEDBlue_On();
	while (lcd_util_instruction_read() & 0x80) {
		return (-1);
	}
	LEDBlue_Off();
	return (0);


}

static void lcd_util_enable_pulse(void) {
	// Turn the enable pin on and off
	lcd_util_pin_e(0);
	delay_1us();
	lcd_util_pin_e(1);
	delay_1us();
	lcd_util_pin_e(0);
	delay_1ms(1);
}

uint8_t lcd_util_register_read(uint8_t is_data_reg) {
	// Performs a read from the LCD
	// Reads from either data reg or instruction reg
	uint8_t data = 0;

	lcd_util_pin_rs(is_data_reg);
	lcd_util_pin_rw(READ);
	delay_1us();
	
	lcd_util_pin_e(1);
	delay_1us();

	data = lcd_util_data_get();
	lcd_util_pin_e(0);

	return (data);
}

uint8_t lcd_util_instruction_read(void) {
	// Read from instruction register
	return (lcd_util_register_read(INSTR));
}

uint8_t lcd_util_data_read(void) {
	// Read from data register
	return (lcd_util_register_read(DATA));
}

int8_t lcd_util_register_write(uint8_t data, uint8_t is_data_reg) {
	// Performs a write operation to the LCD
	if (lcd_util_check_busy()) {
		return (-1);
	}
	lcd_util_pin_rs(is_data_reg);
	lcd_util_pin_rw(WRITE);
	delay_1us();

	lcd_util_data_set(data);

	lcd_util_enable_pulse();
	return (0);
}

int8_t lcd_util_data_write(uint8_t data) {
	// Write to data register
	return lcd_util_register_write(data, DATA);
}

int8_t lcd_util_instruction_write(uint8_t instruction) {
	// Write to instruction register
	return lcd_util_register_write(instruction, INSTR);
}

void lcd_util_init_write(uint8_t data) {
	// Used in initializing the LCD, it's a write operation that
	// does not check the busy flag
	lcd_util_pin_rs(INSTR);
	lcd_util_pin_rw(WRITE);
	lcd_util_data_set(data);
	lcd_util_enable_pulse();
}

void lcd_init_function_set(void) {
	// From page 45 of the HD44780 Datasheet

	delay_1ms(100);				// Wait min 50ms for LCD to boot up (should be long done by this point)
	lcd_util_init_write(0x30);			// Function Set (0011xxxx) set to 8-bit interface
	delay_1ms(5);				// Wait for more than 4.1ms
	lcd_util_init_write(0x30);			// Try again
	delay_1ms(1);				// Wait for more than 100us
	lcd_util_init_write(0x30);			// Try again
	delay_1ms(1);
}
