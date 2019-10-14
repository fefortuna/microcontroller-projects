#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

//void LED_Initialize(void);
void LEDRed_Toggle (void);
void LEDBlue_Toggle (void);
void LEDGreen_Toggle (void);
void LEDRed_On (void);
void LEDRed_Off (void);
void LEDGreen_On (void);
void LEDGreen_Off (void);
void LEDBlue_On (void);
void LEDBlue_Off (void);
void LED_Off (void);
void delay (void);

// My GPIO functions
void GPIO_Initialize(void);
bool button2_value(void);
bool button3_value(void);
#endif


// Realtime Functions
#ifndef REAL_TIME
#define REAL_TIME
typedef struct {
	unsigned int sec;
	unsigned int msec;
	unsigned int usec;
} realtime_t;
extern realtime_t current_time;

unsigned int get_time(realtime_t*);

void delay_1ms(int ms);
void delay_1us(void);

#endif


