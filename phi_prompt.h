/*
  ______  ___                    __
 <  / _ \/ _ \___  __ _____  ___/ /
 / / // / ___/ _ \/ // / _ \/ _  / 
/_/\___/_/   \___/\_,_/_//_/\_,_/  

                Props to Dr Liudr - original author of this lib
--------------------------------------------------------------------------------
 *  \brief     This is an official release of the phi_prompt text-based user interface library for arduino 1.0.
 *  \details   This library requires 1.0 version of phi_interfaces library.
 *  \author    Dr. John Liu
 *  \version   1.0
 *  \date      01/30/2012
 *  \warning   PLEASE DO NOT REMOVE THIS COMMENT WHEN REDISTRIBUTING! No warranty!
 *  \copyright Dr. John Liu. Free software for educational and personal uses. Commercial use without authorization is prohibited.
--------------------------------------------------------------------------------
This is a fork of Dr Liudr's exceptional phi_prompt menu library for areduino-like devices.  It has been modified to function with serial enabled LCDs without further alteration.  The library has also been modified to function with the current avr-libc v1.8.0.   See issue 795 for more informationon that error:
https://code.google.com/p/arduino/issues/detail?id=795
This resulted in compilation errors such as "'prog_char' does not name a type".
                                                                               Many thanks to Dean Camera for his extremely informative paper "GCC and the PROGMEM Attribute" without which I would have had a much more difficult time making this library work with the current avr-gcc v1.8.0 (affects current 1.x and 1.5x versions of the Arduino IDE) 

*/

//The following are switches to certain functions. Comment them out if you don't want a particular function to save program space for larger projects
//#define scrolling // This turns on auto strolling on list items and includes scrolling text library function.
// Render list option bits
#define phi_prompt_arrow_dot B00000001      ///< List display option for using arrow/dot before a list item.
#define phi_prompt_index_list B00000010     ///< List display option for using an index list such as 12*4 for 4 total items and 3 is highlighted.
#define phi_prompt_current_total B00000100  ///< List display option for using a current/total index such as 2/4.
#define phi_prompt_auto_scroll B00001000    ///< List display option for using auto scrolling items longer than the width of the list.
#define phi_prompt_flash_cursor B00010000   ///< List display option for using flash cursor as indicator of highlighted item.
#define phi_prompt_center_choice B00100000  ///< List display option for using centering highlighted item on screen so highlighted item is always in the middle when possible.
#define phi_prompt_scroll_bar B01000000     ///< List display option for using a scroll bar on the right.
#define phi_prompt_invert_text B10000000    ///< List display option for using inverted text. Only some modified version of the library uses it.
#define phi_prompt_list_in_SRAM 0x100       ///< List display option for using a list that is stored in SRAM instead of in PROGMEM.

// Internal function key codes
#define total_function_keys 6       ///< Total number of function keys. At the moment, only 6 functions exist: up/down/left/right/enter/escape
#define function_key_code_base 1    ///< All function key codes start from this number and up continuously. The key codes are generated in wait_on_escape when a function key is pressed.
#define phi_prompt_up 1             ///< Function key code for up
#define phi_prompt_down 2           ///< Function key code for down
#define phi_prompt_left 3           ///< Function key code for left
#define phi_prompt_right 4          ///< Function key code for right
#define phi_prompt_enter 5          ///< Function key code for enter
#define phi_prompt_escape 6         ///< Function key code for escape

#define HD44780_lcd 0               ///< Type of display is HD44780
#define KS0108_lcd 1                ///< Type of display is KS0108 GLCD
#define serial_lcd 2                ///< Type of display is serial lcd

#if ARDUINO < 100
#include <WProgram.h>
#else
#include <Arduino.h>
#endif

#include <SoftwareSerial.h>
#include <avr/pgmspace.h>
#include <phi_interfaces.h>

union buffer_pointer    ///< This defines a union to store various pointer types.
{
  int *i_buffer;
  float * f_buffer;
  char ** list;
  char* msg;
  PGM_P msg_P;
};

union four_bytes        ///< This defines a union to store various data.
{
int i;
long l;
float f;
byte b;
char c;
char c_arr[4];
};

struct phi_prompt_struct    ///< This struct is used in exchange information between caller and function.
{
  buffer_pointer ptr;
  four_bytes low;   // Lower limit for text panel (.c), integers (.i) and floats (.f) and first item displayed + highlighted item for list (.i), say 102 means first item displayed on screen is item 1 and highlighted item is 2, both starting from 0.
  four_bytes high;  // Upper limit for text panel (.c), integers (.i) and floats (.f) and last item for list (.i)
  four_bytes step;  // Step size for integers (.i) and integer/decimal for floats (.i), such as 302 means 3 digits before decimal and 2 digits after.
  byte col;         // Which column to display input
  byte row;         // Which row to display input
  byte width;       // Maximal number of character on integers, floats, a list item, and total allowed input characters for text panel
  int option;       // What display options to choose
  void (*update_function)(phi_prompt_struct *); // This is not being used in this version but reserved for future releases.
}; //22 bytes

void init_phi_prompt(SoftwareSerial *l, multiple_button_input *k[], char ** fk, int w, int h, char i); ///< This is the library initialization routine.
void set_indicator(char i);                         ///< This sets the indicator used in lists/menus. The highlighted item is indicated by this character. Use '~' for a right arrow.
void set_bullet(char i);                            ///< This sets the bullet used in lists/menus. The non-highlighted items are indicated by this character. Use '\xA5' for a center dot.
void set_repeat_time(int i);                        ///< This sets key repeat time, how often a key repeats when held. It uses multiple_button_input.set_repeat()
void enable_key_repeat(boolean i);                  ///< Sets whether to enable key repeat on inputs. This is not used in this version. A future version may make use of it.
void enable_multi_tap(boolean i);                   ///< Sets whether to enable multi-tap on inputs. This is not used in this version. A future version may make use of it.
char inc(char ch, phi_prompt_struct *para);         ///< Increment character ch according to options set in para.
char dec(char ch, phi_prompt_struct *para);         ///< Decrement character ch according to options set in para.
char phi_prompt_translate(char key);                ///< This translates keypad input in case the key press is a function key. If the key press is not a function key, it returns without translation.
void scroll_text(char * src, char * dst, char dst_len, short pos);  ///< This scrolls a string into and out of a narrow window, for displaying long message on a narrow line.
void scroll_text_P(PGM_P src, char * dst, char dst_len, short pos); ///< This scrolls a string stored in PROGMEM into and out of a window, for displaying long message on a narrow line.
void msg_lcd(char* msg_lined);                      ///< This is a quick and easy way to display a string in the PROGMEM to the LCD.
void prev_line(phi_prompt_struct* para);            ///< Seeks previous line in a long message stored in SRAM.
void next_line(phi_prompt_struct* para);            ///< Seeks next line in a long message stored in SRAM.
void prev_line_P(phi_prompt_struct* para);          ///< Seeks previous line in a long message stored in PROGMEM.
void next_line_P(phi_prompt_struct* para);          ///< Seeks previous line in a long message stored in PROGMEM.
void center_text(char * src);                       ///< This function displays a short message centered with the display size.
void scroll_bar_v(byte p, byte c, byte r, byte h);  ///< Displays a scroll bar at column/row with height and percentage.
void long_msg_lcd(phi_prompt_struct *para);         ///< Displays a static long message stored in SRAM that could span multiple lines.
void long_msg_lcd_P(phi_prompt_struct *para);       ///< Displays a static long message stored in PROGMEM that could span multiple lines.
byte render_list(phi_prompt_struct *para);
void clear();
void setCursor(int posNum, int lineNum);
void blink();
void noBlink();
void cursor();
void noCursor(); 
void createChar(uint8_t location, uint8_t charmap[]); 

int wait_on_escape(int ref_time);                   ///< Returns key pressed or NO_KEY if time expires before any key was pressed. This does the key sensing and translation.

int ok_dialog(char msg[]);                          ///< Displays an ok dialog
int yn_dialog(char msg[]);                          ///< Displays a short message with yes/no options.
int input_integer(phi_prompt_struct *para);         ///< Input integer on keypad with fixed step, upper and lower limits.
int input_float(phi_prompt_struct *para);
int select_list(phi_prompt_struct *para);           ///< Displays a list/menu for the user to select. Display options for list: Option 0, display classic list, option 1, display 2X2 list, option 2, display list with index, option 3, display list with index2.
int input_panel(phi_prompt_struct *para);           ///< Input character options for input panel: Option 0, default, option 1 include 0-9 as valid inputs.
int input_number(phi_prompt_struct *para);          ///< Input number on keypad with decimal point and negative.
int text_area(phi_prompt_struct *para);             ///< Displays a text area using message stored in the SRAM.
int text_area_P(phi_prompt_struct *para);           ///< Displays a text area using message stored in PROGMEM.

