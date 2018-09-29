#include <stdint.h>
#include <stdlib.h>
#include "utils.h"
#include "codec.h"
#include "keypad_util.h"
#include "lcd_funcs.h"

void blink_dash(void){
	LEDRed_On();
	delay_1ms(500);
	LEDRed_Off();
	delay_1ms(100);
}
void blink_dot(void){
	LEDBlue_On();
	delay_1ms(200);
	LEDBlue_Off();
	delay_1ms(100);
}
void delay_space(void){
	delay_1ms(1000);
}
void delay_letter(void){
	delay_1ms(500);
}


void flash_morse(short msg){
	// Flashes a single character in morse code

	int contn = 1;
	for (int i = 9; i > 0 && contn; i -= 2){
		if ((msg & (1<<i)) && (~msg & (1<<(i-1)))){
			// Tone is 10, send dash
			blink_dash();
		}
		else if ((~msg & (1<<i)) && (msg & (1<<(i-1)))){
			// Tone is 01, send dot
			blink_dot();
		}
		else if ((~msg & (1<<i)) && (~msg & (1<<i-1))){
			// Tone is 00

			// If 00 is first one, this is a space
			if (i == 15) delay_space();
			else delay_letter();
			// Otherwise, delay for end of word
			contn = 0;

		}

	}
}


void transmit_message(char* msg, int len){
	// Convert char array to encoded morse array
	// Then flash LED based on that
	for (int i = 0; i < len; i++){
		flash_morse(encode(msg[i]));
	}
}

void transmitter(void){
	LEDGreen_On();
	struct string* disp = keypad_listen(); // disp is a string of chars, with a given length
	lcd_print_var(disp->rec_word, disp->word_length);
	LEDGreen_Off();
	transmit_message(disp->rec_word, disp->word_length);
}

