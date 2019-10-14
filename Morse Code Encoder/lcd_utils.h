#ifndef __LCD_UTILS_
#define __LCD_UTILS_

#include "utils.h"

uint8_t lcd_util_data_read(void);
int8_t  lcd_util_data_write(uint8_t data);
uint8_t lcd_util_instruction_read(void);
int8_t  lcd_util_instruction_write(uint8_t instruction);
void    lcd_init_function_set(void);

#endif
