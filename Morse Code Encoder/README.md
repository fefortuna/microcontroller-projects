# Morse Encoder/Decoder
Board: NXP FRDM-K64F

## Description
Goal of this project was to construct a Morse Code encoder.

- Uses a standard 12-button keypad with buttons set up in a matrix format for text input (old cellphone-style typing). Submit message with '#' key. Functions related to using this are in keypad_util.h
- An HD44780 character LCD screen is used to display the input. Clears when message is 'submitted'. Related functions in lcd_utils.h and lcd_funcs.h
- Once submitted, message is output on a pin (connect to an LED to see it). use transmit_message in transmitter.h
