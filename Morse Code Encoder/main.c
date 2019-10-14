#include "3140_concur.h"
#include "utils.h"
#include "lcd_funcs.h"
#include "lcd_utils.h"
#include "keypad_util.h"
#include "transmitter.h"
#include "codec.h"
#include "receiver.h"
#include "lock.h"
#include "shared_structs.h"

void encoder (void)
{
	char recorded_word[16];
	int len;
	while(1){
		LEDGreen_On();
		len = keypad_listen(&recorded_word); // this will only return once the 'submit' key is hit
		lcd_print_var(&recorded_word, len);
		LEDGreen_Off();
		transmit_message(&recorded_word, len);
	}
	
}

void decoder (void)
{
	// Not done :(
	while(1){
		if (button2_value()) lcd_print_bottomline("a              ");
	}
}

int main (void)
{

 	GPIO_Initialize();  
	if (process_create (encoder,30) < 0) {
 		return -1;
 	}
 	if (process_create (decoder,30) < 0) {
 		return -1;
 	}
 	process_start();
 
	//LEDGreen_On();
 
 	while (1){}	 
	return 0;
}
