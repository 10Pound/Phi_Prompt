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

#if ARDUINO < 100
#include <WProgram.h>
#else
#include <Arduino.h>
#endif

#include <SoftwareSerial.h>
#include <avr/pgmspace.h>
#include <phi_interfaces.h>
#include <phi_prompt.h>

const char phi_prompt_lcd_ch0[] PROGMEM = { 4,14,31,64,31,31,31,31,0}; ///< Custom LCD character: Up triangle with block
const char phi_prompt_lcd_ch1[] PROGMEM = { 4,14,31,64,64,64,64,64,0}; ///< Custom LCD character: Up triangle 
const char phi_prompt_lcd_ch2[] PROGMEM = {31,31,31,31,64,64,64,64,0}; ///< Custom LCD character: Top block
const char phi_prompt_lcd_ch3[] PROGMEM = {64,64,64,64,31,31,31,31,0}; ///< Custom LCD character: Bottom block
const char phi_prompt_lcd_ch4[] PROGMEM = {64,64,64,64,64,31,14, 4,0}; ///< Custom LCD character: Down triangle
const char phi_prompt_lcd_ch5[] PROGMEM = {31,31,31,31,64,31,14, 4,0}; ///< Custom LCD character: Down triangle with block
const char* const phi_prompt_lcd_ch_item[] PROGMEM = {phi_prompt_lcd_ch0, phi_prompt_lcd_ch1, phi_prompt_lcd_ch2, phi_prompt_lcd_ch3, phi_prompt_lcd_ch4, phi_prompt_lcd_ch5}; ///< Custom LCD character char array addresses. 

const char yn_00[] PROGMEM = " YES >NO<";          ///< This list item is used to render Y/N dialog
const char yn_01[] PROGMEM = ">YES< NO ";          ///< This list item is used to render Y/N dialog
const char* const yn_items[]= {yn_00,yn_01};  ///< This list  is used to render Y/N dialog

static SoftwareSerial * lcd;                 ///< This pointer stores the SoftwareSerial object for display purpose.
static int lcd_w;                           ///< This is the width of the LCD in number of characters.
static int lcd_h;                           ///< This is the height of the LCD in number of characters.
static char indicator;                      ///< This is the character used as indicator in lists/menus. The highlighted item is indicated by this character. Use '~' for a right arrow.
static char bullet;                         ///< This is the bullet used in lists/menus. The non-highlighted items are indicated by this character. Use '\xA5' for a center dot.
static char ** function_keys;               ///< This points to an array of pointers that each is a zero-terminated string representing function keys.
static multiple_button_input ** mbi_ptr;    ///< This points to an array of pointers that each points to a multiple_button_input object.
static byte lcd_type;                       ///< This indicates the type of lcd, such as HD44780 or GLCD.
static boolean key_repeat_enable=1;         ///< This is not used in this version. A future version may make use of it.
static boolean multi_tap_enable=0;          ///< This is not used in this version. A future version may make use of it.
static phi_prompt_struct shared_struct;     ///< This struct is shared among simple function calls.
//Utilities
/**
 * \details This initializes the phi_prompt library. It needs to be called before any phi_prompt functions are called.
 * \param l This is the address of your LCD object, which you already used begin() on, such as &LCD.
 * \param k This is the name of the pointer array that stores the address of (pointer to) all your input keypads. The last element of the array needs to be 0 to terminate the array.
 * \param fk This is the name of the array that stores the names of all char arrays with function keys. Make sure you use strings such as "U" for each array to indicate function keys instead of char like 'U'.
 * \param w This is the width of the LCD in number of characters.
 * \param h This is the height of the LCD in number of characters.
 * \param i This is the character used as indicator in lists/menus. The highlighted item is indicated by this character. Use '~' for a right arrow.
 */
void init_phi_prompt(SoftwareSerial *l, multiple_button_input *k[], char ** fk, int w, int h, char i)
{
  lcd=l;
  mbi_ptr=k;
  function_keys=fk;
  lcd_w=w;
  lcd_h=h;
  indicator=i;
  bullet='\xA5';
  byte ch_buffer[10]; // This buffer is required for custom characters on the LCD.
  if (lcd!=0)
  {
    for (int i=0;i<6;i++)
    {
      strcpy_P((char*)ch_buffer,(char*)pgm_read_word(&(phi_prompt_lcd_ch_item[i])));
      createChar(i, ch_buffer);
    }
  }
  lcd_type=HD44780_lcd;
}


void set_indicator(char i)
{
  indicator=i;
}

void set_bullet(char i)
{
  bullet=i;
}

void set_repeat_time(int i)
{
  mbi_ptr[0]->set_repeat(i);
}

void enable_key_repeat(boolean i)
{
  key_repeat_enable=i;
}

void enable_multi_tap(boolean i)
{
//if (!multi_tap_enable) multi_tap(init);
  multi_tap_enable=i;
}

/**
 * \details Increment character ch according to options set in para. This function is used in input panel with up/down key to increment the current character to the next.
 * \param para This is the phi_prompt struct to carry information between callers and functions.
 * Option 0: default, option 1: include 0-9 as valid inputs, option 2: only 0-9 are valid, option 3: only 0-9 are valid and the first digit can be '-', option 4: only 0-9 are valid and number increments, option 5: only 0-9 are valid and the first digit can be '-' and number increments.
 * To do:
 * Option 2-4, automatically increment or decrement a whole series of numbers when they are connected, such as File0009.txt when increase 9 (if only 0-9 are allowed) will give File0010.txt.
 * If you are not interested in the inner working of this library, don't call it.
 * \return It returns the character after the increment.
 */
char inc(char ch, phi_prompt_struct *para)
{
  if ((ch<para->high.c)&&(ch>=para->low.c)) return (++ch);
  else
  {
    switch (para->option)
    {
      case 0: // No options. The high and low determine range of characters you can enter.
      if (ch==para->high.c) ch=para->low.c;
      break;
      
      case 1: // Include 0-9
      if (ch=='9') ch=para->low.c;
      else if ((ch>='0')&&(ch<'9')) ch++;
      else if (ch==para->high.c) ch='0';
      break;

      case 2: // only 0-9
      case 4: // only 0-9 and automatically decrements (done at higher level)
      if (ch=='9') ch='0';
      else ch++;
      break;

      case 3: // only 0-9 and '-'
      case 5: // only 0-9 and '-' and automatically increments (done at higher level)
      if (ch=='9') ch='-';
      else if (ch=='-') ch='0';
      else ch++;
      break;
    }
  }
  return ch;
}

/**
 * \details Decrement character ch according to options set in para. This function is used in input panel with up/down key to decrement the current character to the previous.
 * \param para This is the phi_prompt struct to carry information between callers and functions.
 * Option 0: default, option 1: include 0-9 as valid inputs, option 2: only 0-9 are valid, option 3: only 0-9 are valid and the first digit can be '-', option 4: only 0-9 are valid and number increments, option 5: only 0-9 are valid and the first digit can be '-' and number increments.
 * To do:
 * Option 2-4, automatically increment or decrement a whole series of numbers when they are connected, such as File0009.txt when increase 9 (if only 0-9 are allowed) will give File0010.txt.
 * If you are not interested in the inner working of this library, don't call it.
 * \return It returns the character after the decrement.
 */
char dec(char ch, phi_prompt_struct *para)// Decrease character. Used in input panels
{
  if ((ch<=para->high.c)&&(ch>para->low.c)) return (--ch);
  else
  {
    switch (para->option)
    {
      case 0: // No options. The high and low determine range of characters you can enter.
      if (ch==para->low.c) ch=para->high.c;
      break;
      
      case 1: // Include 0-9
      if (ch=='0') ch=para->high.c;
      else if ((ch>'0')&&(ch<='9')) ch--;
      else if (ch==para->low.c) ch='9';
      break;

      case 2: // only 0-9
      case 4: // only 0-9 and automatically decrements (done at higher level)
      if (ch=='0') ch='9';
      else ch--;
      break;

      case 3: // only 0-9 and '-'
      case 5: // only 0-9 and '-' and automatically decrements (done at higher level)
      if (ch=='0') ch='-';
      else if (ch=='-') ch='9';
      else ch--;
      break;
    }
  }
  return ch;
}

/**
 * \details This function translates key press returned from all keypads. This function is only called by wait_on_escape.
 * \return If a key is defined as a function key, it returns the code of the function key defined in the function_key_base define section.
 * If the key is not a function key, it returns the key unaltered.
 * If you are not interested in the inner working of this library, don't call it.
 */
char phi_prompt_translate(char key)
{
  for (byte i=0;i<total_function_keys;i++)
  {
    byte j=0;
    while(function_keys[i][j])
    {
      if (key==function_keys[i][j]) return (function_key_code_base+i);
      j++;
    }
  }
  return key;
}

//Text renderers

/**
 * \details This function displays a short message centered with the display size.
 * This outputs a text center-aligned on the display depending on the display size.
 * Eg. center_text("Introduction") outputs ">>>>Introduction<<<<" on 20 column display.
 */
void center_text(char * src)
{
  char msg_buffer[21];
  byte j=0;
  for (byte i=0;i<lcd_w;i++)
  {
    if (i<lcd_w/2-(strlen(src)-strlen(src)/2)) msg_buffer[i]='>';
    else if (i>=lcd_w/2+strlen(src)/2) msg_buffer[i]='<';
    else 
    {
      msg_buffer[i]=src[j];
      j++;
    }
  }
  msg_buffer[lcd_w]=0; // Terminate the string
  lcd->print(msg_buffer);
}

/**
 * \details This copies the right amount of text into a narrow space so it can be displayed and scrolled to show the entire message.
 * If you are not very interested in the inner working of this library, this is not for you. It is used to auto scroll long list items.
 * \param src This points to the long string that needs to be scrolled inside a narrow line.
 * \param dst This points to the buffer that has a length of the narrow line width + 1 (for 0 termination). The part of the text that will be displayed is copied to this buffer after the function call.
 * \param dst_len This is the width of the narrow line. Only this many characters from the source will be copied to the dst buffer.
 * \param pos This is the position of the long text. When pos=0, the text's first portion displays left justified in the dst buffer. When pos<0, spaces are padded before the text, useful for scrolling from right to left. When pos>dst_len, space is padded to the end of the text to make it scroll all the way to the left and eventually disappear. 
 * \return It returns the text to be displayed in the buffer and no value is directly returned.
 */
void scroll_text(char * src, char * dst, char dst_len, short pos)
{
  for (byte j=0;j<dst_len;j++)
  {
    if ((pos<0)||(pos>strlen(src)-1))
    {
      dst[j]=' ';
    }
    else dst[j]=src[pos];
    pos++;
  }
  dst[dst_len]=0;
}

/**
 * \details This copies the right amount of text (stored in PROGMEM) into a narrow space so it can be displayed and scrolled to show the entire message.
 * If you are not very interested in the inner working of this library, this is not for you. It is used to auto scroll long list items.
 * \param src This points to the long string that needs to be scrolled inside a narrow line.
 * \param dst This points to the buffer that has a length of the narrow line width + 1 (for 0 termination). The part of the text that will be displayed is copied to this buffer after the function call.
 * \param dst_len This is the width of the narrow line. Only this many characters from the source will be copied to the dst buffer.
 * \param pos This is the position of the long text. When pos=0, the text's first portion displays left justified in the dst buffer. When pos<0, spaces are padded before the text, useful for scrolling from right to left. When pos>dst_len, space is padded to the end of the text to make it scroll all the way to the left and eventually disappear. 
 * \return It returns the text to be displayed in the buffer and no value is directly returned.
 */
void scroll_text_P(PGM_P src, char * dst, char dst_len, short pos)
{
  for (byte j=0;j<dst_len;j++)
  {
    if ((pos<0)||(pos>strlen_P(src)-1))
    {
      dst[j]=' ';
    }
    else dst[j]=pgm_read_byte_near(src+pos);
    pos++;
  }
  dst[dst_len]=0;
}

/**
 * \details This is a quick and easy way to display a string in the PROGMEM to the LCD. Note the string should have a maximum of 20 characters.
 * \param msg_line This is the name of the char string stored in PROGMEM.
 */
void msg_lcd(char* msg_line)
{
  char msg_buffer[21];
  strcpy_P(msg_buffer,msg_line); 
  lcd->print(msg_buffer);
}

/**
 * \details Seeks previous line in a long message stored in SRAM. This seems easy until you start adding \n and \t etc. into the picture.
 * \param para This is the phi_prompt struct to carry information between callers and functions.
 * This function assumes the long message position has been always sought properly. Any improper movement of the long message pointer could produce unpredictable results.
 * If you are not interested in the inner working of this library, use text_area instead.
 * Return values are updated throught the struct.
 */
void prev_line(phi_prompt_struct* para)
{
  byte columns=para->step.c_arr[1];
  if (para->low.i<=0)
  {
    para->low.i=0;
    return;
  }
  if (para->ptr.msg[para->low.i-1]=='\n')
  { //Seek beginning of a paragraph.
    int dec=para->low.i-2;
    while(para->ptr.msg[dec]!='\n')
    {
      dec--;
      if (dec==0)
      {
        para->low.i=0;
        return;
      }
    }
    para->low.i-=((para->low.i-1-dec-1)%columns+1);
  }

  else para->low.i-=columns;
  return;
}

/**
 * \details Seeks next line in a long message stored in SRAM. This seems easy until you start adding \n and \t etc. into the picture.
 * \param para This is the phi_prompt struct to carry information between callers and functions.
 * This function assumes the long message position has been always sought properly. Any improper movement of the long message pointer could produce unpredictable results.
 * If you are not interested in the inner working of this library, use text_area instead.
 * Return values are updated throught the struct.
 */
void next_line(phi_prompt_struct* para)
{
  byte columns=para->step.c_arr[1];
  for (int i=para->low.i;i<para->low.i+columns;i++)
  {
    if (para->ptr.msg[i]=='\n')
    {
      para->low.i=i+1;
      return;
    }

    if (i==strlen(para->ptr.msg))
    {
      return;
    }
  }
  para->low.i+=columns;
}

/**
 * \details Seeks previous line in a long message stored in PROGMEM. This seems easy until you start adding \n and \t etc. into the picture.
 * \param para This is the phi_prompt struct to carry information between callers and functions.
 * This function assumes the long message position has been always sought properly. Any improper movement of the long message pointer could produce unpredictable results.
 * If you are not interested in the inner working of this library, use text_area instead.
 * Return values are updated throught the struct.
 */
void prev_line_P(phi_prompt_struct* para)
{
  byte columns=para->step.c_arr[1];
  if (para->low.i<=0)
  {
    para->low.i=0;
    return;
  }
  if (pgm_read_byte_near(para->ptr.msg_P+para->low.i-1)=='\n')
  { //Seek beginning of a paragraph.
    int dec=para->low.i-2;
    while(pgm_read_byte_near(para->ptr.msg_P+dec)!='\n')
    {
      dec--;
      if (dec==0)
      {
        para->low.i=0;
        return;
      }
    }
    para->low.i-=((para->low.i-1-dec-1)%columns+1);
  }

  else para->low.i-=columns;
  return;
}

/**
 * \details Seeks next line in a long message stored in PROGMEM. This seems easy until you start adding \n and \t etc. into the picture.
 * \param para This is the phi_prompt struct to carry information between callers and functions.
 * This function assumes the long message position has been always sought properly. Any improper movement of the long message pointer could produce unpredictable results.
 * If you are not interested in the inner working of this library, use text_area instead.
 * Return values are updated throught the struct.
 */
void next_line_P(phi_prompt_struct* para)
{
  byte columns=para->step.c_arr[1];
  for (int i=para->low.i;i<para->low.i+columns;i++)
  {
    if (pgm_read_byte_near(para->ptr.msg_P+i)=='\n')
    {
      para->low.i=i+1;
      return;
    }

    if (i==strlen_P(para->ptr.msg_P))
    {
      return;
    }
  }
  para->low.i+=columns;
}

/**
 * \details Displays a static long message stored in SRAM that could span multiple lines.
 * \param para This is the phi_prompt struct to carry information between callers and functions.
 * Option 0: just display message. Option 1: display message with a scrollbar to the right.
 * If you are not interested in the inner working of this library, use text_area instead.
 * Return values are updated throught the struct.
 */
void long_msg_lcd(phi_prompt_struct* para)
{
  byte columns=para->step.c_arr[1], rows=para->step.c_arr[0], ch, inc=0;
//  noBlink();
  for (byte i=0;i<rows;i++)
  {
    if ((para->low.i+inc>=strlen(para->ptr.msg))||(para->ptr.msg[para->low.i+inc]=='\n'))
    {
      ch=0;
      inc++;
    }
    else ch=para->ptr.msg[para->low.i+inc];
    setCursor(para->col,para->row+i);
    for (byte j=0;j<columns;j++)
    {
      if (ch==0) lcd->write(' ');
      else
      {
        lcd->write(ch);
        ch=para->ptr.msg[para->low.i+(++inc)];
        if ((ch=='\n')&&(j<columns-1))
        {
          ch=0;
          inc++;
        }
      }
    }
  }
  if (para->option==1)
  {
    scroll_bar_v(((long)para->low.i)*100/strlen(para->ptr.msg),para->col+columns,para->row,rows);
  }
}

/**
 * \details Displays a static long message stored in PROGMEM that could span multiple lines.
 * \param para This is the phi_prompt struct to carry information between callers and functions.
 * Option 0: just display message. Option 1: display message with a scrollbar to the right.
 * If you are not interested in the inner working of this library, use text_area instead.
 * Return values are updated throught the struct.
 */
void long_msg_lcd_P(phi_prompt_struct* para) // Displays a long message stored in PROGMEM that could span multiple lines.
{
  byte columns=para->step.c_arr[1], rows=para->step.c_arr[0], ch, inc=0;
//  noBlink();
  for (byte i=0;i<rows;i++)
  {
    if ((para->low.i+inc>=strlen_P(para->ptr.msg_P))||(pgm_read_byte_near(para->ptr.msg_P+para->low.i+inc)=='\n'))
    {
      ch=0;
      inc++;
    }
    else ch=pgm_read_byte_near(para->ptr.msg_P+para->low.i+inc);
    setCursor(para->col,para->row+i);
    for (byte j=0;j<columns;j++)
    {
      if (ch==0) lcd->write(' ');
      else
      {
        lcd->write(ch);
        ch=pgm_read_byte_near(para->ptr.msg_P+para->low.i+(++inc));
        if ((ch=='\n')&&(j<columns-1))
        {
          ch=0;
          inc++;
        }
      }
    }
  }

  if (para->option==1)
  {
    scroll_bar_v(((long)para->low.i)*100/strlen_P(para->ptr.msg_P),para->col+columns,para->row,rows);
  }
}


/**
 * \details Displays a scroll bar at column/row with height and percentage.
 * If you are not very interested in the inner working of this library, this is not for you.
 * This is the only function in phi_prompt library to use custom characters.
  * If you used custom characters and want to use this function or a long message with scroll bar, run the init again to reinitialize custom characters.
 * \param percentage This goes between 0 and 99, representing the location indicator on the bar.
 * \param column This is the column location of the scroll bar's top.
 * \param row This is the row location of the scroll bar's top.
 * \param v_height This is the height of the bar in number of rows.
 */
void scroll_bar_v(byte percent, byte column, byte row, byte v_height)
{
  int mapped;
  if (percent>99) percent=99;
  mapped=(int)(v_height*2-2)*percent/100; // This is mapped position, 2 per row of bar.
  for (byte i=0;i<v_height;i++)
  {
    setCursor(column,row+i);
    if (i==(mapped+1)/2)
    {
      if (i==0)
      {
        lcd->write((uint8_t)0);
      }
      else if (i==v_height-1)
      {
        lcd->write(5);
      }
      else
      {
        if (mapped+1==(mapped+1)/2*2) lcd->write(2);
        else lcd->write(3);
      }
    }
    else
    {
      if (i==0)
      {
        lcd->write(1);
      }
      else if (i==v_height-1)
      {
        lcd->write(4);
      }
      else
      {
        lcd->write(' ');
      }
    }
  }
}

/**
 * \details Displays a static list or menu stored in SRAM or PROGMEM that could span multiple lines.
 * \param para This is the phi_prompt struct to carry information between callers and functions.
 * Option is very extensive and please refer to the documentation and option table.
 * The options are combined with OR operation with "Render list option bits" you can find in the library include file.
 * To find out what each option does exactly, run phi_prompt_big_show demo code and try the options out and write the number down.
 * If you are not interested in the inner working of this library, use text_area instead.
 * \return If further update is needed, it returns 1. The caller needs to call it again to update display, such as scrolling item.
 * If it returns 0 then no further display update is needed and the caller can stop calling it.
 */
byte render_list(phi_prompt_struct* para)
{
  byte ret=0, _first_item, _last_item, columns=para->step.c_arr[1], rows=para->step.c_arr[0], item_per_screen=columns*rows, x1=para->col, y1=para->row, x2=para->step.c_arr[3], y2=para->step.c_arr[2]; // Which items to display.
  char list_buffer[22];
  long pos=millis()/500;

  if (para->option&phi_prompt_center_choice) // Determine first item on whether choice is displayed centered.
  {
    _first_item=para->low.i-item_per_screen/2;
    if (_first_item>127) _first_item=0;
    else if (para->low.i-item_per_screen/2+item_per_screen>para->high.i) _first_item=para->high.i+1-item_per_screen;
  }
  else
  {
    _first_item=(para->low.i/item_per_screen)*item_per_screen;
  }
    
  _last_item=_first_item+item_per_screen-1; // Determine last item based on first item, total item per screen, and total item.
  if (_last_item>para->high.i) _last_item=para->high.i;
  
  for (byte i=_first_item;i<_first_item+item_per_screen;i++)
  {
    if (i<=_last_item) // Copy item
    {
      if ((para->option&phi_prompt_list_in_SRAM))
      {
        if ((para->option&phi_prompt_auto_scroll)&&(i==para->low.i)&&strlen(*(para->ptr.list+i))>para->width) // Determine what portion of the item to be copied. In case of no auto scrolling, only first few characters are copied till the display buffer fills. In case of auto scrolling, a certain portion of the item is copied.
        {
          pos=pos%(strlen(*(para->ptr.list+i))+para->width)-para->width;
          scroll_text(*(para->ptr.list+i), list_buffer, para->width, pos);//Does the actual copy
          ret=1; // More update is needed to scroll text.
        }
        else //Does the actual truncation
        {
          byte len=strlcpy(list_buffer,*(para->ptr.list+i), para->width+1);
          if (len<para->width)
          {
            for (byte k=len;k<para->width;k++)
            {
              list_buffer[k]=' ';
            }
            list_buffer[para->width]=0;
          }
        }
      }
      else
      {
        if ((para->option&phi_prompt_auto_scroll)&&(i==para->low.i)&&strlen_P((char*)pgm_read_word(para->ptr.list+i))>para->width) // Determine what portion of the item to be copied. In case of no auto scrolling, only first few characters are copied till the display buffer fills. In case of auto scrolling, a certain portion of the item is copied.
        {
          pos=pos%(strlen_P((char*)pgm_read_word(para->ptr.list+i))+para->width)-para->width;
          scroll_text_P((char*)pgm_read_word(para->ptr.list+i), list_buffer, para->width, pos);//Does the actual copy
          ret=1; // More update is needed to scroll text.
        }
        else //Does the actual truncation
        {
          byte len=strlcpy_P(list_buffer,(char*)pgm_read_word(para->ptr.list+i), para->width+1);
          if (len<para->width)
          {
            for (byte k=len;k<para->width;k++)
            {
              list_buffer[k]=' ';
            }
            list_buffer[para->width]=0;
          }
        }
      }
    }
    else // Fill blank
    {
      byte j;
      for (j=0;j<para->width;j++)
      {
        list_buffer[j]=' ';
      }
      list_buffer[j]=0;
    }
//Display item on LCD
    setCursor(para->col+((i-_first_item)/rows)*(para->width+1), para->row+(i-_first_item)%rows);

    if (para->option&phi_prompt_arrow_dot) // Determine whether to render arrow and dot. In case of yes, the buffer is shifted to the right one character.       
    {
      if (i<=_last_item)
      {
      lcd->write((i==para->low.i)?indicator:bullet);// Show ">" or a dot
      }
      else
      {
        lcd->write(' ');
      }
    }
    lcd->print(list_buffer);
  }

  if (para->option&phi_prompt_index_list) // Determine whether to display 1234567890 index
  {
    setCursor(x2,y2);
    for (byte i=0;i<=para->high.i;i++)
    {
      if (i==para->low.i) lcd->write(indicator); // Display indicator on index
      else lcd->write(i%10+'1');
    }
  }
  
  else if (para->option&phi_prompt_current_total) // Determine whether to display current/total index
  {
    sprintf(list_buffer,"%c%d/%d", indicator,para->low.i+1, para->high.i+1);
    setCursor(x2,y2);
    lcd->print(list_buffer);// Prints index
  }
  
  if (para->option&phi_prompt_scroll_bar) // Determine whether to display scroll bar
  {
    scroll_bar_v(((int)para->low.i+1)*100/(para->high.i+1),para->col+columns*(para->width+1)-1*(!(para->option&phi_prompt_arrow_dot)),para->row,rows);
  }
  
  if (para->option&phi_prompt_flash_cursor) // Determine whether to display flashing cursor
  {
    setCursor(para->col+((para->low.i-_first_item)/rows)*(para->width+1), para->row+(para->low.i-_first_item)%rows);
    blink();
  }
  else noBlink();
  
  if (para->option&phi_prompt_invert_text) // Determine whether to invert highlighted item
  {
    
  }
  return ret;  
}
//------------------------------------------------------------------------------
void clear(){
  lcd->write(0xFE);  //command flag 
  lcd->write(0x01);  //clear command.
  delay(50);
}

void setCursor(int posNum, int lineNum){
  int delayTime = 50;
  int lcdPosition = 0;  // initialize lcdPosition 
  // posNum has to be within 1 to 20,
  // lineNum has to be within 1 to 4
    if (posNum < 21) {
      if (lineNum==0){
        lcdPosition = 128;
      } else if (lineNum==1){
        lcdPosition = 192;
      } else if (lineNum==2){
        lcdPosition = 148;
      } else if (lineNum==3){
        lcdPosition = 212;
    }
  } 
    if (lcdPosition > 0){
    // add to start of linepos to get the position 
      lcdPosition = lcdPosition + posNum;
      lcd->write(0xFE);  //command flag
      delay(delayTime);
      lcd->write(lcdPosition);   //position
      delay(delayTime);
    }
  }
  
void blink(){
  int delayTime = 50;
  lcd->write(0xFE); 
  lcd->write(0x0D); 
  delay(delayTime);
}

void noBlink(){
  int delayTime = 50;
  lcd->write(0xFE); 
  lcd->write(0x0C); 
  delay(delayTime);
}

void cursor(){
  int delayTime = 50;
  lcd->write(0xFE); 
  lcd->write(0x0E); 
  delay(delayTime); 
}

void noCursor(){
  int delayTime = 50;
  lcd->write(0xFE); 
  lcd->write(0x0C); 
  delay(delayTime);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void createChar(uint8_t location, uint8_t charmap[]) {
  location &= 0x7; // we only have 8 locations 0-7
  lcd->write(254);   //command flag
  lcd->write(64+location*8);  //clear command.
  for (int i=0; i<8; i++) {
    lcd->write(charmap[i]); 
    }
  }
//Interactions

/**
 * \details This function is the center of phi_prompt key sensing. It polls all input keypads for inputs for the length of ref_time in ms
 * If a key press is sensed, it attempts to translate it into function keys or pass the result unaltered if it is not a function key.
 * It only detects one key presses so holding multiple keys will not produce what you want.
 * For function key codes, refer to the "Internal function key codes" section in the library header.
 * \param ref_time This is the time the function is allowed to wait, while polling on all input keypads.
 * \return Returns translated key or NO_KEY if no key is detected after the time expires.
 */
int wait_on_escape(int ref_time)
{
//Wait on button push.
  long temp0;
  byte temp1;
  temp0=millis();
  do
  {
    byte i=0;
    while(mbi_ptr[i])
    {
      temp1=mbi_ptr[i]->getKey();
      if (temp1!=NO_KEY) return (phi_prompt_translate(temp1));
      i++;
    }
  }   while ((millis()-temp0<ref_time));

  return (NO_KEY);
}

//Inputs
/**
 * \details Input an integer value with wrap-around capability. Integers are inputted with up and down function keys. The value has upper and lower limits and step.
 * Pressing number keys has no effect since the input needs to be restricted with limits and steps.
 * This function prints the initial value first so the caller doesn't need to.
 * Function traps until the update is finalized by the left, right, enter button or escape button.
 * Return values are updated throught the pointer.
 * \param para This is the phi_prompt struct to carry information between callers and functions.
 * Display options for integers:
 * 0: space pad right, 1: zero pad left, 2: space pad left.
 * \return Returns function keys pushed so the caller can determine what to do:
 * Go back to the last slot with left (-3)
 * Go forward to the next slot with right (-4)
 * Enter(1)
 * Escape(-1).
 */
int input_integer(phi_prompt_struct *para)
{
  
  int number=*(para->ptr.i_buffer);
  byte temp1;
  byte space=para->width;
  char msg[space+1];
  char format[]="%00d";
  for (byte i=0;i<space;i++) msg[i]=' '; // Create mask the size of the output space.
  msg[space]=0;
  setCursor(para->col,para->row); // Write mask to erase previous content.
  lcd->print(msg);
  setCursor(para->col,para->row); // Write initial value.
  switch (para->option) // Prints out the content once before accepting user inputs.
  {
    case 0://padding with space to the right (padding done in erasing phase)
    sprintf(msg,"%d",number);
    break;

    case 1://padding with zero to the right
    format[2]+=space;
    sprintf(msg,format,number);
    break;
  }

  lcd->print(msg);
  setCursor(para->col,para->row); // Write initial value.
  cursor();

  while(true)
  {
    temp1=wait_on_escape(50);
    switch (temp1)
    {
      case phi_prompt_up:
      if (number+(para->step.i)<=(para->high.i)) number+=(para->step.i);
      else number=para->low.i;
      setCursor(para->col,para->row);
      for (byte i=0;i<space;i++) msg[i]=' '; // Create mask the size of the output space.
      msg[space]=0;
      lcd->print(msg);
      setCursor(para->col,para->row);
      
      switch (para->option)
      {
        case 0://padding with space to the right (padding done in erasing phase)
        sprintf(msg,"%d",number);
        break;

        case 1://padding with zero to the right
        format[2]='0'+space;
        sprintf(msg,format,number);
        break;
      }

      lcd->print(msg);
      setCursor(para->col,para->row);
      break;
      
      case phi_prompt_down:
      if (number-para->step.i>=para->low.i) number-=para->step.i;
      else number=para->high.i;
      setCursor(para->col,para->row);
      for (byte i=0;i<space;i++) msg[i]=' '; // Create mask the size of the output space.
      msg[space]=0;
      lcd->print(msg);
      setCursor(para->col,para->row);

      switch (para->option)
      {
        case 0://padding with space to the right (padding done in erasing phase)
        sprintf(msg,"%d",number);
        break;

        case 1://padding with zero to the right
        format[2]='0'+space;
        sprintf(msg,format,number);
        break;
      }

      lcd->print(msg);
      setCursor(para->col,para->row);
      break;
      
      case phi_prompt_left: // Left is pressed
      *(para->ptr.i_buffer)=number;
      noCursor();
      return (-3);
      break;
      
      case phi_prompt_right: // Right is pressed
      *(para->ptr.i_buffer)=number;
      noCursor();
      return (-4);
      break;
      
      case phi_prompt_enter: // Enter is pressed
      *(para->ptr.i_buffer)=number;
      noCursor();
      return(1);
      break;
      
      case phi_prompt_escape: // Escape is pressed
      noCursor();
      return (-1);
      break;
      
      default:
      break;
    }
  }
}

/*
This is still supported but not number keypad support has not been expanded to it and it will disappear from the library in future releases.
Input a floating point value with fixed decimal point. Ironic but true.
This function prints the initial value first so the caller doesn't need to.
Function traps until the update is finalized by the enter button or escape button.
Return values are updated throught the pointer.
Returns buttons pushed so the caller can determine what to do:
Go back to the last slot with left (-3)
Go forward to the next slot with right (-4)
Enter(1)
Escape(-1).
Display options for floats:
0: only positive numbers allowed, 1: only negative numbers allowed, 2: both positive and negative numbers are allowed.
*/
int input_float(phi_prompt_struct *para)
{
  phi_prompt_struct myTextInput;
  char number[17]; // This is the buffer that will store the content of the text panel.
  char format[]="%04d.%02d";
  byte before=para->step.c_arr[1], after=para->step.c_arr[0]; // How many digits to display before and after decimal point.
  int ret;
  format[2]=before+'0';
  format[7]=after+'0';
  sprintf(number,format,int(*para->ptr.f_buffer),int((*para->ptr.f_buffer-int(*para->ptr.f_buffer))*pow(10,after)*(*para->ptr.f_buffer>0?1:-1)+0.5)); // Form the string to represent current value. This string will be the default value of the float input.

  myTextInput.ptr.msg=number; // Assign the text buffer address
  myTextInput.width=before+after+1; // Length of the input panel is 12 characters.
  myTextInput.col=para->col; // Display input panel at required column
  myTextInput.row=para->row; // Display input panel at required row
  switch (para->option)
  {
    case 0:
    myTextInput.low.c='0'; // Text panel valid input starts with character '-'.
    myTextInput.high.c='9'; // Text panel valid input ends with character '-'. Could end with '.' if real floating number is enabled.
    myTextInput.option=0; // Option 1 incluess 0-9 as valid characters. Option 0, default, option 1 include 0-9 as valid inputs.
    break;

    case 1:
    number[0]='-';
    myTextInput.low.c='0'; // Text panel valid input starts with character '-'.
    myTextInput.high.c='9'; // Text panel valid input ends with character '-'. Could end with '.' if real floating number is enabled.
    myTextInput.option=0; // Option 1 incluess 0-9 as valid characters. Option 0, default, option 1 include 0-9 as valid inputs.
    break;

    case 2:
    myTextInput.low.c='-'; // Text panel valid input starts with character '-'.
    myTextInput.high.c='-'; // Text panel valid input ends with character '-'. Could end with '.' if real floating number is enabled.
    myTextInput.option=1; // Option 1 incluess 0-9 as valid characters. Option 0, default, option 1 include 0-9 as valid inputs.
    break;

    default:
    break;
  }
  
  ret=input_panel(&myTextInput);
  if (ret!=-1)
  {  
    sscanf(number,"%d%*c%d",&myTextInput.high.i,&myTextInput.low.i);
    *para->ptr.f_buffer=(-1)*(myTextInput.high.i<0?1:-1)*myTextInput.low.i/pow(10.0,(float)after)+myTextInput.high.i;
  }
  return ret;  
}

/**
 * \details Select from a list with wrap-around capability.
 * \param para This is the phi_prompt struct to carry information between callers and functions.
 * \par Display option
 * Option 0: display classic list, option 1: display MXN list, option 2: display list with index, option 3: display list with index2, option 4: display MXN list with scrolling
 *
 * This function prints the initial value first so the caller doesn't need to.
 * Function traps until the update is finalized by the left, right, enter button or escape button.
 * Return values are updated throught the pointer.
 * \return Returns buttons pushed so the caller can determine what to do. The function returns 1 if enter is pressed, -1 if the input is cancelled.
 */
int select_list(phi_prompt_struct *para)
{
  int temp1;
  byte render, width=para->width;
  char msg[width+1];
  render=render_list(para);
  while(true)
  {
    temp1=wait_on_escape(50);
    byte columns=para->step.c_arr[1], rows=para->step.c_arr[0];
    switch (temp1)
    {
      case NO_KEY:
      if (render)
      {
        render=render_list(para);
      }
      break;
      
      case phi_prompt_up: ///< Up is pressed. Move to the previous item.
      if (para->low.i-1>=0) para->low.i--;
      else para->low.i=para->high.i;
      render=render_list(para);
      break;
      
      case phi_prompt_down: ///< Down is pressed. Move to the next item.
      if ((para->low.i+1)<=(para->high.i)) para->low.i++;
      else para->low.i=0;
      render=render_list(para);
      break;
      
      case phi_prompt_left: ///< Left is pressed
      if (para->low.i-para->row>=0) para->low.i-=para->row;
      render=render_list(para);
      break;
      
      case phi_prompt_right: ///< Right is pressed
      if (para->low.i+para->row<=para->high.i) para->low.i+=para->row;
      render=render_list(para);
      break;
      
      case phi_prompt_enter: ///< Enter is pressed
      noCursor();
      return(1);
      break;
      
      case phi_prompt_escape: ///< Escape is pressed
      noCursor();
      return (-1);
      break;
      
      default: ///< Any other keys will flip the list to the next page.
      if ((para->low.i+columns*rows)<=(para->high.i)) para->low.i+=columns*rows;
      else if (para->low.i==para->high.i) para->low.i=0;
      else para->low.i=para->high.i;
      render=render_list(para);
      break;
    }
  }
}

/**
 * \details Alphanumerical input panel for texts up to 16 characters.
 * \param para This is the phi_prompt struct to carry information between callers and functions.
 * If you input the characters with up/down keys, then each character is restricted to start with the character _start and end with character _end.
 * Since up/down key input cycles through all characters, it is slow. This way with _start and _end characters, you don't have to cycle through all characters.
 * If you use a larger keypad to input, such as matrix keypad or multi-tap enabled matrix keypad, the restriction won't apply.
 * The total length will not exceed length.
 * The first letter appears at column, row.
 * The function prints the content of the buffer msg before polling for input.
 * The function fills the buffer after the last character with \0 only if the buffer is not filled. The caller is responsible to fill the character beyond the end of the buffer with \0.
 * Input character options for input panel:
 * Option 0: default, option 1: include 0-9 as valid inputs, option 2: only 0-9 are valid, option 3: only 0-9 are valid and the first digit can be '-', option 4: only 0-9 are valid and number increments, option 5: only 0-9 are valid and the first digit can be '-' and number increments.
 * To do:
 * Option 2-4, automatically increment or decrement a whole series of numbers when they are connected, such as File0009.txt when increase 9 (if only 0-9 are allowed) will give File0010.txt.
 * \return The function returns number of actual characters. The function returns -1 if the input is cancelled.
 */
int input_panel(phi_prompt_struct *para)
{
  byte pointer=0, chr;
  int temp1;
  setCursor(para->col,para->row);
  lcd->print(para->ptr.msg);
  setCursor(para->col,para->row);
  cursor();
  while(true)
  {
    temp1=wait_on_escape(50);
    chr=*(para->ptr.msg+pointer); // Loads the current character.
    switch (temp1)
    {
      case phi_prompt_up:
      *(para->ptr.msg+pointer)=inc(chr, para);
      lcd->write(*(para->ptr.msg+pointer));
      setCursor(pointer+para->col,para->row);
      
      //do automatic increment
      break;
      
      case phi_prompt_down:
      *(para->ptr.msg+pointer)=dec(chr, para);
      lcd->write(*(para->ptr.msg+pointer));
      setCursor(pointer+para->col,para->row);
      
      //do automatic decrement
      break;
      
      case phi_prompt_left: // Left is pressed
      if (pointer>0)
      {
        pointer--;
        setCursor(pointer+para->col,para->row);
      }
      else
      {
        noCursor();
        return (-3);
      }
      break;
      
      case '\b': ///< Back space is pressed. Erase the current character with space and back up one character.
      *(para->ptr.msg+pointer)=' ';
      lcd->write(*(para->ptr.msg+pointer));
      if (pointer>0)
      {
        pointer--;
      }
      setCursor(pointer+para->col,para->row);
      break;
      
      case phi_prompt_right: ///< Right is pressed
      if (pointer<(para->width)-1)
      {
        pointer++;
        setCursor(pointer+para->col,para->row);
      }
      else
      {
        noCursor();
        return (-4);
      }
      break;
      
      case phi_prompt_enter: ///< Enter is pressed
      case '\n': ///< New line is received
      noCursor();
      return(1);
      break;
      
      case phi_prompt_escape: ///< Escape is pressed
      noCursor();
      return (-1);
      break;
      
      default: ///< Other keys were pressed
      if (temp1==NO_KEY) break;
      *(para->ptr.msg+pointer)=temp1;
      lcd->write(*(para->ptr.msg+pointer));
      if (pointer<(para->width)-1)
      {
        pointer++;
      }
      setCursor(pointer+para->col,para->row);
      break;
    }
  }
}

/**
 * \details Numerical input panel for a number up to 16 characters. This function is for keypads with numerical keys only. If you only have up/down keys, please use input_integer().
 * \param para This is the phi_prompt struct to carry information between callers and functions.
 * 
 * Up key outputs a negative sign and moves cursor to the right.
 * Down key outputs a decimal sign and moves cursor to the right.
 * If you use a larger keypad to input, such as matrix keypad or multi-tap enabled matrix keypad, the restriction won't apply.
 * The total length will not exceed length.
 * The first letter appears at column, row.
 * The function prints the content of the buffer msg before polling for input.
 * The function fills the buffer after the last character with \0 only if the buffer is not filled. The caller is responsible to fill the character beyond the end of the buffer with \0.
 * After the function returns, you will need to use sscanf to convert the string into number. See example code.
 * \return The function returns number of actual characters. The function returns -1 if the input is cancelled.
 */
int input_number(phi_prompt_struct *para)
{
  byte pointer=0, chr;
  int temp1;
  setCursor(para->col,para->row);
  lcd->print(para->ptr.msg);
  setCursor(para->col,para->row);
  cursor();
  while(true)
  {
    temp1=wait_on_escape(50);
    chr=*(para->ptr.msg+pointer); // Loads the current character.
    switch (temp1)
    {
      case phi_prompt_up: ///< Up key outputs a negative sign and moves cursor to the right.
      *(para->ptr.msg+pointer)='-';
      lcd->write(*(para->ptr.msg+pointer));
      if (pointer<(para->width)-1)
      {
        pointer++;
      }
      setCursor(pointer+para->col,para->row);
      break;
      
      case phi_prompt_down:
      *(para->ptr.msg+pointer)='.'; ///< Down key outputs a decimal sign and moves cursor to the right.
      lcd->write(*(para->ptr.msg+pointer));
      if (pointer<(para->width)-1)
      {
        pointer++;
      }
      setCursor(pointer+para->col,para->row);
      break;
      
      case phi_prompt_left: // Left is pressed
      if (pointer>0)
      {
        pointer--;
        setCursor(pointer+para->col,para->row);
      }
      else
      {
        noCursor();
        return (-3);
      }
      break;
      
      case '\b': ///< Back space is pressed. Erase the current character with space and back up one character.
      *(para->ptr.msg+pointer)=' ';
      lcd->write(*(para->ptr.msg+pointer));
      if (pointer>0)
      {
        pointer--;
      }
      setCursor(pointer+para->col,para->row);
      break;
      
      case phi_prompt_right: ///< Right is pressed
      if (pointer<(para->width)-1)
      {
        pointer++;
        setCursor(pointer+para->col,para->row);
      }
      else
      {
        noCursor();
        return (-4);
      }
      break;
      
      case phi_prompt_enter: ///< Enter is pressed
      case '\n': ///< New line is received
      noCursor();
      return(1);
      break;
      
      case phi_prompt_escape: ///< Escape is pressed
      noCursor();
      return (-1);
      break;
      
      default: ///< Other keys were pressed. Accept only number keys.
      if ((temp1>='0')&&(temp1<='9'))
      {
        *(para->ptr.msg+pointer)=temp1;
        lcd->write(*(para->ptr.msg+pointer));
        if (pointer<(para->width)-1)
        {
          pointer++;
        }
        setCursor(pointer+para->col,para->row);
      }
      break;
    }
  }
}

/**
 * \details Displays a text area using message stored in the SRAM.
 * \param para This is the phi_prompt struct to carry information between callers and functions.
 * It traps execution. It monitors the key pad input and changes message position with up and down keys.
 * When the user presses left and right keys, the message scrolls up or down one page, which is 2 lines if you display a 3-line message or 3 lines if you display a 4-line message.
 * If the user presses 1-9 number keys, the function returns these numbers in ASCII for simple list select functions.
 * \return '1'-'9' if the user presses one of these keys. Enter was pressed (1), Escape was pressed (-1).
 */
int text_area(phi_prompt_struct *para)
{
  byte columns=para->step.c_arr[1], rows=para->step.c_arr[0], ch;
  long_msg_lcd(para);
  while(true)
  {
    byte temp1=wait_on_escape(50);
    switch (temp1)
    {
      case phi_prompt_up:
      prev_line(para);
      long_msg_lcd(para);
      break;
      
      case phi_prompt_down:
      next_line(para);
      long_msg_lcd(para);
      break;

      case phi_prompt_left: ///< Left is pressed. Scroll up one page, which is total_row-1 lines.
      for (byte i=0;i<para->step.c_arr[0]-1;i++) prev_line(para);
      long_msg_lcd(para);
      break;
      
      case phi_prompt_right: ///< Right is pressed.  Scroll down one page, which is total_row-1 lines.
      for (byte i=0;i<para->step.c_arr[0]-1;i++) next_line(para);
      long_msg_lcd(para);
      break;
      
      case phi_prompt_enter: ///< Enter is pressed
      return(1);
      break;
      
      case phi_prompt_escape: ///< Escape is pressed
      return (-1);
      break;
      
      default:
      if ((temp1>='1')&&(temp1<='9')) return (temp1); ///< Returns numbers for simple select lists.
      break;
    }
  }
}

/**
 * \details Displays a text area using message stored in PROGMEM.
 * \param para This is the phi_prompt struct to carry information between callers and functions.
 * It traps execution. It monitors the key pad input and changes message position with up and down keys.
 * When the user presses left and right keys, the message scrolls up or down one page, which is 2 lines if you display a 3-line message or 3 lines if you display a 4-line message.
 * If the user presses 1-9 number keys, the function returns these numbers in ASCII for simple list select functions.
 * \return '1'-'9' if the user presses one of these keys. Enter was pressed (1), Escape was pressed (-1).
 */
int text_area_P(phi_prompt_struct *para) // Displays a text area using message stored in the PROGMEM
{
  byte columns=para->step.c_arr[1], rows=para->step.c_arr[0], ch;
  long_msg_lcd_P(para);
  while(true)
  {
    byte temp1=wait_on_escape(50);
    switch (temp1)
    {
      case phi_prompt_up:
      prev_line_P(para);
      long_msg_lcd_P(para);
      break;
      
      case phi_prompt_down:
      next_line_P(para);
      long_msg_lcd_P(para);
      break;

      case phi_prompt_left: ///< Left is pressed. Scroll up one page, which is total_row-1 lines.
      for (byte i=0;i<para->step.c_arr[0]-1;i++) prev_line_P(para);
      long_msg_lcd_P(para);
      break;
      
      case phi_prompt_right: ///< Right is pressed.  Scroll down one page, which is total_row-1 lines.
      for (byte i=0;i<para->step.c_arr[0]-1;i++) next_line_P(para);
      long_msg_lcd_P(para);
      break;
      
      case phi_prompt_enter: ///< Enter is pressed
      return(1);
      break;
      
      case phi_prompt_escape: ///< Escape is pressed
      return (-1);
      break;
      
      default:
      if ((temp1>='1')&&(temp1<='9')) return (temp1); ///< Returns numbers for simple select lists.
      break;
    }
  }
}

/**
 * \details Displays a short message with yes/no options.
 * \param msg This is the message to be displayed with the yes/no choice.
 * It traps execution. It monitors the key pad input and toggles between yes and no with up and down keys.
 * The function auto scales to fit on any LCD.
 * \return Returns 0 for yes choice 1 for no choice, -1 if Escape was pressed.
 */
int yn_dialog(char msg[])
{
  phi_prompt_struct yn_list;
  
  clear(); // Clear the lcd.
// Use the yn_list struct to display the message as a long message to enable multiple line question.
  yn_list.ptr.msg=msg; // Assign the address of the text string to the pointer.
  yn_list.low.i=0; // Default text starting position. 0 is highly recommended.
  yn_list.high.i=((sizeof(msg)-1)>lcd_w*lcd_h-10)?lcd_w*lcd_h-10:(sizeof(msg)-1); // Position of the last character in the text string, which is size of the string - 1.
  yn_list.step.c_arr[0]=lcd_h; // row
  yn_list.step.c_arr[1]=lcd_w; // column
  yn_list.col=0; // Display the text area starting at column 0
  yn_list.row=0; // Display the text area starting at row 0
  yn_list.option=0; // Option 0, display classic message, option 1, display message with scroll bar on right.

  long_msg_lcd(&yn_list);

  yn_list.ptr.list=(char**)&yn_items; // Assign the address of the list array to the pointer.
  yn_list.low.i=0; // Default item highlighted on the list is #0, the first item on the list.
  yn_list.high.i=1; // Last item on the list is size of the list - 1.
  yn_list.width=9; // Length in characters of the longest list item, for screen update purpose.
  yn_list.col=lcd_w-9; // Display the list a column to make it right-alighed
  yn_list.row=lcd_h-1; // Display the list at a row to make it bottom-alighed
  yn_list.step.c_arr[0]=1; // 1 row
  yn_list.step.c_arr[1]=1; // 1 column
  yn_list.option=0; // Option 3 is an indexed list for clarity. Option 0, display classic list, option 1, display MXN list, option 2, display list with index, option 3, display list with index2.

  if (select_list(&yn_list)!=-1) return yn_list.low.i; //If the user didn't press escape (return -1) then update the user choice with the value in myListInput.low.
}

/**
 * \details Displays a short message with OK.
 * \param msg This is the message to be displayed with the OK button.
 * It traps execution. It monitors the key pad input and returns if any key is pressed.
 * The function auto scales to fit on any LCD.
 * \return It returns no value but for compatibility it is defined to return integer.
 */
int ok_dialog(char msg[])
{
  phi_prompt_struct yn_list;
  
  clear(); // Clear the lcd.
// Use the yn_list struct to display the message as a long message to enable multiple line question.
  yn_list.ptr.msg=msg; // Assign the address of the text string to the pointer.
  yn_list.low.i=0; // Default text starting position. 0 is highly recommended.
  yn_list.high.i=((sizeof(msg)-1)>lcd_w*lcd_h-5)?lcd_w*lcd_h-5:(sizeof(msg)-1); // Position of the last character in the text string, which is size of the string - 1.
  yn_list.step.c_arr[0]=lcd_h; // row
  yn_list.step.c_arr[1]=lcd_w; // column
  yn_list.col=0; // Display the text area starting at column 0
  yn_list.row=0; // Display the text area starting at row 0
  yn_list.option=0; // Option 0, display classic message, option 1, display message with scroll bar on right.

  long_msg_lcd(&yn_list);

  setCursor((lcd_w-4),lcd_h-1);
  lcd->print(">OK<");
  
  while(wait_on_escape(500)==0)
  {
  }
}
