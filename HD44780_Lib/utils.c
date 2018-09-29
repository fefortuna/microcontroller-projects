#include <fsl_device_registers.h>
#include "utils.h"


// TO DO:
/*
Edit definitions below to correspond to which GPIO ports and pins you used
	Ports A to E are represented by numbers 0 to 4
*/

#define D7_PORT 3
#define D7_PIN  3
#define D6_PORT 3
#define D6_PIN  3
#define D5_PORT 3
#define D5_PIN  3
#define D4_PORT 3
#define D4_PIN  3
#define D3_PORT 3
#define D3_PIN  3
#define D2_PORT 3
#define D2_PIN  3
#define D1_PORT 3
#define D1_PIN  3
#define D0_PORT 3
#define D0_PIN  3

/*----------------------------------------------------------------------------
  GPIO_Initialize
 *----------------------------------------------------------------------------*/

void GPIO_Initialize(void){

	// Enable Clock to Ports that are used (Edit to only contain the ports you used)
  	SIM->SCGC5    |= (1<<13)|(1<<12)|(1<<11)|(1<<10)|(1<<9);  
	
	// Setup LCD Pins
	
	PORTD->PCR[3]  = (1<<8);  // D7
	PORTD->PCR[2]  = (1<<8);  // D6
	PORTD->PCR[0]  = (1<<8);  // D5
	PORTC->PCR[4]  = (1<<8);  // D4
	PORTC->PCR[12] = (1<<8);  // D4
	PORTC->PCR[3]  = (1<<8);  // D2
	PORTC->PCR[2]  = (1<<8);  // D1
	PORTA->PCR[2]  = (1<<8);  // D0
	PORTB->PCR[23] = (1<<8);  // EN
	PORTA->PCR[1]  = (1<<8);  // R/W
	PORTB->PCR[9]  = (1<<8);  // RS
	
	PTD->PDOR |= (1<<3 | 1<<2 | 1<<0);
	PTC->PDOR |= (1<<4 | 1<<3 | 1<<2 | 1<<12);
	PTA->PDOR |= (1<<2 | 1<<1 );
	PTB->PDOR |= (1<<9 | 1<<23);
	
	PTD->PDDR |= (1<<3 | 1<<2 | 1<<0);
	PTC->PDDR |= (1<<4 | 1<<3 | 1<<2 | 1<<12);
	PTA->PDDR |= (1<<2 | 1<<1);
	PTB->PDDR |= (1<<9 | 1<<23);

	NVIC_EnableIRQ(PORTC_IRQn);
	NVIC_SetPriority(PORTC_IRQn, 2);
}


// ---------------------------------------------
// Real time tracking Functions
// ---------------------------------------------
realtime_t current_time = {0,0}; // in milliseconds 

unsigned int get_time(realtime_t *time){
	return (1000 * time->sec) + time->msec;
};

void delay_1us(){}
	// I've measured it on an oscilloscope, calling this function creates about a 1us delay, or close enough to it

void delay_1ms(int ms){
	// microsecond delay, dependent on utils.h file
	unsigned int start_ms = (1000 * current_time.sec) + current_time.msec;
		while (((1000 * current_time.sec) + current_time.msec) - start_ms < ms);
}

void PIT1_IRQHandler(void){
	__disable_irq();
	current_time.msec++;
	if (current_time.msec == 1000){
		current_time.msec = 0;
		current_time.sec++;
	}
	
	PIT->CHANNEL[1].TFLG = 1;
	__enable_irq();
}
