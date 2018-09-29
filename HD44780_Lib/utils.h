#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

// My GPIO functions
void GPIO_Initialize(void);
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


