#ifndef _TRANSMITTER_
#define _TRANSMITTER_

void transmitter(void);
void transmit_message(char* msg, int len);
void flash_morse(short msg);
void blink_dash(void);
void blink_dot(void);

void delay_space(void);
void delay_letter(void);

#endif
