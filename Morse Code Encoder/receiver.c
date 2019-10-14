#include <stdbool.h>
#include <limits.h>
#include <string.h>

#include "utils.h"
#include "codec.h"
#include "lcd_funcs.h"

#include "receiver.h"

#define DASH_LENGTH 500
#define	WORD_SPACE 1250


// Data structures


/**
 * Binary-encoded Morse buffer.
 */
static struct morse_buffer {
	short morse; // The buffered output of the Morse decoder
	char index;  // The current bit index
} m_buf;

static struct string_buffer {
	char ascii[17]; // The ASCII output
	char index;     // The current string index
} str_buf;

// The number of consecutive milliseconds that the receiver has been the same value
static unsigned millis;

static realtime_t last_time;

// The last two states of the button
static bool last_val, prev_val;

// Callback functions

/**
 * Initializes or resets the Morse buffer.
 */
void morse_buffer_init(void) {
	m_buf.morse = 0;
	m_buf.index = 8;
}

void string_buffer_init(void) {
	memset(str_buf.ascii, ' ', sizeof(str_buf.ascii) - 1);
	str_buf.ascii[sizeof(str_buf.ascii) - 1] = '\0';
	str_buf.index = 0;
}

void record_dot(void) {
	m_buf.morse |= 0x01 << m_buf.index;
	
	if ((m_buf.index -= 2) == 0) {
		add_char();
	}
}

void record_dash(void) {
	m_buf.morse |= 0x02 << m_buf.index;
	
	if ((m_buf.index -= 2) == 0) {
		add_char();
	}
}

void add_char(void) {
	struct dec_list *map_ptr = dec_get(m_buf.morse);
	if (map_ptr != NULL) {
		str_buf.ascii[str_buf.index++] = map_ptr->ascii;
	} else {
		str_buf.ascii[str_buf.index] = '#'; // Invalid character; display error symbol and don't advance
	}
	morse_buffer_init();
	
	handle_overflow();
}

void add_space(void) {
	str_buf.ascii[str_buf.index++] = ' ';
	handle_overflow();
}

void handle_overflow(void) {
	int n = sizeof(str_buf.ascii) - 1;
	if (str_buf.index >= n) {
		memcpy(str_buf.ascii, str_buf.ascii + 1, n);
		str_buf.index--;
	}
}

/**
 * Called by the timer ISR every millisecond.
 */
void rcv_millis(void) {
}

/**
 * Called by the button ISR whenever the button is pressed or released.
 */
void rcv_press(void) {
}

/**
 * The receiver process body.
 */
void receiver(void) {
	morse_buffer_init();
	string_buffer_init();
	last_time.sec = last_time.msec = last_time.usec = 0;
	last_val = prev_val = 0;
	
	while (1) {
		// The time in milliseconds elapsed since the last button event
		millis = get_time(&current_time) - get_time(&last_time);
	
		// Update prev_val and last_val
		prev_val = last_val;
		last_val = button2_value();
		
		// Display last_val
		if (last_val) LEDBlue_On();
		else LEDBlue_Off();

		// The end of a '1' pulse
		if (prev_val && !last_val) {
			if (millis >= DASH_LENGTH) record_dash(); // if pulse was longer than DASH_LENGTH
			else record_dot(); // if pulse was shorter

			last_time = current_time;
		}
	
		// The end of a '0' pulse
		if (!prev_val && last_val) {
			if (millis >= DASH_LENGTH && millis < WORD_SPACE) add_char();

			last_time = current_time;
		}
		
		// '0' pulse longer than WORD_SPACE ms
		if (!last_val && millis == WORD_SPACE) add_space();

		// Repeatedly print the string buffer to the display
		lcd_print_bottomline(str_buf.ascii);
	}
}
