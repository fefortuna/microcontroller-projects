#ifndef _LCD_FUNCS_
#define _LCD_FUNCS_

#include <stdint.h>

#define HD44780_DISP_ON 0x04
#define HD44780_CUR_ON 0x02
#define HD44780_BLINK_ON 0x01

int8_t 	lcd_clear_display(void);
int8_t 	lcd_return_home(void);
int8_t 	lcd_entry_mode_set(uint8_t do_increment, uint8_t do_shift);
int8_t 	lcd_display_on_off_control(uint8_t ctrlword);
int8_t 	lcd_cursor_or_display_shift(uint8_t shift_screen, uint8_t shift_right);
int8_t 	lcd_function_set(uint8_t bus_is_8bit, uint8_t display_is_2line, uint8_t display_font_5x11);
int8_t 	lcd_set_cgram_address(uint8_t cgram_address);
int8_t 	lcd_set_ddram_address(uint8_t ddram_address);
uint8_t lcd_read_busy_flag_and_address(void);
int8_t  lcd_write_data_to_cg_or_ddram(uint8_t data);
uint8_t lcd_read_data_from_cg_or_ddram(void);
void	lcd_clear_topline(void);
void	lcd_print_bottomline(char str[16]);
void	lcd_print_topline(char str[16]);
void	lcd_print_var(char str[], int len);
void	lcd_add_char(char str);
void    lcd_replace_char(char str);
void    lcd_remove_char(void);
void	lcd_init(void);

#endif /* _LCD_FUNCS_ */
