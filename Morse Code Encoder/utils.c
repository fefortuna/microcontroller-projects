#include <fsl_device_registers.h>
#include "utils.h"


/*----------------------------------------------------------------------------
  Functions that initialize LEDs and other GPIO ports
 *----------------------------------------------------------------------------*/
void LED_Initialize(void) {
	// Called by GPIO Initialize
	PORTB->PCR[22] = (1 << 8);             // Pin PTB22 (LED) is GPIO
 	PORTB->PCR[21] = (1 << 8);             // Pin PTB21 (LED)is GPIO 
 	PORTE->PCR[26] = (1 << 8);             // Pin PTE26 (LED) is GPIO

	PTB->PDOR |= (1 << 21 | 1 << 22);      // switch Red/Green LED off?
	PTE->PDOR |= 1 << 26;                  // switch Blue LED off?
	
	PTB->PDDR |= (1 << 21 | 1 << 22); 		 // enable PTB18/19 as Output
	PTE->PDDR |= 1 << 26;                  // enable PTE26 as Output
}

void GPIO_Initialize(void){
	// Enable Clock to Port A-E
  	SIM->SCGC5    |= (1<<13)|(1<<12)|(1<<11)|(1<<10)|(1<<9);  

  	// Setup LEDs
	LED_Initialize();
	
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
	
	// Setup Buttons SW2 and SW3
	PORTC->PCR[6] = (1<<8);// | PORT_PCR_IRQC(1001));
	PORTA->PCR[4] = (1<<8);

	// Setup Keypad pins
	PORTC->PCR[16] = (1<<8); // ROW 1 (out)
	PORTC->PCR[17] = (1<<8); // ROW 2 (out)
	PORTB->PCR[18] = (1<<8); // ROW 3 (out)
	PORTB->PCR[19] = (1<<8); // ROW 4 (out)
	PORTC->PCR[9]  = (1<<8 | 1<<19 | 1<<16); // COL 3 (inp), with interrupt on rising edge
	PORTC->PCR[8]  = (1<<8 | 1<<19 | 1<<16); // COL 2 (inp), with interrupt on rising edge
	PORTC->PCR[1]  = (1<<8 | 1<<19 | 1<<16); // COL 1 (inp), with interrupt on rising edge
	PTC->PDOR |= (1<<16 | 1<<17);
	PTB->PDOR |= (1<<18 | 1<<19);
	PTC->PDDR |= (1<<16 | 1<<17);
	PTB->PDDR |= (1<<18 | 1<<19);
	NVIC_EnableIRQ(PORTC_IRQn);
	NVIC_SetPriority(PORTC_IRQn, 2);
}

/*----------------------------------------------------------------------------
  LED Functions
 *----------------------------------------------------------------------------*/

void LEDRed_Toggle (void) {
	PTB->PTOR = 1 << 22; 	   /* Red LED Toggle */
}
void LEDBlue_Toggle (void) {
	PTB->PTOR = 1 << 21; 	   /* Blue LED Toggle */
}
void LEDGreen_Toggle (void) {
	PTE->PTOR = 1 << 26; 	   /* Green LED Toggle */
}
void LEDRed_On (void) {
  PTB->PCOR   = 1 << 22;   /* Red LED On*/
}
void LEDRed_Off (void) {
  PTB->PSOR   = 1 << 22;   /* Red LED Off*/
}
void LEDGreen_On (void) {
  PTE->PCOR   = 1 << 26;   /* Green LED On*/
}
void LEDGreen_Off (void) {
  PTE->PSOR   = 1 << 26;   /* Green LED On*/
}

void LEDBlue_On (void) {
  PTB->PCOR   = 1 << 21;   /* Blue LED On*/
}
void LEDBlue_Off (void) {
  PTB->PSOR   = 1 << 21;   /* Blue LED On*/
}

void LED_Off (void) {	
	// Save and disable interrupts (for atomic LED change)
	uint32_t m;
	m = __get_PRIMASK();
	__disable_irq();
	
  PTB->PSOR   = 1 << 22;   /* Green LED Off*/
  PTB->PSOR   = 1 << 21;   /* Red LED Off*/
  PTE->PSOR   = 1 << 26;   /* Blue LED Off*/
	
	// Restore interrupts
	__set_PRIMASK(m);
}

// ------------------------------------------------------
// Other
// ------------------------------------------------------

bool button2_value(void){
	if ((1<<6) & PTC->PDIR) return 0;
	return 1;
} 
bool button3_value(void){
	if ((1<<4) & PTA->PDIR) return 0;
	return 1;
} 

void delay(void){
	int j;
	for(j=0; j<1000000; j++);
}


// ---------------------------------------------
// Realtime tracking Functions
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
