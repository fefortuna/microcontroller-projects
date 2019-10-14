#ifndef KEYPAD_UTIL_H
#define KEYPAD_UTIL_H

struct string{
	char* rec_word;
	int word_length;
};

struct string* keypad_listen(void);

#endif
