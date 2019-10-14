#include <stdint.h>
#include "lcd_funcs.h"
#include "lcd_utils.h"
#include "lock.h"
#include "utils.h"


lock_t l;
uint8_t top_line_addr;

int8_t lcd_clear_display(void) {
	int8_t ret = lcd_util_instruction_write(0x01);
	delay_1ms(2);
	return (ret);
}

int8_t lcd_return_home(void) {
	return (lcd_util_instruction_write(0x02));
}

int8_t lcd_entry_mode_set(uint8_t do_increment, uint8_t do_shift) {
	uint8_t data = 0x04;

	if (do_increment) {
		data = data | 0x02;
	}

	if (do_shift) {
		data = data | 0x01;
	}

	return (lcd_util_instruction_write(data));
}

int8_t lcd_display_on_off_control(uint8_t ctrlword) {
	uint8_t data = 0x08 | (ctrlword & 0x07);
	return (lcd_util_instruction_write(data));
}

int8_t lcd_cursor_or_display_shift(uint8_t shift_screen, uint8_t shift_right) {
	uint8_t data = 0x10;

	if (shift_screen) {
		data = data | 0x08;
	}

	if (shift_right) {
		data = data | 0x04;
	}

	return (lcd_util_instruction_write(data));
}

int8_t lcd_function_set(uint8_t bus_is_8bit, uint8_t display_is_2line, uint8_t display_font_5x11) {
	uint8_t data = 0x20;

	if (bus_is_8bit) {
		data = data | 0x10;
	}

	if (display_is_2line) {
		data = data | 0x08;
	}

	if (display_font_5x11) {
		data = data | 0x04;
	}

	return (lcd_util_instruction_write(data));
}

int8_t lcd_set_cgram_address(uint8_t cgram_address) {
	uint8_t data = 0x40;

	data = data | (cgram_address & 0x3F);
	return (lcd_util_instruction_write(data));
}

int8_t lcd_set_ddram_address(uint8_t ddram_address) {
	uint8_t data = 0x80;

	data = data | (ddram_address & 0x7F);
	return (lcd_util_instruction_write(data));
}

uint8_t lcd_read_busy_flag_and_address(void) {
	return (lcd_util_instruction_read());
}

int8_t lcd_write_data_to_cg_or_ddram(uint8_t data) {
	return (lcd_util_data_write(data));
}

uint8_t lcd_read_data_from_cg_or_ddram(void) {
	return (lcd_util_data_read());
}

void lcd_clear_topline(void){
	l_lock(&l);
	lcd_set_ddram_address(0x00);
	for (int i = 0; i < 16; i++){
		lcd_util_data_write(' ');
	}
	lcd_set_ddram_address(0x00);
	l_unlock(&l);
}

void lcd_print_bottomline(char str[16]){
	// Clears display, then prints 16 characters to the screen
	l_lock(&l);
	top_line_addr = lcd_read_busy_flag_and_address();
	lcd_set_ddram_address(0x40);
	for (int i = 0; i < 16; i++){
		lcd_util_data_write(str[i]);
	}
	lcd_set_ddram_address(top_line_addr);
	l_unlock(&l);
}
void lcd_print_topline(char str[16]){
	// Clears display, then prints 16 characters to the screen
	l_lock(&l);
	lcd_set_ddram_address(0x00);
	for (int i = 0; i < 16; i++){
		lcd_util_data_write(str[i]);
	}
	lcd_set_ddram_address(0x00);
	l_unlock(&l);
}

void lcd_print_var(char str[], int len){
	lcd_clear_topline();
	l_lock(&l);
	for (int i = 0; i < len; i++){
		lcd_util_data_write(str[i]);
	}
	l_unlock(&l);
}

void lcd_add_char(char str){
	l_lock(&l);
	lcd_util_data_write(str);
	l_unlock(&l);
}

void lcd_replace_char(char str){
	l_lock(&l);
	lcd_cursor_or_display_shift(0, 0);
	lcd_util_data_write(str);
	l_unlock(&l);
}
void lcd_remove_char(void){
	l_lock(&l);
	lcd_cursor_or_display_shift(0, 0);
	lcd_util_data_write(0x10);
	lcd_cursor_or_display_shift(0, 0);
	l_unlock(&l);
}



void lcd_init(void) {
	lcd_init_function_set();
	lcd_util_instruction_write(0x38); 	// Function Set
	delay_1ms(1);
	lcd_util_instruction_write(0x0E);	// Display ON/OFF
	delay_1ms(1);
	lcd_util_instruction_write(0x06);	// Entry Mode Set
	delay_1ms(1);
	lcd_clear_display();
	delay_1ms(1);
	
	l_init(&l);
}
