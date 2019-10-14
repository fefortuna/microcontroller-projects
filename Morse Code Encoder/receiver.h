#ifndef __RECEIVER_H__
#define __RECEIVER_H__

void morse_buffer_init(void);
void string_buffer_init(void);
void record_dot(void);
void record_dash(void);
void add_char(void);
void add_space(void);
void handle_overflow(void);

void rcv_millis(void);
void rcv_press(void);
void receiver(void);

#endif
