# DTMF dialer

MCU: PIC32MX250
For use with Sean Carrol's Big Board

## Requirements
This project requires 
	pt_cornell_1_2_1.h by Bruce Land
	tft_master.c by Syed Tahmid Mahbub
	tft_master.h by Syed Tahmid Mahbub
	tft_gfx.c by Syed Tahmid Mahbub
	tft_gfx.h by Syed Tahmid Mahbub
	config.h by Syed Tahmid Mahbub
	glcdfont.c

All of these can be found in people.ece.cornell.edu/land/courses/ece4760/

It was written and compiled with the use of MPLAB X IDE

## Description
The goal of this product was to construct a standard Dual Tone Multi-Frequency generator, similar to the ones used by touch-tone phones. The system records up to 12 digits, then plays back the corresponding frequencies by using Direct Digital Synthesis sampling at a rate of 131kHz through a DAC, which is then output to speakers. The system must adhere to DTMF standard. 
