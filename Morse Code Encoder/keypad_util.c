#include <stdint.h>
#include <stdlib.h>
#include <fsl_device_registers.h>
#include "keypad_util.h"
#include "utils.h"
#include "lcd_funcs.h"


#define ROWS 4 //four rows
#define COLS 3 //three columns

char keys[ROWS][COLS][5] = {
    {{'!','!','!','!','1'},{'A','B','C','C','2'},{'D','E','F','F','3'}},
    {{'G','H','I','I','4'},{'J','K','L','L','5'},{'M','N','O','O','6'}},
    {{'P','Q','R','S','7'},{'T','U','V','V','8'},{'W','X','Y','Z','9'}},
    {{'*','*','*','*','*'},{' ',' ',' ',' ','0'},{'#','#','#','#','#'}},
};

//int rec_word_count = 0;
volatile char keypad_row = 0;
volatile char keypad_col = 0;
volatile uint8_t char_set = 0; // Set to 1 if new input from the keyboard has been received, and keypad_row/col haven't been processed

// Turn rows on or off
void rows_on(void){
	PTC->PSOR = (1<<16 | 1<<17);
	PTB->PSOR = (1<<18 | 1<<19);
}
void rows_off(void){
	PTC->PCOR = (1<<16 | 1<<17);
	PTB->PCOR = (1<<18 | 1<<19);
}

// Turna  specific row on, and others off.
void row_on(char pick){
	rows_off();
	switch(pick){
		case 1: PTC->PSOR = (1<<16); break;
		case 2: PTC->PSOR = (1<<17); break;
		case 3: PTB->PSOR = (1<<18); break;
		case 4: PTB->PSOR = (1<<19); break;
	}
}

// Main function, listens for keypad input until message is complete
int keypad_listen(char* recorded_word){
	char contn = 1;
	int wc = 0;
	char key;
	char disp;
	char disp_count;

	lcd_clear_topline();

	while (contn){
		while(!char_set);
		char_set = 0;
		key = keys[keypad_row][keypad_col][4];
		if (key == '#' || (wc >= 16 && (disp_count == 4 || key != disp))) contn = 0;
		else if (key == '*'){
			// Delete last char
			disp = 0;
			wc -= 1;
			recorded_word[wc+1] = 0x00;
			lcd_remove_char();
		}
		else if (key == disp && disp_count < 4){
			// Same key was pressed again
			disp_count++;
			recorded_word[wc-1] = keys[keypad_row][keypad_col][disp_count];
			lcd_replace_char(keys[keypad_row][keypad_col][disp_count]);
		}
		else{
			// Different key was pressed
			disp_count = 0;

			if (keypad_row == 0 & keypad_col == 0){
				// If the 1 button was pressed
				// Nothing happens, just breaks the line of repeated presses
				disp = 0; 
			}
			else{
				// If another key was pressed
				disp = key;
				recorded_word[wc] = keys[keypad_row][keypad_col][0];
				wc += 1;
				lcd_add_char(keys[keypad_row][keypad_col][0]);
			}
				
		}
		
	}
	lcd_clear_topline();
	return wc;
}



void PORTC_IRQHandler(void){
	uint8_t col_val = 0;
	uint8_t col_pins[3] = {1, 8, 9};
	// char col_vals[3] = {'a', 'b', 'c'}; // For debugging
	// char row_vals[4] = {'1', '2', '3', '4'}; // For debugging

	for (int i = 0; i < 3; i++){
		if (PORTC->ISFR & (1<<col_pins[i])){
			// lcd_add_char(col_vals[i]); // For debugging
			keypad_col = i;
			col_val = col_pins[i];
		}
	}


	for (int j = 0; j < 4; j++){
		row_on(j+1);
		if (PTC->PDIR & (1<<col_val)){
			// lcd_add_char(row_vals[j]); // For debugging
			keypad_row = j;
		}
	}

	rows_on();
	char_set = 1;
	PORTC->ISFR &= (1<<1 | 1<<8 | 1<<9);  // Clear ISR flags (use & b/c setting to 1 again clears it, weirdly enough)
	
}
