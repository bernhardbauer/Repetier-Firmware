/*
    This file is part of the Repetier-Firmware for RF devices from Conrad Electronic SE.

    Repetier-Firmware is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Repetier-Firmware is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Repetier-Firmware.  If not, see <http://www.gnu.org/licenses/>.
*/


#define UI_MAIN
#include "Repetier.h"
extern const int8_t encoder_table[16] PROGMEM ;

#include <math.h>
#include <stdlib.h>
#include <inttypes.h>
#include <ctype.h>


#if BEEPER_TYPE==2 && defined(UI_HAS_I2C_KEYS) && UI_I2C_KEY_ADDRESS!=BEEPER_ADDRESS
#error Beeper address and i2c key address must be identical
#else
#if BEEPER_TYPE==2
#define UI_I2C_KEY_ADDRESS BEEPER_ADDRESS
#endif // BEEPER_TYPE==2
#endif // BEEPER_TYPE==2 && defined(UI_HAS_I2C_KEYS) && UI_I2C_KEY_ADDRESS!=BEEPER_ADDRESS

#if UI_PRINT_AUTORETURN_TO_MENU_AFTER || UI_MILL_AUTORETURN_TO_MENU_AFTER
millis_t g_nAutoReturnTime       = 0;
bool     g_nAutoReturnMessage    = false;
#endif // UI_PRINT_AUTORETURN_TO_MENU_AFTER || UI_MILL_AUTORETURN_TO_MENU_AFTER

char    g_nYesNo                 = 0;       // 0 = no, 1 = yes
volatile char    g_nContinueButtonPressed = 0;
char    g_nServiceRequest        = 0;
char    g_nPrinterReady          = 0;

void beep(uint8_t duration,uint8_t count)
{
#if FEATURE_BEEPER
    if( !Printer::enableBeeper )
    {
        // we shall not beep
        return;
    }

#if BEEPER_TYPE!=0
#if BEEPER_TYPE==1 && defined(BEEPER_PIN) && BEEPER_PIN>=0
    SET_OUTPUT(BEEPER_PIN);
#endif //  BEEPER_TYPE==1 && defined(BEEPER_PIN) && BEEPER_PIN>=0

#if BEEPER_TYPE==2
    HAL::i2cStartWait(BEEPER_ADDRESS+I2C_WRITE);
#if UI_DISPLAY_I2C_CHIPTYPE==1
    HAL::i2cWrite( 0x14); // Start at port a
#endif // UI_DISPLAY_I2C_CHIPTYPE==1
#endif // BEEPER_TYPE==2

    for(uint8_t i=0; i<count; i++)
    {
#if BEEPER_TYPE==1 && defined(BEEPER_PIN) && BEEPER_PIN>=0
#if defined(BEEPER_TYPE_INVERTING) && BEEPER_TYPE_INVERTING
        WRITE(BEEPER_PIN,LOW);
#else
        WRITE(BEEPER_PIN,HIGH);
#endif // defined(BEEPER_TYPE_INVERTING) && BEEPER_TYPE_INVERTING
#else
#if UI_DISPLAY_I2C_CHIPTYPE==0
#if BEEPER_ADDRESS == UI_DISPLAY_I2C_ADDRESS
        HAL::i2cWrite(uid.outputMask & ~BEEPER_PIN);
#else
        HAL::i2cWrite(~BEEPER_PIN);
#endif // BEEPER_ADDRESS == UI_DISPLAY_I2C_ADDRESS
#endif // UI_DISPLAY_I2C_CHIPTYPE==0

#if UI_DISPLAY_I2C_CHIPTYPE==1
        HAL::i2cWrite((BEEPER_PIN) | uid.outputMask);
        HAL::i2cWrite(((BEEPER_PIN) | uid.outputMask)>>8);
#endif // UI_DISPLAY_I2C_CHIPTYPE==1
#endif // BEEPER_TYPE==1 && defined(BEEPER_PIN) && BEEPER_PIN>=0

        HAL::delayMilliseconds(duration);

#if BEEPER_TYPE==1 && defined(BEEPER_PIN) && BEEPER_PIN>=0
#if defined(BEEPER_TYPE_INVERTING) && BEEPER_TYPE_INVERTING
        WRITE(BEEPER_PIN,HIGH);
#else
        WRITE(BEEPER_PIN,LOW);
#endif // defined(BEEPER_TYPE_INVERTING) && BEEPER_TYPE_INVERTING
#else
#if UI_DISPLAY_I2C_CHIPTYPE==0

#if BEEPER_ADDRESS == UI_DISPLAY_I2C_ADDRESS
        HAL::i2cWrite((BEEPER_PIN) | uid.outputMask);
#else
        HAL::i2cWrite(255);
#endif // BEEPER_ADDRESS == UI_DISPLAY_I2C_ADDRESS
#endif // UI_DISPLAY_I2C_CHIPTYPE==0

#if UI_DISPLAY_I2C_CHIPTYPE==1
        HAL::i2cWrite( uid.outputMask);
        HAL::i2cWrite(uid.outputMask>>8);
#endif // UI_DISPLAY_I2C_CHIPTYPE==1
#endif // BEEPER_TYPE==1 && defined(BEEPER_PIN) && BEEPER_PIN>=0

        HAL::delayMilliseconds(duration);
    }

#if BEEPER_TYPE==2
    HAL::i2cStop();
#endif // BEEPER_TYPE==2
#endif // BEEPER_TYPE!=0
#endif // FEATURE_BEEPER

} // beep


bool UIMenuEntry::showEntry() const
{
    bool    ret = true;
    uint8_t f, f2;


    f = HAL::readFlashByte((const prog_char*)&filter);
    if(f!=0)
        ret = (f & Printer::menuMode) != 0;
    f2 = HAL::readFlashByte((const prog_char*)&nofilter);
    if(ret && f2!=0)
    {
        ret = (f2 & Printer::menuMode) == 0;
    }
    return ret;

} // showEntry


#if UI_DISPLAY_TYPE!=0
UIDisplay uid;
char displayCache[UI_ROWS][MAX_COLS+1];

// Menu up sign - code 1
// ..*.. 4
// .***. 14
// *.*.* 21
// ..*.. 4
// ***.. 28
// ..... 0
// ..... 0
// ..... 0
const uint8_t character_back[8] PROGMEM = {4,14,21,4,28,0,0,0};

// Degrees sign - code 2
// ..*.. 4
// .*.*. 10
// ..*.. 4
// ..... 0
// ..... 0
// ..... 0
// ..... 0
// ..... 0
const uint8_t character_degree[8] PROGMEM = {4,10,4,0,0,0,0,0};

// selected - code 3
// ..... 0
// ***** 31
// ***** 31
// ***** 31
// ***** 31
// ***** 31
// ***** 31
// ..... 0
// ..... 0
const uint8_t character_selected[8] PROGMEM = {0,31,31,31,31,31,0,0};

// unselected - code 4
// ..... 0
// ***** 31
// *...* 17
// *...* 17
// *...* 17
// *...* 17
// ***** 31
// ..... 0
// ..... 0
const uint8_t character_unselected[8] PROGMEM = {0,31,17,17,17,31,0,0};

// unselected - code 5
// ..*.. 4
// .*.*. 10
// .*.*. 10
// .*.*. 10
// .*.*. 10
// .***. 14
// ***** 31
// ***** 31
// .***. 14
const uint8_t character_temperature[8] PROGMEM = {4,10,10,10,14,31,31,14};

// unselected - code 6
// ..... 0
// ***.. 28
// ***** 31
// *...* 17
// *...* 17
// ***** 31
// ..... 0
// ..... 0
const uint8_t character_folder[8] PROGMEM = {0,28,31,17,17,31,0,0};

// printer ready - code 7
// *...* 17
// .*.*. 10
// ..*.. 4
// *...* 17
// ..*.. 4
// .*.*. 10
// *...* 17
// *...* 17
const byte character_ready[8] PROGMEM = {17,10,4,17,4,10,17,17};

const long baudrates[] PROGMEM = {9600,14400,19200,28800,38400,56000,57600,76800,111112,115200,128000,230400,250000,256000,
                                  460800,500000,921600,1000000,1500000,0
                                 };


const byte c1[8] PROGMEM = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00001,
  B00010,
  B00011,
  B00011
};

const byte c2[8] PROGMEM = {
  B00000,
  B00000,
  B00101,
  B01011,
  B01111,
  B10111,
  B11111,
  B11110
};

const byte c3[8] PROGMEM = {
  B00000,
  B01001,
  B10110,
  B01111,
  B11111,
  B11111,
  B00111,
  B00011
};

const byte c4[8] PROGMEM = {
  B01111,
  B01011,
  B01111,
  B01111,
  B10111,
  B10111,
  B01111,
  B10111
};

const byte c7[8] PROGMEM = {
  B01111,
  B10111,
  B01011,
  B00111,
  B00001,
  B00011,
  B00000,
  B00000
};

const byte c8[8] PROGMEM = {
  B00000,
  B11000,
  B11000,
  B11100,
  B11110,
  B11111,
  B11111,
  B00111
};

const byte c9[8] PROGMEM = {
  B00000,
  B00000,
  B00001,
  B00011,
  B10111,
  B11111,
  B11111,
  B11110
};

bool normalchars = false;

#define LCD_ENTRYMODE       0x04                    /**< Set entrymode */

/** @name GENERAL COMMANDS */
/*@{*/
#define LCD_CLEAR           0x01                    /**< Clear screen */
#define LCD_HOME            0x02                    /**< Cursor move to first digit */
/*@}*/

/** @name ENTRYMODES */
/*@{*/
#define LCD_ENTRYMODE       0x04                    /**< Set entrymode */
#define LCD_INCREASE        LCD_ENTRYMODE | 0x02    /**<    Set cursor move direction -- Increase */
#define LCD_DECREASE        LCD_ENTRYMODE | 0x00    /**<    Set cursor move direction -- Decrease */
#define LCD_DISPLAYSHIFTON  LCD_ENTRYMODE | 0x01    /**<    Display is shifted */
#define LCD_DISPLAYSHIFTOFF LCD_ENTRYMODE | 0x00    /**<    Display is not shifted */
/*@}*/

/** @name DISPLAYMODES */
/*@{*/
#define LCD_DISPLAYMODE     0x08                    /**< Set displaymode */
#define LCD_DISPLAYON       LCD_DISPLAYMODE | 0x04  /**<    Display on */
#define LCD_DISPLAYOFF      LCD_DISPLAYMODE | 0x00  /**<    Display off */
#define LCD_CURSORON        LCD_DISPLAYMODE | 0x02  /**<    Cursor on */
#define LCD_CURSOROFF       LCD_DISPLAYMODE | 0x00  /**<    Cursor off */
#define LCD_BLINKINGON      LCD_DISPLAYMODE | 0x01  /**<    Blinking on */
#define LCD_BLINKINGOFF     LCD_DISPLAYMODE | 0x00  /**<    Blinking off */
/*@}*/

/** @name SHIFTMODES */
/*@{*/
#define LCD_SHIFTMODE       0x10                    /**< Set shiftmode */
#define LCD_DISPLAYSHIFT    LCD_SHIFTMODE | 0x08    /**<    Display shift */
#define LCD_CURSORMOVE      LCD_SHIFTMODE | 0x00    /**<    Cursor move */
#define LCD_RIGHT           LCD_SHIFTMODE | 0x04    /**<    Right shift */
#define LCD_LEFT            LCD_SHIFTMODE | 0x00    /**<    Left shift */
/*@}*/

/** @name DISPLAY_CONFIGURATION */
/*@{*/
#define LCD_CONFIGURATION   0x20                        /**< Set function */
#define LCD_8BIT            LCD_CONFIGURATION | 0x10    /**<    8 bits interface */
#define LCD_4BIT            LCD_CONFIGURATION | 0x00    /**<    4 bits interface */
#define LCD_2LINE           LCD_CONFIGURATION | 0x08    /**<    2 line display */
#define LCD_1LINE           LCD_CONFIGURATION | 0x00    /**<    1 line display */
#define LCD_5X10            LCD_CONFIGURATION | 0x04    /**<    5 X 10 dots */
#define LCD_5X7             LCD_CONFIGURATION | 0x00    /**<    5 X 7 dots */

#define LCD_SETCGRAMADDR     0x40

#define lcdPutChar(value)   lcdWriteByte(value,1)
#define lcdCommand(value)   lcdWriteByte(value,0)

static const uint8_t        LCDLineOffsets[] PROGMEM = UI_LINE_OFFSETS;
static const char           versionString[] PROGMEM  = UI_VERSION_STRING;


#if UI_DISPLAY_TYPE==1 || UI_DISPLAY_TYPE==2

void lcdWriteNibble(uint8_t value)
{
    WRITE(UI_DISPLAY_D4_PIN,value & 1);
    WRITE(UI_DISPLAY_D5_PIN,value & 2);
    WRITE(UI_DISPLAY_D6_PIN,value & 4);
    WRITE(UI_DISPLAY_D7_PIN,value & 8);
    WRITE(UI_DISPLAY_ENABLE_PIN, HIGH);// enable pulse must be >450ns
    __asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");
    WRITE(UI_DISPLAY_ENABLE_PIN, LOW);
    __asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");

} // lcdWriteNibble


void lcdWriteByte(uint8_t c,uint8_t rs)
{
#if UI_DISPLAY_RW_PIN<0
    HAL::delayMicroseconds(UI_DELAYPERCHAR);
#else
    SET_INPUT(UI_DISPLAY_D4_PIN);
    SET_INPUT(UI_DISPLAY_D5_PIN);
    SET_INPUT(UI_DISPLAY_D6_PIN);
    SET_INPUT(UI_DISPLAY_D7_PIN);
    WRITE(UI_DISPLAY_RW_PIN, HIGH);
    WRITE(UI_DISPLAY_RS_PIN, LOW);
    uint8_t busy;
    do
    {
        WRITE(UI_DISPLAY_ENABLE_PIN, HIGH);
        __asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");
        busy = READ(UI_DISPLAY_D7_PIN);
        WRITE(UI_DISPLAY_ENABLE_PIN, LOW);
        __asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");
        WRITE(UI_DISPLAY_ENABLE_PIN, HIGH);
        __asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");
        WRITE(UI_DISPLAY_ENABLE_PIN, LOW);
        __asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");
    }
    while (busy);
    SET_OUTPUT(UI_DISPLAY_D4_PIN);
    SET_OUTPUT(UI_DISPLAY_D5_PIN);
    SET_OUTPUT(UI_DISPLAY_D6_PIN);
    SET_OUTPUT(UI_DISPLAY_D7_PIN);
    WRITE(UI_DISPLAY_RW_PIN, LOW);
#endif // UI_DISPLAY_RW_PIN<0

    WRITE(UI_DISPLAY_RS_PIN, rs);
    WRITE(UI_DISPLAY_D4_PIN, c & 0x10);
    WRITE(UI_DISPLAY_D5_PIN, c & 0x20);
    WRITE(UI_DISPLAY_D6_PIN, c & 0x40);
    WRITE(UI_DISPLAY_D7_PIN, c & 0x80);
    WRITE(UI_DISPLAY_ENABLE_PIN, HIGH);   // enable pulse must be >450ns
    __asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");
    WRITE(UI_DISPLAY_ENABLE_PIN, LOW);
    __asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");

    WRITE(UI_DISPLAY_D4_PIN, c & 0x01);
    WRITE(UI_DISPLAY_D5_PIN, c & 0x02);
    WRITE(UI_DISPLAY_D6_PIN, c & 0x04);
    WRITE(UI_DISPLAY_D7_PIN, c & 0x08);
    WRITE(UI_DISPLAY_ENABLE_PIN, HIGH);   // enable pulse must be >450ns
    __asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");
    WRITE(UI_DISPLAY_ENABLE_PIN, LOW);
    __asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");

} // lcdWriteByte

void initCspecchars(){
    uid.createChar(1,c1);
    uid.createChar(2,c2);
    uid.createChar(3,c3);
    uid.createChar(4,c4);
    uid.createChar(5,c7);
    uid.createChar(6,c8);
    uid.createChar(7,c9);
    normalchars = false;
}
void initNSpecchars(){
    uid.createChar(1,character_back);
    uid.createChar(2,character_degree);
    uid.createChar(3,character_selected);
    uid.createChar(4,character_unselected);
    uid.createChar(5,character_temperature);
    uid.createChar(6,character_folder);
    uid.createChar(7,character_ready);
    normalchars = true; 
}

void initializeLCD(bool normal)
{
    // bring all display pins into a defined state
    SET_INPUT(UI_DISPLAY_D4_PIN);
    SET_INPUT(UI_DISPLAY_D5_PIN);
    SET_INPUT(UI_DISPLAY_D6_PIN);
    SET_INPUT(UI_DISPLAY_D7_PIN);
    SET_INPUT(UI_DISPLAY_RS_PIN);

#if UI_DISPLAY_RW_PIN>-1
    SET_INPUT(UI_DISPLAY_RW_PIN);
#endif // UI_DISPLAY_RW_PIN>-1

    SET_INPUT(UI_DISPLAY_ENABLE_PIN);

    // SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
    // according to datasheet, we need at least 40ms after power rises above 2.7V
    // before sending commands. Arduino can turn on way before 4.5V.
    // is this delay long enough for all cases??
    HAL::delayMilliseconds(500);
    SET_OUTPUT(UI_DISPLAY_D4_PIN);
    SET_OUTPUT(UI_DISPLAY_D5_PIN);
    SET_OUTPUT(UI_DISPLAY_D6_PIN);
    SET_OUTPUT(UI_DISPLAY_D7_PIN);
    SET_OUTPUT(UI_DISPLAY_RS_PIN);

#if UI_DISPLAY_RW_PIN>-1
    SET_OUTPUT(UI_DISPLAY_RW_PIN);
#endif // UI_DISPLAY_RW_PIN>-1

    SET_OUTPUT(UI_DISPLAY_ENABLE_PIN);

    // Now we pull both RS and R/W low to begin commands
    WRITE(UI_DISPLAY_RS_PIN, LOW);
    WRITE(UI_DISPLAY_ENABLE_PIN, LOW);

    //put the LCD into 4 bit mode
    // this is according to the hitachi HD44780 datasheet
    // figure 24, pg 46

    // we start in 8bit mode, try to set 4 bit mode
    // at this point we are in 8 bit mode but of course in this
    // interface 4 pins are dangling unconnected and the values
    // on them don't matter for these instructions.
    WRITE(UI_DISPLAY_RS_PIN, LOW);
    HAL::delayMicroseconds(10);
    lcdWriteNibble(0x03);
    HAL::delayMicroseconds(5000); // I have one LCD for which 4500 here was not long enough.
    // second try
    lcdWriteNibble(0x03);
    HAL::delayMicroseconds(150); // wait
    // third go!
    lcdWriteNibble(0x03);
    HAL::delayMicroseconds(150);
    // finally, set to 4-bit interface
    lcdWriteNibble(0x02);
    HAL::delayMicroseconds(150);
    // finally, set # lines, font size, etc.
    lcdCommand(LCD_4BIT | LCD_2LINE | LCD_5X7);

    lcdCommand(LCD_CLEAR);                  //- Clear Screen
    HAL::delayMilliseconds(2); // clear is slow operation
    lcdCommand(LCD_INCREASE | LCD_DISPLAYSHIFTOFF); //- Entrymode (Display Shift: off, Increment Address Counter)
    lcdCommand(LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKINGOFF);    //- Display on
    uid.lastSwitch = uid.lastRefresh = HAL::timeInMilliseconds();
    if(normal){
        initNSpecchars();
    }
} // initializeLCD

// ----------- end direct LCD driver
#endif // UI_DISPLAY_TYPE==1 || UI_DISPLAY_TYPE==2


#if UI_DISPLAY_TYPE<4
void UIDisplay::printRow(uint8_t r,char *txt,char *txt2,uint8_t changeAtCol)
{
    changeAtCol = RMath::min((uint8_t)UI_COLS,changeAtCol);
    uint8_t col=0;

    // Set row
    if(r >= UI_ROWS) return;

#if UI_DISPLAY_TYPE==3
    lcdStartWrite();
#endif // UI_DISPLAY_TYPE==3

    lcdWriteByte(128 + HAL::readFlashByte((const char *)&LCDLineOffsets[r]),0); // Position cursor
    char c;
    while((c=*txt) != 0x00 && col<changeAtCol)
    {
        txt++;
        lcdPutChar(c);
#if FEATURE_SEE_DISPLAY
        //cache whatever you write to the display!
        displayCache[r][col] = c;
#endif //FEATURE_SEE_DISPLAY
        col++;
    }
    while(col<changeAtCol)
    {
        lcdPutChar(' ');
#if FEATURE_SEE_DISPLAY
        //cache whatever you write to the display!
        displayCache[r][col] = ' ';
#endif //FEATURE_SEE_DISPLAY
        col++;
    }
    if(txt2!=NULL)
    {
        while((c=*txt2) != 0x00 && col<UI_COLS)
        {
            txt2++;
            lcdPutChar(c);
#if FEATURE_SEE_DISPLAY
            //cache whatever you write to the display!
            displayCache[r][col] = c;
#endif //FEATURE_SEE_DISPLAY
            col++;
        }
        while(col<UI_COLS)
        {
            lcdPutChar(' ');
#if FEATURE_SEE_DISPLAY
            //cache whatever you write to the display!
            displayCache[r][col] = ' ';
#endif //FEATURE_SEE_DISPLAY
            col++;
        }
    }

#if UI_DISPLAY_TYPE==3
    lcdStopWrite();
#endif // UI_DISPLAY_TYPE==3

} // printRow
#endif // UI_DISPLAY_TYPE<4

char printCols[MAX_COLS+1];
UIDisplay::UIDisplay()
{
    locked = 0;
} // UIDisplay


#if UI_ANIMATION
void slideIn(uint8_t row,FSTRINGPARAM(text))
{
    char *empty="";
    int8_t i = 0;
    uid.col=0;
    uid.addStringP(text);
    printCols[uid.col]=0;
    for(i=UI_COLS-1; i>=0; i--)
    {
        uid.printRow(row,empty,printCols,i);
    HAL::delayMilliseconds(10);
    }

} // slideIn
#endif // UI_ANIMATION


void UIDisplay::initialize()
{
    oldMenuLevel = -2;
    flags = 0;
    menuLevel = 0;
    shift = -2;
    menuPos[0] = 0;
    lastAction = 0;
    lastButtonAction = 0;
    activeAction = 0;
    statusMsg[0] = 0;

    messageLine1 = NULL;
    messageLine2 = NULL;
    messageLine3 = NULL;
    messageLine4 = NULL;

    ui_init_keys();

#if SDSUPPORT
    cwd[0]='/';
    cwd[1]=0;
    folderLevel=0;
#endif // SDSUPPORT

#if UI_DISPLAY_TYPE>0
    initializeLCD(false);
    initCspecchars();

#if UI_ANIMATION==false || UI_DISPLAY_TYPE==5
#if UI_DISPLAY_TYPE == 5
    //u8g picture loop
    u8g_FirstPage(&u8g);
    do
    {
#endif // UI_DISPLAY_TYPE == 5

        for(uint8_t y=0; y<UI_ROWS; y++) displayCache[y][0] = 0;
        printRowP(0, PSTR(BIGC0) );
        printRowP(1, PSTR(BIGC1) );
#if UI_ROWS>3
        printRowP(UI_ROWS-2, PSTR(BIGC2) );
#endif // UI_ROWS>3
#if UI_ROWS>2
        printRowP(UI_ROWS-1, PSTR(BIGC3) );
#endif // UI_ROWS>2

#if UI_DISPLAY_TYPE == 5
    }
    while( u8g_NextPage(&u8g) );  //end picture loop
#endif // UI_DISPLAY_TYPE == 5
#else
    slideIn(0, versionString);
    strcpy(displayCache[0], printCols);
    slideIn(1, PSTR(UI_PRINTER_NAME));
    strcpy(displayCache[1], printCols);

#if UI_ROWS>2
    slideIn(UI_ROWS-1, PSTR(UI_PRINTER_COMPANY));
    strcpy(displayCache[UI_ROWS-1], printCols);
#endif // UI_ROWS>2
#endif // UI_ANIMATION==false || UI_DISPLAY_TYPE==5

#endif // UI_DISPLAY_TYPE>0

#if UI_DISPLAY_I2C_CHIPTYPE==0 && (BEEPER_TYPE==2 || defined(UI_HAS_I2C_KEYS))
    // Make sure the beeper is off
    HAL::i2cStartWait(UI_I2C_KEY_ADDRESS+I2C_WRITE);
    HAL::i2cWrite(255); // Disable beeper, enable read for other pins.
    HAL::i2cStop();
#endif // UI_DISPLAY_I2C_CHIPTYPE==0 && (BEEPER_TYPE==2 || defined(UI_HAS_I2C_KEYS))

    if( READ(5) == 0 && READ(11) == 0 && READ(42) == 0 )
    {
        g_nServiceRequest = 1;
    }

} // initialize


#if UI_DISPLAY_TYPE==1 || UI_DISPLAY_TYPE==2 || UI_DISPLAY_TYPE==3
void UIDisplay::createChar(uint8_t location,const uint8_t charmap[])
{
    location &= 0x7; // we only have 8 locations 0-7
    lcdCommand(LCD_SETCGRAMADDR | (location << 3));
    for (int i=0; i<8; i++)
    {
        lcdPutChar(pgm_read_byte(&(charmap[i])));
    }

} // createChar
#endif // UI_DISPLAY_TYPE==1 || UI_DISPLAY_TYPE==2 || UI_DISPLAY_TYPE==3


void  UIDisplay::waitForKey()
{
    int nextAction = 0;


    lastButtonAction = 0;
    while(lastButtonAction==nextAction)
    {
        ui_check_slow_keys(nextAction);
    }

} // waitForKey


void UIDisplay::printRowP(uint8_t r,PGM_P txt)
{
    if(r >= UI_ROWS) return;
    col=0;
    addStringP(txt);
    printCols[col]=0;
    printRow(r,printCols,NULL,UI_COLS);

} // printRowP


void UIDisplay::addInt(int value,uint8_t digits,char fillChar)
{
    if(col>=MAX_COLS) return;
    uint8_t dig=0,neg=0;
    if(value<0)
    {
        value = -value;
        neg=1;
        dig++;
    }
    char buf[7]; // Assumes 8-bit chars plus zero byte.
    char *str = &buf[6];
    buf[6]=0;
    do
    {
        unsigned int m = value;
        value /= 10;
        char c = m - 10 * value;
        *--str = c + '0';
        dig++;
    }
    while(value);
    if(neg)
        printCols[col++]='-';
    if(digits<6)
        while(dig<digits)
        {
            *--str = fillChar; //' ';
            dig++;
        }
    while(*str && col<MAX_COLS)
    {
        printCols[col++] = *str;
        str++;
    }

} // addInt


void UIDisplay::addLong(long value,char digits)
{
    if(col>=MAX_COLS) return;
    uint8_t dig = 0,neg=0;
    if(value<0)
    {
        neg=1;
        value = -value;
        dig++;
    }
    char buf[13]; // Assumes 8-bit chars plus zero byte.
    char *str = &buf[12];
    buf[12]=0;
    do
    {
        unsigned long m = value;
        value /= 10;
        char c = m - 10 * value;
        *--str = c + '0';
        dig++;
    }
    while(value);
    if(neg)
        printCols[col++]='-';
    if(digits<=11)
        while(dig<digits)
        {
            *--str = ' ';
            dig++;
        }
    while(*str && col<MAX_COLS)
    {
        printCols[col++] = *str;
        str++;
    }

} // addLong


const float roundingTable[] PROGMEM = {0.5,0.05,0.005,0.0005,0.00005};
void UIDisplay::addFloat(float number, char fixdigits,uint8_t digits)
{
    if(col>=MAX_COLS) return;
    // Handle negative numbers
    if (number < 0.0)
    {
        printCols[col++]='-';
        if(col>=MAX_COLS) return;
        number = -number;
        fixdigits--;
    }

    digits = (digits <= 4 ? digits : 4);
    number += pgm_read_float(&roundingTable[digits]); // for correct rounding

    // Extract the integer part of the number and print it
    unsigned long int_part = (unsigned long)number;
    float remainder = number - (float)int_part;
    addLong(int_part,fixdigits);
    if(col>=UI_COLS) return;

    // Print the decimal point, but only if there are digits beyond
    if (digits > 0)
    {
        printCols[col++]='.';
    }

    // Extract digits from the remainder one at a time
    while (col<MAX_COLS && digits-- > 0)
    {
        remainder *= 10.0;
        uint8_t toPrint = uint8_t(remainder);
        printCols[col++] = '0'+toPrint;
        remainder -= toPrint;
    }

} // addFloat


void UIDisplay::addStringP(FSTRINGPARAM(text))
{
    while(col<MAX_COLS)
    {
        uint8_t c = HAL::readFlashByte(text++);
        if(c==0) return;
        printCols[col++]=c;
    }

} // addStringP


UI_STRING(ui_text_on,UI_TEXT_ON)
UI_STRING(ui_text_off,UI_TEXT_OFF)
UI_STRING(ui_text_0,UI_TEXT_0)
UI_STRING(ui_text_1,UI_TEXT_1)
UI_STRING(ui_text_white,UI_TEXT_WHITE)
UI_STRING(ui_text_color,UI_TEXT_COLOR)
UI_STRING(ui_text_manual,UI_TEXT_MANUAL)
UI_STRING(ui_text_unknown,UI_TEXT_UNKNOWN)
UI_STRING(ui_text_na,UI_TEXT_NA)
UI_STRING(ui_yes,UI_TEXT_YES)
UI_STRING(ui_no,UI_TEXT_NO)
UI_STRING(ui_ok,UI_TEXT_OK)
UI_STRING(ui_fail,UI_TEXT_FAIL)
UI_STRING(ui_neetfix,UI_TEXT_NEEDFIX)
UI_STRING(ui_up,UI_TEXT_UP)
UI_STRING(ui_down,UI_TEXT_DOWN)
UI_STRING(ui_selected,UI_TEXT_SEL)
UI_STRING(ui_unselected,UI_TEXT_NOSEL)
UI_STRING(ui_text_print_mode,UI_TEXT_PRINT_MODE)
UI_STRING(ui_text_mill_mode,UI_TEXT_MILL_MODE)
UI_STRING(ui_text_z_single,UI_TEXT_Z_SINGLE)
UI_STRING(ui_text_z_circuit,UI_TEXT_Z_CIRCUIT)
UI_STRING(ui_text_z_mode_min,UI_TEXT_Z_MODE_MIN)
UI_STRING(ui_text_z_mode_surface,UI_TEXT_Z_MODE_SURFACE)
UI_STRING(ui_text_z_mode_gcode,UI_TEXT_Z_MODE_GCODE)
UI_STRING(ui_text_z_mode_z_origin,UI_TEXT_Z_MODE_Z_ORIGIN)
UI_STRING(ui_text_hotend_v1,UI_TEXT_HOTEND_V1)
UI_STRING(ui_text_hotend_v2,UI_TEXT_HOTEND_V2)
UI_STRING(ui_text_miller_one_track,UI_TEXT_MILLER_ONE_TRACK)
UI_STRING(ui_text_miller_two_tracks,UI_TEXT_MILLER_TWO_TRACKS)
UI_STRING(ui_text_z_compensation_active,UI_TEXT_Z_COMPENSATION_ACTIVE)

void UIDisplay::parse(char *txt,bool ram)
{
    int     ivalue = 0;
    float   fvalue = 0;


    while(col<MAX_COLS)
    {
       char c=(ram ? *(txt++) : pgm_read_byte(txt++));
        if(c==0) break; // finished
        if(c!='%')
        {
            printCols[col++]=c;
            continue;
        }

        // dynamic parameter, parse meaning and replace
        char c1=(ram ? *(txt++) : pgm_read_byte(txt++));
        char c2=(ram ? *(txt++) : pgm_read_byte(txt++));
        switch(c1)
        {
            case '%':
            {
                if(c2=='%' && col<MAX_COLS)
                    printCols[col++]='%';                                                               // %%% : The % char
                break;
            }
            case '1':
            {
                if(c2 == '1'){                                                                          // %11 : Kill Line if Millingmode and use following string for display, else stop here
#if FEATURE_MILLING_MODE
                    if ( Printer::operatingMode == OPERATING_MODE_MILL )
                    {
                        for(uint8_t n = 0; n < MAX_COLS+1 ; n++) printCols[n]=0; //clear all text
                        col = 0; //reset linemarker
                    }else
#endif // FEATURE_MILLING_MODE
                    { //this is not milling mode: stop here
                        if(col<MAX_COLS) printCols[col++]=0; //write 0 to end of string
                        col = MAX_COLS; //end while
                    }
                }
                break;
            }
            case 'a': // Acceleration settings
            {
                if(c2=='x')      addFloat(Printer::maxAccelerationMMPerSquareSecond[X_AXIS],5,0);       // %ax : X acceleration during print moves
                else if(c2=='y') addFloat(Printer::maxAccelerationMMPerSquareSecond[Y_AXIS],5,0);       // %ay : Y acceleration during print moves
                else if(c2=='z') addFloat(Printer::maxAccelerationMMPerSquareSecond[Z_AXIS],5,0);       // %az : Z acceleration during print moves
                else if(c2=='X') addFloat(Printer::maxTravelAccelerationMMPerSquareSecond[X_AXIS],5,0); // %aX : X acceleration during travel moves
                else if(c2=='Y') addFloat(Printer::maxTravelAccelerationMMPerSquareSecond[Y_AXIS],5,0); // %aY : Y acceleration during travel moves
                else if(c2=='Z') addFloat(Printer::maxTravelAccelerationMMPerSquareSecond[Z_AXIS],5,0); // %aZ : Z acceleration during travel moves
                else if(c2=='j') addFloat(Printer::maxJerk,3,1);                                        // %aj : Max. jerk
                else if(c2=='J') addFloat(Printer::maxZJerk,3,1);                                       // %aJ : Max. Z-jerk
                break;
            }
            case 'd':
            {
                if(c2=='o') addStringP(Printer::debugEcho()?ui_text_on:ui_text_off);                    // %do : Debug echo state
                else if(c2=='i') addStringP(Printer::debugInfo()?ui_text_on:ui_text_off);               // %di : Debug info state
                else if(c2=='e') addStringP(Printer::debugErrors()?ui_text_on:ui_text_off);             // %de : Debug error state
                else if(c2=='d') addStringP(Printer::debugDryrun()?ui_text_on:ui_text_off);             // %dd : Debug dry run state
                else if(c2=='b') addStringP(Printer::enableBeeper?ui_text_on:ui_text_off);              // %db : beeper state
                break;
            }
            case 'D':
            {
                if(c2=='x')         addLong(g_nScanXStepSizeMm,3);                                      // %Dx : scan step size x
                else if(c2=='y')    addLong(g_nScanYStepSizeMm,3);                                      // %Dy : scan step size y
                break;
            }

#if FEATURE_HEAT_BED_Z_COMPENSATION
            case 'H':
            {
                if(c2=='B')         addLong(g_nActiveHeatBed,1);                                        // %HB : active heat bed z matrix
                else if(c2=='O')    addFloat((float)g_offsetZCompensationSteps * Printer::invAxisStepsPerMM[Z_AXIS] * 1000.0f,3,0); // %HO : active heat bed min z offset in um
                break;
            }
#endif // FEATURE_HEAT_BED_Z_COMPENSATION

#if FEATURE_WORK_PART_Z_COMPENSATION
            case 'W':
            {
                if(c2=='P')         addLong(g_nActiveWorkPart,1);                                       // %WP : active work part z matrix
                break;
            }
#endif // FEATURE_WORK_PART_Z_COMPENSATION

            case 'e': // Extruder temperature
            {
                if(c2=='r')                                                                             // %er : Extruder relative mode
                {
                    addStringP(Printer::relativeExtruderCoordinateMode?ui_yes:ui_no);
                    break;
                }

#if FEATURE_MILLING_MODE
                if( Printer::operatingMode == OPERATING_MODE_MILL )
                {
                    // we do not maintain temperatures in milling mode
                    addStringP( PSTR( "   " ));
                    break;
                }
#endif // FEATURE_MILLING_MODE

                if( !g_nPrinterReady )
                {
                    // avoid to show the current temperatures before we have measured them
                    addStringP( PSTR( "   " ));
                    break;
                }

                ivalue = UI_TEMP_PRECISION;
                if(Printer::flag0 & PRINTER_FLAG0_TEMPSENSOR_DEFECT)
                {
                    addStringP(PSTR("def"));
                    break;
                }
                if(c2=='c') fvalue=Extruder::current->tempControl.currentTemperatureC;                  // %ec : Current extruder temperature
                else if(c2>='0' && c2<='9') fvalue=extruder[c2-'0'].tempControl.currentTemperatureC;    // %e0..9 : Temp. of extruder 0..9
                else if(c2=='b') fvalue=Extruder::getHeatedBedTemperature();                            // %eb : Current heated bed temperature
                else if(c2=='B')                                                                        
                {
                    ivalue=0;
                    fvalue=Extruder::getHeatedBedTemperature();
                }
                addFloat(fvalue,3,0 /*ivalue*/);
                break;
            }
            case 'E': // Target extruder temperature
            {
#if FEATURE_MILLING_MODE
                if( Printer::operatingMode == OPERATING_MODE_MILL )
                {
                    // we do not maintain temperatures in milling mode
                    addStringP( PSTR( "   " ));
                    break;
                }
#endif // FEATURE_MILLING_MODE

                if(c2=='c') fvalue=Extruder::current->tempControl.targetTemperatureC;                   // %Ec : Target temperature of current extruder
                else if(c2>='0' && c2<='9') fvalue=extruder[c2-'0'].tempControl.targetTemperatureC;     // %E0-9 : Target temperature of extruder 0..9

#if HAVE_HEATED_BED
                else if(c2=='b') fvalue=heatedBedController.targetTemperatureC;                         // %Eb : Target temperature of heated bed
#endif // HAVE_HEATED_BED

                addFloat(fvalue,3,0 /*UI_TEMP_PRECISION*/);
                break;
            }

            case 'F': // FAN speed
            {
#if FAN_PIN>-1 && FEATURE_FAN_CONTROL
                if(c2=='s') {                                                                           // %Fs : Fan speed in Percent
                    addFloat(Printer::getFanSpeed(false)/2.55f,3,1); 
                }
                else if(c2=='h') {                                                                      // %Fh : Fan frequency in Hz --> Wert passt grob, ist aber nicht exakt! F_CPU/3906/255*mode...
                    addInt((1 << cooler_pwm_speed)*15, 3); 
                }
                else if(c2=='m'){                                                                       // %Fm : Fan modulation type PWM or PDM
                    if(cooler_mode == COOLER_MODE_PDM) addStringP( PSTR("PDM") );
                    else                               addStringP( PSTR("PWM") );
                }
#endif // FAN_PIN>-1 && FEATURE_FAN_CONTROL
#if FEATURE_ZERO_DIGITS
                if(c2=='H')                                                                             // %FH : Digit Homing to Zero ON/OFF
                {
                    addStringP(Printer::g_pressure_offset_active ? ui_text_on : ui_text_off);
                }
#endif // FEATURE_ZERO_DIGITS
#if FEATURE_DIGIT_Z_COMPENSATION
                if(c2=='C')                                                                             // %FC : Digits force-bend-hotend-down Compensation Z ON/OFF
                {
                    addStringP(g_nDigitZCompensationDigits_active ? ui_text_on : ui_text_off);
                }
#endif // FEATURE_DIGIT_Z_COMPENSATION
                break;
            }
            case 'f':
            {
                if(c2=='x') addFloat(Printer::maxFeedrate[X_AXIS],5,0);                                 // %fx : Max. feedrate x direction
                else if(c2=='y') addFloat(Printer::maxFeedrate[Y_AXIS],5,0);                            // %fy : Max. feedrate y direction
                else if(c2=='z') addFloat(Printer::maxFeedrate[Z_AXIS],5,0);                            // %fz : Max. feedrate z direction
                else if(c2=='X') addFloat(Printer::homingFeedrate[X_AXIS],5,0);                         // %fX : Homing feedrate x direction
                else if(c2=='Y') addFloat(Printer::homingFeedrate[Y_AXIS],5,0);                         // %fY : Homing feedrate y direction
                else if(c2=='Z') addFloat(Printer::homingFeedrate[Z_AXIS],5,0);                         // %fZ : Homing feedrate z direction
                break;
            }
            case 'i':
            {
                if(c2=='s') addLong(stepperInactiveTime/1000,4);                                        // %is : Stepper inactive time in seconds
                else if(c2=='p') addLong(maxInactiveTime/1000,4);                                       // %ip : Max. inactive time in seconds
                break;
            }
            case 'O': // ops related stuff
            {
#if NUM_EXTRUDER>1
                if(c2=='a')                                                                             // %Oa : Active Extruder
                {
                    if( Extruder::current->id == 0 )
                    {
                        addStringP(ui_text_0);
                    }
                    else
                    {
                        addStringP(ui_text_1);
                    }
                }
#endif // NUM_EXTRUDER>1

#if NUM_EXTRUDER>1 
                if(c2=='E')                                                                             // %OE : Extruder offset X [mm]
                {
                    addFloat(extruder[1].xOffset/Printer::axisStepsPerMM[X_AXIS],4,3);
                }
                if(c2=='F')                                                                             // %OF : Extruder offset Y [mm]
                {
                    addFloat(extruder[1].yOffset/Printer::axisStepsPerMM[Y_AXIS],4,3);
                }
                if(c2=='S')                                                                             // %OS : Extruder spring displacement Z [mm]
                {
                    addFloat(extruder[1].zOffset/Printer::axisStepsPerMM[Z_AXIS],4,3);
                }
#endif // NUM_EXTRUDER>1

                if(c2=='M')                                                                             // %OM : operating mode
                {
#if FEATURE_MILLING_MODE
                    addStringP(Printer::operatingMode==OPERATING_MODE_PRINT?ui_text_print_mode:ui_text_mill_mode);
#else
                    addStringP(ui_text_print_mode);
#endif // FEATURE_MILLING_MODE
                }
                if(c2=='Z')                                                                             // %OZ : z endstop type                                                  
                {
#if FEATURE_CONFIGURABLE_Z_ENDSTOPS
                    addStringP(Printer::ZEndstopType==ENDSTOP_TYPE_SINGLE?ui_text_z_single:ui_text_z_circuit);
#else
                    addStringP(ui_text_z_single);
#endif // FEATURE_CONFIGURABLE_Z_ENDSTOPS
                }
                break;
            }
            case 'h':
            {
                if(c2=='x' && col<MAX_COLS)                                                                             // %hx : x homed      
                {
                    if(Printer::flag3 & PRINTER_FLAG3_X_HOMED) printCols[col++]='*';
                }
                else if(c2=='y' && col<MAX_COLS)                                                                             // %hy : y homed      
                {
                    if(Printer::flag3 & PRINTER_FLAG3_Y_HOMED) printCols[col++]='*';
                }
                else if(c2=='z' && col<MAX_COLS)                                                                             // %hz : z homed      
                {
                    if(Printer::flag3 & PRINTER_FLAG3_Z_HOMED) printCols[col++]='*';
                }
                else if(c2=='a' && col<MAX_COLS)                                                                             // %ha : all homed      
                {
                    if(Printer::flag3 & PRINTER_FLAG3_X_HOMED && Printer::flag3 & PRINTER_FLAG3_Y_HOMED && Printer::flag3 & PRINTER_FLAG3_Z_HOMED) printCols[col++]='*';
                }
                break;
            }
            case 'l':
            {
                if(c2=='a') addInt(lastAction,4);                                                       

#if FEATURE_CASE_LIGHT
                else if(c2=='o') addStringP(Printer::enableCaseLight?ui_text_on:ui_text_off);           // %lo : Lights on/off
#endif // FEATURE_CASE_LIGHT

#if FEATURE_RGB_LIGHT_EFFECTS
                else if(c2=='i')                                                                        // %li : Light: White/Color
                {
                    if( Printer::RGBLightModeForceWhite )
                    {
                        addStringP(ui_text_white);
                    }
                    else
                    {
                        switch(Printer::RGBLightMode)
                        {
                            case RGB_MODE_OFF:          addStringP(ui_text_off);    break;
                            case RGB_MODE_WHITE:        addStringP(ui_text_white);  break;
                            case RGB_MODE_AUTOMATIC:    addStringP(ui_text_color);  break;
                            case RGB_MODE_MANUAL:       addStringP(ui_text_manual); break;
                        }       
                    }
                }
#endif // FEATURE_RGB_LIGHT_EFFECTS

                break;
            }

            case 'm':
            {
                if(c2=='Y' && col<MAX_COLS)                                                                             // %mY : menu yes
                {
                    if(g_nYesNo)    printCols[col++]=CHAR_SELECTED;
                    else            printCols[col++]=' ';
                }
                else if(c2=='N' && col<MAX_COLS)                                                                        // %mN : menu no
                {
                    if(g_nYesNo)    printCols[col++]=' ';
                    else            printCols[col++]=CHAR_SELECTED;
                }
                else if(c2=='t')                                                                        // %mt : miller type                                                     
                {
#if FEATURE_CONFIGURABLE_MILLER_TYPE
                    addStringP(Printer::MillerType==MILLER_TYPE_ONE_TRACK?ui_text_miller_one_track:ui_text_miller_two_tracks);
#endif // FEATURE_CONFIGURABLE_MILLER_TYPE
                }
                else if(c2=='1')                                                                        // %m1 : message line 1
                {
                    if(messageLine1!=0){
                        col = 0; //Nibbels: kill first space because this is menutab
                        parse((char PROGMEM *)messageLine1,false); //addStringP((char PROGMEM *)messageLine1);
                    }   
                    break;
                }
                else if(c2=='2')                                                                        // %m2 : message line 2
                {
                    if(messageLine2!=0){
                        col = 0; //Nibbels: kill first space because this is menutab
                        parse((char PROGMEM *)messageLine2,false); //addStringP((char PROGMEM *)messageLine2);
                    }
                    break;
                }
                else if(c2=='3')                                                                        // %m3 : message line 3
                {
                    if(messageLine3!=0){
                        col = 0; //Nibbels: kill first space because this is menutab
                        parse((char PROGMEM *)messageLine3,false); //addStringP((char PROGMEM *)messageLine3);
                    }   
                    break;
                }
                else if(c2=='4')                                                                        // %m4 : message line 4
                {
                    if(messageLine4!=0){
                        col = 0; //Nibbels: kill first space because this is menutab
                        parse((char PROGMEM *)messageLine4,false); //addStringP((char PROGMEM *)messageLine4);
                    } 
                    break;
                }
                break;
            }

            case 'M':
            {
                if(c2=='X' && col<MAX_COLS)                                                            // %MX : Motorcurrent X
                {
                    addFloat(Printer::motorCurrent[X_AXIS]/63.0f,1,2);  //(126 = ~2A) *2.0f/126.0f
                    if(col<MAX_COLS) printCols[col++]='A';
                    if(col<MAX_COLS) printCols[col++]=' ';
                    addInt((int)Printer::motorCurrent[X_AXIS],3);
                }
                else if(c2=='Y' && col<MAX_COLS)                                                       // %MY : Motorcurrent Y
                {
                    addFloat(Printer::motorCurrent[Y_AXIS]/63.0f,1,2);  //(126 = ~2A) *2.0f/126.0f
                    if(col<MAX_COLS) printCols[col++]='A';
                    if(col<MAX_COLS) printCols[col++]=' ';
                    addInt((int)Printer::motorCurrent[Y_AXIS],3);
                }
                else if(c2=='Z')                                                                       // %MZ : Motorcurrent Z
                {
                    addFloat(Printer::motorCurrent[Z_AXIS]/63.0f,1,2);  //(126 = ~2A) *2.0f/126.0f
                    if(col<MAX_COLS) printCols[col++]='A';
                    if(col<MAX_COLS) printCols[col++]=' ';
                    addInt((int)Printer::motorCurrent[Z_AXIS],3);
                }
                else if(c2=='0')                                                                       // %M0 : Motorcurrent T0
                {
                    addFloat(Printer::motorCurrent[E_AXIS]/63.0f,1,2);  //(126 = ~2A) *2.0f/126.0f
                    if(col<MAX_COLS) printCols[col++]='A';
                    if(col<MAX_COLS) printCols[col++]=' ';
                    if(g_pauseStatus == PAUSE_STATUS_PAUSED) addStringP(PSTR(UI_TEXT_PAUSED));
                    else addInt((int)Printer::motorCurrent[E_AXIS],3);
                }
                else if(c2=='1')                                                                       // %M1 : Motorcurrent T1
                {
                    addFloat(Printer::motorCurrent[E_AXIS+1]/63.0f,1,2);  //(126 = ~2A) *2.0f/126.0f
                    if(col<MAX_COLS) printCols[col++]='A';
                    if(col<MAX_COLS) printCols[col++]=' ';
                    if(g_pauseStatus == PAUSE_STATUS_PAUSED) addStringP(PSTR(UI_TEXT_PAUSED));
                    else addInt((int)Printer::motorCurrent[E_AXIS+1],3);
                }
                break;
            }

            case 'o':
            {
                if(c2=='s')                                                                             // %os : Status message
                {
                    if(locked)
                    {
                        //Com::printFLN( PSTR( "locked" ) );
                        // do not change the status message in case it is locked
                        parse(statusMsg,true);
                    }
                    else
                    {
#if SDSUPPORT
                        if(sd.sdactive && sd.sdmode)
                        {
                            if( g_pauseMode >= PAUSE_MODE_PAUSED )
                            {
                                // do not show the printing/milling progress while we are paused
                                parse(statusMsg,true);
                            }
                            else
                            {
#if FEATURE_MILLING_MODE
                                if( Printer::operatingMode == OPERATING_MODE_PRINT )
                                {
                                    addStringP(PSTR(UI_TEXT_PRINT_POS));
                                    unsigned long percent;
                                    if(sd.filesize<20000000) percent=sd.sdpos*100/sd.filesize;
                                    else percent = (sd.sdpos>>8)*100/(sd.filesize>>8);
                                    addInt((int)percent,3);
                                    if(col<MAX_COLS)
                                        printCols[col++]='%';
                                }
                                else
                                {
                                    if ( !g_nZOriginSet )
                                    {
                                        parse(statusMsg,true);
                                    }
                                    else
                                    {
                                        addStringP(PSTR(UI_TEXT_MILL_POS));

                                        unsigned long percent;
                                        if(sd.filesize<20000000) percent=sd.sdpos*100/sd.filesize;
                                        else percent = (sd.sdpos>>8)*100/(sd.filesize>>8);
                                        addInt((int)percent,3);
                                        if(col<MAX_COLS)
                                            printCols[col++]='%';
                                    }
                                }
#else
                                addStringP(PSTR(UI_TEXT_PRINT_POS));

                                unsigned long percent;
                                if(sd.filesize<20000000) percent=sd.sdpos*100/sd.filesize;
                                else percent = (sd.sdpos>>8)*100/(sd.filesize>>8);
                                addInt((int)percent,3);
                                if(col<MAX_COLS)
                                    printCols[col++]='%';
#endif // FEATURE_MILLING_MODE
                            }
                        }
                        else
#endif // SDSUPPORT

                            parse(statusMsg,true);
                    }
                    break;
                }
                if(c2=='c')                                                                             // %oc : Connection baudrate
                {
                    addLong(baudrate,6);
                    break;
                }
                if(c2=='B')                                                                             // %oB : Buffer length
                {
                    addInt((int)PrintLine::linesCount,2);
                    break;
                }
                if(c2=='f')                                                                             // %of : flow multiplier
                {
                    addInt(Printer::extrudeMultiply
 #if FEATURE_DIGIT_FLOW_COMPENSATION
                            * g_nDigitFlowCompensation_flowmulti
 #endif // FEATURE_DIGIT_FLOW_COMPENSATION
                    ,3);
                    break;
                }
                if(c2=='m')                                                                             // %om : Speed multiplier
                {
                    addInt(Printer::feedrateMultiply
 #if FEATURE_DIGIT_FLOW_COMPENSATION
                            * g_nDigitFlowCompensation_feedmulti
 #endif // FEATURE_DIGIT_FLOW_COMPENSATION
                    ,3);
                    break;
                }
                if(c2=='v')                                                                             // %ov : Active Speed
                {
                    addFloat(Printer::v
 #if FEATURE_DIGIT_FLOW_COMPENSATION
                            * g_nDigitFlowCompensation_feedmulti
 #endif // FEATURE_DIGIT_FLOW_COMPENSATION
                    ,3,2);
                    break;
                }
                if(c2=='p')                                                                             // %op : Is single double or quadstepping?
                {
                    switch(Printer::stepsPerTimerCall){
                        case 1: addStringP( PSTR(" Sgl") ); break; //Single Stepping aktiv
                        case 2: addStringP( PSTR(" Dbl") ); break; //Double Stepping aktiv
                        case 4: addStringP( PSTR(" Qud") ); break; //Quad Stepping aktiv
                    }
                    break;
                }
 
#if FEATURE_230V_OUTPUT
                if(c2=='u')
                {
                    addStringP(Printer::enable230VOutput?ui_text_on:ui_text_off);                       // %ou : 230V output on/off
                    break;
                }
#endif // FEATURE_230V_OUTPUT

#if FEATURE_24V_FET_OUTPUTS
                if(c2=='l')
                {
                    addStringP(Printer::enableFET1?ui_text_on:ui_text_off);                     // %ol : FET 1 X42 output on/off
                    break;
                }
#endif // FEATURE_24V_FET_OUTPUTS

#if FEATURE_24V_FET_OUTPUTS
                if(c2=='n')
                {
                    addStringP(Printer::enableFET2?ui_text_on:ui_text_off);                     // %on : FET 2 X44 output on/off
                    break;
                }
#endif // FEATURE_24V_FET_OUTPUTS

                // Extruder output level
                if(c2>='0' && c2<='9')                                                                  // %o0..9 : Output level extruder 0..9 is % including %sign
                {
                    ivalue=pwm_pos[c2-'0'];
                }

#if HAVE_HEATED_BED
                else if(c2=='b')                                                                        // %ob : Output level heated bed
                {
                    ivalue=pwm_pos[heatedBedController.pwmIndex];
                }
#endif // HAVE_HEATED_BED

                else if(c2=='C')                                                                        // %oC : Output level current extruder
                {
                    ivalue=pwm_pos[Extruder::current->id];
                }

                ivalue=(ivalue*100)/255;
                addInt(ivalue,3);
                if(col<MAX_COLS)
                    printCols[col++]='%';
                break;
            }
            case 'x':
            {
                char    bDefect = false;


                if(c2>='0' && c2<='3')                                                                  
                {
                    if(c2=='0')                                                                         // %x0 : X position
                    {
                        if( Printer::blockAll )
                        {
                            // we can not move any more
                            bDefect = true;
                        }
                        else
                        {
                            fvalue = Printer::currentXPosition();
                        }
                    }
                    else if(c2=='1')                                                                    // %x1 : Y position
                    {
                        if( Printer::blockAll )
                        {
                            // we can not move any more
                            bDefect = true;
                        }
                        else
                        {
                            fvalue = Printer::currentYPosition();
                        }
                    }
                    else if(c2=='2')                                                                    // %x2 : Z position
                    {
                        if( Printer::blockAll )
                        {
                            // we can not move any more
                            bDefect = true;
                        }
                        else
                        {
                            fvalue = Printer::currentZPosition();
                        }
                    }
                    else                                                                                // %x3 : Current extruder position
                    {
                        if( Printer::blockAll )
                        {
                            // we can not move any more
                            bDefect = true;
                        }
                        else
                        {
                            fvalue = (float)Printer::queuePositionLastSteps[E_AXIS]*Printer::invAxisStepsPerMM[E_AXIS];
                        }
                    }

                    if( bDefect )
                    {
                        addStringP( PSTR(" def") );
                    }
                    else
                    {
                        addFloat(fvalue,4,2);
                    }
                }
                break;
            }
            case 'y':
            {
                break;
            }
            case 'z':                                                                                   // %z0 : Z Offset
            {
                if(c2=='0')
                {
                    addInt(Printer::ZOffset,4);
                }
                else if(c2=='m')                                                                        // %zm : Z Scale
                {
#if FEATURE_MILLING_MODE
                    if( Printer::operatingMode == OPERATING_MODE_MILL )
                    {
                        //addStringP(Printer::ZMode==Z_VALUE_MODE_Z_ORIGIN?ui_text_z_mode_z_origin:ui_text_z_mode_surface);
                        addStringP(Printer::ZMode==Z_VALUE_MODE_Z_ORIGIN ? ui_text_z_mode_z_origin : (Printer::ZMode==Z_VALUE_MODE_SURFACE ? ui_text_z_mode_surface : ui_text_z_mode_gcode) );
                    }
                    else
#endif // FEATURE_MILLING_MODE
                    {
                        addStringP(Printer::ZMode==Z_VALUE_MODE_Z_MIN ? ui_text_z_mode_min : (Printer::ZMode==Z_VALUE_MODE_SURFACE ? ui_text_z_mode_surface : ui_text_z_mode_gcode) );
                    }
                }
                else if(c2=='s')                                                                        // %zs : Z-Schraube korrektur mm
                {
                    addFloat(g_ZSchraubenSollKorrekturWarm_mm,1,3);                    
                }
                else if(c2=='S')                                                                        // %zS : Z-Schraube korrektur Umdrehungen
                {
                    addFloat(g_ZSchraubenSollDrehungenWarm_U,1,1);
                }
                else if(c2=='F')                                                                        // %zF : Z-Schraube falsch
                {
                    if(g_ZSchraubeOk < 0) addStringP( ui_ok ); //ok
                    else if(g_ZSchraubeOk > 0) addStringP( ui_neetfix ); //zu hohes bett
                    else addStringP( ui_fail ); //fail
                }
                else if(c2=='D')                                                                        // %zD : Z-Schraube richtung
                {
                    if(g_ZSchraubenSollKorrekturWarm_mm > 0) addStringP( ui_up );
                    else addStringP( ui_down );
                }
                break;
            }
            case 'X': // Extruder related
            {
#if NUM_EXTRUDER>0
                if(c2>='0' && c2<='9')                                                                  // %X0..9 : Extruder selected marker
                {
                    addStringP(Extruder::current->id==c2-'0'?ui_selected:ui_unselected);
                }

                else if(c2=='i')                                                                        // %Xi : PID I gain
                {
                    addFloat(Extruder::current->tempControl.pidIGain,4,2);
                }
                else if(c2=='p')                                                                        // %Xp : PID P gain
                {
                    addFloat(Extruder::current->tempControl.pidPGain,4,2);
                }
                else if(c2=='d')                                                                        // %Xd : PID D gain
                {
                    addFloat(Extruder::current->tempControl.pidDGain,4,2);
                }
                else if(c2=='m')                                                                        // %Xm : PID drive min
                {
                    if(uid.menuLevel == 4 && uid.menuPos[uid.menuLevel-1] < NUM_TEMPERATURE_LOOPS){
                        addInt(-1*tempController[uid.menuPos[uid.menuLevel-1]]->pidDriveMin,3); 
                    }else{
                        addInt(Extruder::current->tempControl.pidDriveMin,3);
                    }
                }
                else if(c2=='M')                                                                        // %XM : PID drive max
                {
                    if(uid.menuLevel == 4 && uid.menuPos[uid.menuLevel-1] < NUM_TEMPERATURE_LOOPS){
                        addInt(tempController[uid.menuPos[uid.menuLevel-1]]->pidDriveMax,3); 
                    }else{
                        addInt(Extruder::current->tempControl.pidDriveMax,3);
                    }
                }
                else if(c2=='D')                                                                        // %XD : PID max
                {
                    if(uid.menuLevel == 4 && uid.menuPos[uid.menuLevel-1] < NUM_TEMPERATURE_LOOPS){
                        addInt(tempController[uid.menuPos[uid.menuLevel-1]]->pidMax*100/255,3); 
                    }else{
                        addInt(Extruder::current->tempControl.pidMax*100/255,3);
                    }
                }
                else if(c2=='S')                                                                        // %XS : Temperature Sensor
                {
                    if(uid.menuLevel == 4 && uid.menuPos[uid.menuLevel-1] < NUM_TEMPERATURE_LOOPS){
                        addInt(tempController[uid.menuPos[uid.menuLevel-1]]->sensorType,2); //mit type 100 wärens 3 zeichen, aber das kommt in praxis nicht vor. 
                        switch(tempController[uid.menuPos[uid.menuLevel-1]]->sensorType){
                            case 1: {
                                addStringP( PSTR(UI_TEXT_SENSOR_1) );
                                break;
                            }
                            case 3: {
                                addStringP( PSTR(UI_TEXT_SENSOR_3) );
                                break;
                            }
                            case 4: {
                                addStringP( PSTR(UI_TEXT_SENSOR_4) );
                                break;
                            }
                            case 8: {
                                addStringP( PSTR(UI_TEXT_SENSOR_8) );
                                break;
                            }
                            case 13: {
                                addStringP( PSTR(UI_TEXT_SENSOR_13) );
                                break;
                            }
                        }
                    }else{
                        addInt(Extruder::current->tempControl.sensorType,3);
                    }
                }

                else if(c2=='w')                                                                        // %Xw : Extruder watch period in seconds
                {
                    addInt(Extruder::current->watchPeriod,4);
                }

#if RETRACT_DURING_HEATUP
                else if(c2=='T')                                                                        // %XT : Extruder wait retract temperature
                {
                    addInt(Extruder::current->waitRetractTemperature,4);
                }
                else if(c2=='U')                                                                        // %XU : Extruder wait retract unit
                {
                    addInt(Extruder::current->waitRetractUnits,2);
                }
#endif // RETRACT_DURING_HEATUP

#if USE_ADVANCE
#ifdef ENABLE_QUADRATIC_ADVANCE
                else if(c2=='a')
                {
                    addFloat(Extruder::current->advanceK,3,0);
                }
#endif // ENABLE_QUADRATIC_ADVANCE

                else if(c2=='l')                                                                        // %Xl : Advance L value
                {
                    addFloat(Extruder::current->advanceL,3,0);
                }
                else if(c2=='b')                                                                        // %Xb : E0 Advance L value
                {
                    addFloat(extruder[0].advanceL,3,0);
                }
                else if(c2=='c')                                                                        // %Xc : E1 Advance L value
                {
                    addFloat(extruder[1].advanceL,3,0);
                }
#endif // USE_ADVANCE

                else if(c2=='f')                                                                        // %Xf : Extruder max. start feedrate
                {
                    addFloat(Extruder::current->maxStartFeedrate,5,0);
                }
                else if(c2=='F')                                                                        // %XF : Extruder max. feedrate
                {
                    addFloat(Extruder::current->maxFeedrate,5,0);
                }
                else if(c2=='A')                                                                        // %XA : Extruder max. acceleration
                {
                    addFloat(Extruder::current->maxAcceleration,5,0);
                }
#endif // NUM_EXTRUDER>0
                else if(c2 == 'g')                                                                      // %Xg : Printer::stepsDoublerFrequency 
                {
                    addInt(Printer::stepsDoublerFrequency,5);
                    addStringP( PSTR(" ") );
                    addFloat(Printer::stepsDoublerFrequency/RMath::max(XAXIS_STEPS_PER_MM,YAXIS_STEPS_PER_MM),3,0);
                    addStringP( PSTR("mm/s") );
                }
#if FEATURE_MILLING_MODE
                else if(c2=='Z')                                                                        // %XZ : Milling special max. acceleration
                {
                    addInt(Printer::max_milling_all_axis_acceleration,3);
                }
#endif // FEATURE_MILLING_MODE
                break;
            }
            case 's': // Endstop positions
            {
                if(c2=='x')                                                                             // %sx : State of x min endstop
                {
#if (X_MIN_PIN > -1) && MIN_HARDWARE_ENDSTOP_X
                    addStringP(Printer::isXMinEndstopHit()?ui_text_on:ui_text_off);
#else
                    addStringP(ui_text_na);
#endif // (X_MIN_PIN > -1) && MIN_HARDWARE_ENDSTOP_X
                }
                if(c2=='X')                                                                             // %sX : State of x max endstop
#if (X_MAX_PIN > -1) && MAX_HARDWARE_ENDSTOP_X
                    addStringP(Printer::isXMaxEndstopHit()?ui_text_on:ui_text_off);
#else
                    addStringP(ui_text_na);
#endif // (X_MAX_PIN > -1) && MAX_HARDWARE_ENDSTOP_X

                if(c2=='y')                                                                             // %sy : State of y min endstop
#if (Y_MIN_PIN > -1)&& MIN_HARDWARE_ENDSTOP_Y
                    addStringP(Printer::isYMinEndstopHit()?ui_text_on:ui_text_off);
#else
                    addStringP(ui_text_na);
#endif // (Y_MIN_PIN > -1)&& MIN_HARDWARE_ENDSTOP_Y

                if(c2=='Y')                                                                             // %sY : State of y max endstop
#if (Y_MAX_PIN > -1) && MAX_HARDWARE_ENDSTOP_Y
                    addStringP(Printer::isYMaxEndstopHit()?ui_text_on:ui_text_off);
#else
                    addStringP(ui_text_na);
#endif // (Y_MAX_PIN > -1) && MAX_HARDWARE_ENDSTOP_Y

                if(c2=='z')                                                                             // %sz : State of z min endstop
                {
#if (Z_MIN_PIN > -1) && MIN_HARDWARE_ENDSTOP_Z

#if FEATURE_CONFIGURABLE_Z_ENDSTOPS
                    if( Printer::ZEndstopUnknown )
                    {
                        addStringP(ui_text_unknown);
                    }
                    else
                    {
                        addStringP(Printer::isZMinEndstopHit()?ui_text_on:ui_text_off);
                    }
#else
                    addStringP(Printer::isZMinEndstopHit()?ui_text_on:ui_text_off);
#endif // FEATURE_CONFIGURABLE_Z_ENDSTOPS

#else
                    addStringP(ui_text_na);
#endif // (Z_MIN_PIN > -1) && MIN_HARDWARE_ENDSTOP_Z
                }
                if(c2=='Z')                                                                             // %sZ : State of z max endstop
                {
#if (Z_MAX_PIN > -1) && MAX_HARDWARE_ENDSTOP_Z

#if FEATURE_CONFIGURABLE_Z_ENDSTOPS
                    if( Printer::ZEndstopUnknown )
                    {
                        addStringP(ui_text_unknown);
                    }
                    else
                    {
                        addStringP(Printer::isZMaxEndstopHit()?ui_text_on:ui_text_off);
                    }
#else
                    addStringP(Printer::isZMaxEndstopHit()?ui_text_on:ui_text_off);
#endif // FEATURE_CONFIGURABLE_Z_ENDSTOPS

#else
                    addStringP(ui_text_na);
#endif // (Z_MAX_PIN > -1) && MAX_HARDWARE_ENDSTOP_Z
                }
                if(c2=='C')                                                                             // %sC : State of the z compensation
                {
#if FEATURE_HEAT_BED_Z_COMPENSATION
                    if( Printer::doHeatBedZCompensation )
                    {
                        addStringP(ui_text_z_compensation_active);
                    }
#endif // FEATURE_HEAT_BED_Z_COMPENSATION

#if FEATURE_WORK_PART_Z_COMPENSATION
                    if( Printer::doWorkPartZCompensation )
                    {
                        addStringP(ui_text_z_compensation_active);
                    }
#endif // FEATURE_WORK_PART_Z_COMPENSATION
                }
                
                if(c2=='S')                                                                             // %sS : State of the sensible offset
                {
#if FEATURE_SENSIBLE_PRESSURE
                    if( Printer::doHeatBedZCompensation )
                    {
                        addInt((int)g_nSensiblePressureOffset,3);
                    }else{
                        addInt(0,3);    
                    }
#endif // FEATURE_SENSIBLE_PRESSURE
                }
              
                if(c2=='M')                                                                             // %sM : State of the sensible offset
                {
#if FEATURE_SENSIBLE_PRESSURE
                    if( Printer::doHeatBedZCompensation && g_nSensiblePressureDigits > 0 )
                    {
                        if(g_nSensiblePressure1stMarke) addStringP( PSTR( "^" ));
                        else addStringP( PSTR( "@" ));
                        addInt((int)g_nSensiblePressureDigits,5);
                    }else{
                        addStringP(ui_text_off);
                    }
#endif // FEATURE_SENSIBLE_PRESSURE
                }
                
                if(c2=='1')                                                                             // %s1 : current value of the strain gauge
                {
                    addInt(g_nLastDigits,5);
                }

                break;
            }
            case 'S':
            {
                if(c2=='0')      addFloat(extruder[0].stepsPerMM,4,0);                                                // %S0 : Steps per mm extruder0
                else if(c2=='1') addFloat(extruder[1].stepsPerMM,4,0);                                                // %S1 : Steps per mm extruder1
                else if(c2=='e') addFloat(Extruder::current->stepsPerMM,3,1);                                         // %Se : Steps per mm current extruder
                else if(c2=='z') addFloat(g_nManualSteps[Z_AXIS] * Printer::invAxisStepsPerMM[Z_AXIS] * 1000,4,0);    // %Sz : Mikrometer per Z-Single_Step (Z_Axis)
                else if(c2=='M' && col<MAX_COLS) if(g_ZMatrixChangedInRam) printCols[col++]='*';                      // %SM : Matrix has changed in Ram and is ready to Save. -> *)
                break;
            }
            case 'p':
            {
                if(c2=='x')                                                                             // %px: mode of the Position X menu
                {
                    switch( Printer::moveMode[X_AXIS] )
                    {
                        case MOVE_MODE_SINGLE_STEPS:
                        {
                            addStringP(PSTR(UI_TEXT_MOVE_MODE_SINGLE_STEPS));
                            break;
                        }
                        case MOVE_MODE_SINGLE_MOVE:
                        {
                            addStringP(PSTR(UI_TEXT_MOVE_MODE_SINGLE_MOVE));
                            break;
                        }
                        case MOVE_MODE_1_MM:
                        {
                            addStringP(PSTR(UI_TEXT_MOVE_MODE_1_MM));
                            break;
                        }
                        case MOVE_MODE_10_MM:
                        {
                            addStringP(PSTR(UI_TEXT_MOVE_MODE_10_MM));
                            break;
                        }
                        case MOVE_MODE_50_MM:
                        {
                            addStringP(PSTR(UI_TEXT_MOVE_MODE_50_MM));
                            break;
                        }
                    }
                }
                if(c2=='y')                                                                             // %py: mode of the Position Y menu
                {
                    switch( Printer::moveMode[Y_AXIS] )
                    {
                        case MOVE_MODE_SINGLE_STEPS:
                        {
                            addStringP(PSTR(UI_TEXT_MOVE_MODE_SINGLE_STEPS));
                            break;
                        }
                        case MOVE_MODE_SINGLE_MOVE:
                        {
                            addStringP(PSTR(UI_TEXT_MOVE_MODE_SINGLE_MOVE));
                            break;
                        }
                        case MOVE_MODE_1_MM:
                        {
                            addStringP(PSTR(UI_TEXT_MOVE_MODE_1_MM));
                            break;
                        }
                        case MOVE_MODE_10_MM:
                        {
                            addStringP(PSTR(UI_TEXT_MOVE_MODE_10_MM));
                            break;
                        }
                        case MOVE_MODE_50_MM:
                        {
                            addStringP(PSTR(UI_TEXT_MOVE_MODE_50_MM));
                            break;
                        }
                    }
                }
                if(c2=='z')                                                                             // %pz: mode of the Position Z menu
                {
                    switch( Printer::moveMode[Z_AXIS] )
                    {
                        case MOVE_MODE_SINGLE_STEPS:
                        {
                            addStringP(PSTR(UI_TEXT_MOVE_MODE_SINGLE_STEPS));
                            break;
                        }
                        case MOVE_MODE_SINGLE_MOVE:
                        {
                            addStringP(PSTR(UI_TEXT_MOVE_MODE_SINGLE_MOVE));
                            break;
                        }
                        case MOVE_MODE_1_MM:
                        {
                            addStringP(PSTR(UI_TEXT_MOVE_MODE_1_MM));
                            break;
                        }
                        case MOVE_MODE_10_MM:
                        {
                            addStringP(PSTR(UI_TEXT_MOVE_MODE_10_MM));
                            break;
                        }
                        case MOVE_MODE_50_MM:
                        {
                            addStringP(PSTR(UI_TEXT_MOVE_MODE_50_MM));
                            break;
                        }
                    }
                }
#if FEATURE_EMERGENCY_PAUSE
                if(c2=='l')                                                                             // %pl : g_nEmergencyPauseDigitsMin [1700/kg]
                {
                    addLong(g_nEmergencyPauseDigitsMin,6);
                    break;
                }
                if(c2=='h')                                                                             // %ph : g_nEmergencyPauseDigitsMax [1700/kg]
                {
                    addLong(g_nEmergencyPauseDigitsMax,6);
                    break;
                }
#endif //FEATURE_EMERGENCY_PAUSE
#if FEATURE_EMERGENCY_STOP_ALL
                if(c2=='L')                                                                             // %pL : g_nZEmergencyStopAllMin [1700/kg]
                {
                    addLong(g_nZEmergencyStopAllMin,6);
                    break;
                }
                if(c2=='H')                                                                             // %pH : g_nZEmergencyStopAllMax [1700/kg]
                {
                    addLong(g_nZEmergencyStopAllMax,6);
                    break;
                }
#endif // FEATURE_EMERGENCY_STOP_ALL
                break;
            }
            case 'P':
            {
                if(c2=='N') addStringP(PSTR(UI_PRINTER_NAME));                                          // %PN : Printer name
                break;
            }
            case 'U':                                                                                   // %U1: Page1  
            {
                if(c2=='1')                                                                             // temperature icon
                {
                    if(!normalchars) initNSpecchars();
                    addStringP(PSTR("\005"));
                }
                else if(c2=='2')                                                                             // replace line with new line
                {
                    char    mode = OPERATING_MODE_PRINT;
#if FEATURE_MILLING_MODE
                    mode = Printer::operatingMode;
#endif // FEATURE_MILLING_MODE
                    if ( mode == OPERATING_MODE_MILL )
                    {
                        for(uint8_t n = 0; n < MAX_COLS+1 ; n++) printCols[n]=0; //clear all text
                        col = 0; //reset linemarker
#if FEATURE_230V_OUTPUT
 #if FEATURE_CASE_LIGHT
                        addStringP( PSTR( "MIL X19:" )); // -> Die restliche Zeile wird komplett überschrieben und der rest verworfen. Weil Millingmode keine Temperaturen hat.
                        addStringP(Printer::enableCaseLight?ui_text_on:ui_text_off);
                        addStringP( PSTR( " 230V:" )); 
                        addStringP(Printer::enable230VOutput?ui_text_on:ui_text_off);
 #else
                        addStringP( PSTR( "MILLER  230V:" )); 
                        addStringP(Printer::enable230VOutput?ui_text_on:ui_text_off);
 #endif
#else //FEATURE_230V_OUTPUT
 #if FEATURE_CASE_LIGHT
                        addStringP( PSTR( "MILLER X19:" )); // -> Die restliche Zeile wird komplett überschrieben und der rest verworfen. Weil Millingmode keine Temperaturen hat.
                        addStringP(Printer::enableCaseLight?ui_text_on:ui_text_off);
 #else
                        addStringP( PSTR( "MILLER" ));
 #endif
#endif //FEATURE_230V_OUTPUT
                    }
                }
                break;
            }
            case 'C':{
#if FEATURE_READ_CALIPER
                if(c2=='a')                                                                             // %Ca : Caliper Active Reading
                {
                    addLong(abs(caliper_um)+caliper_collect_adjust,5);
                }
                else if(c2=='m')                                                                        // %Cm : Caliper Average in mm
                {
                    addFloat(0.001f*caliper_collect_um/caliper_collect_count,2,3);
                }
                else if(c2=='s')                                                                        // %Cs : Caliper Filament Standard
                {
                    addFloat(0.001f * caliper_filament_standard,2,3);
                }
                else if(c2=='c')                                                                        // %Cc : Caliper Correction
                {
                    addInt(caliper_collect_adjust,4);
                }
                else if(c2=='n')                                                                        // %Cn : needed flow multi for measurement
                {
                    if(caliper_collect_um && caliper_collect_count){
                        float dim = (float)caliper_filament_standard / (caliper_collect_um / caliper_collect_count);
                        float multi = 100.0f * dim*dim;
                        addInt((int)multi,3);
                    }else{
                        addInt(100,3);
                    }
                }
#endif //FEATURE_READ_CALIPER
#if FEATURE_DIGIT_FLOW_COMPENSATION
                if(c2=='U')                                                                        // %CU : digit flow lower limit
                {
                    addInt((int)g_nDigitFlowCompensation_Fmin,5);
                }
                else if(c2=='O')                                                                        // %CO : digit flow higher limit
                {
                    addInt((int)g_nDigitFlowCompensation_Fmax,5);
                }
                else if(c2=='F')                                                                        // %CF : digit flow flowrate
                {
                    addInt((int)g_nDigitFlowCompensation_intense,3); //eigentlcih sollten wir F und E hier tauschen... egal.
                }
                else if(c2=='E')                                                                        // %CE : digit flow feedrate
                {
                    addInt((int)g_nDigitFlowCompensation_speed_intense,3);
                }
#endif // FEATURE_DIGIT_FLOW_COMPENSATION
                break;
            }
            case 'L':
            {
                if(c2=='L')                                                                             // %LL : Last Layer (Direct + Queue + Extr. Zoffset)
                {
                    addFloat(float(Printer::queuePositionZLayerLast*Printer::invAxisStepsPerMM[Z_AXIS]),3,2);
                    break;
                }
                else if(c2=='C')                                                                        // %LC : Current Layer (Direct + Queue + Extr. Zoffset)
                {
                    addFloat(float(Printer::queuePositionZLayerCurrent*Printer::invAxisStepsPerMM[Z_AXIS]),3,2);
                    break;
                }
                else if(c2=='H')                                                                        // %LH : Layer Height
                {
                    addFloat(float(Printer::queuePositionZLayerCurrent-Printer::queuePositionZLayerLast)*Printer::invAxisStepsPerMM[Z_AXIS],1,2);
                    break;
                }
                else if(c2=='P')                                                                        // %LP : ECMP %
                {
                    addFloat(Printer::compensatedPositionOverPercE*100,1,2);
                    break;
                }
                else if(c2=='m')                                                                        // %Lm : g_minZCompensationSteps
                {
                    addFloat(float(g_minZCompensationSteps*Printer::invAxisStepsPerMM[Z_AXIS]),1,2);
                    break;
                }
                else if(c2=='M')                                                                        // %LM : g_maxZCompensationSteps
                {
                    addFloat(float(g_maxZCompensationSteps*Printer::invAxisStepsPerMM[Z_AXIS]),2,2);
                    break;
                }
                break;
            }
            case 'Z':                                                                                   // %Z1-Z4: Page5 service intervall, %Z5-Z8: Page4 printing/milling time
            {
                if(c2=='1')                                                                             // Shows text printing/milling time since last service
                {
#if FEATURE_SERVICE_INTERVAL
                    addStringP(PSTR(UI_TEXT_SERVICE_TIME));
#endif // FEATURE_SERVICE_INTERVAL
                }
                else if(c2=='2')                                                                        // Shows printing/milling time since last service
                {
#if FEATURE_SERVICE_INTERVAL
#if EEPROM_MODE!=0
                    char    mode = OPERATING_MODE_PRINT;


#if FEATURE_MILLING_MODE
                    mode = Printer::operatingMode;
#endif // FEATURE_MILLING_MODE

                    if ( mode == OPERATING_MODE_PRINT )
                    {
                        bool alloff = true;
                        for(uint8_t i=0; i<NUM_EXTRUDER; i++)
                            if(tempController[i]->targetTemperatureC>15) alloff = false;

                        long uSecondsServicePrint = (alloff ? 0 : (HAL::timeInMilliseconds()-Printer::msecondsPrinting)/1000) + HAL::eprGetInt32(EPR_PRINTING_TIME_SERVICE);
                        long tmp_service = uSecondsServicePrint/86400;
                        uSecondsServicePrint-=tmp_service*86400;
                        addInt(tmp_service,5);
                        addStringP(PSTR(UI_TEXT_PRINTTIME_DAYS));
                        tmp_service=uSecondsServicePrint/3600;
                        addInt(tmp_service,2);
                        addStringP(PSTR(UI_TEXT_PRINTTIME_HOURS));
                        uSecondsServicePrint-=tmp_service*3600;
                        tmp_service = uSecondsServicePrint/60;
                        addInt(tmp_service,2,'0');
                        addStringP(PSTR(UI_TEXT_PRINTTIME_MINUTES));
                    }
                    else
                    {
                        long uSecondsServicePrint = (HAL::timeInMilliseconds()-Printer::msecondsMilling)/1000 + HAL::eprGetInt32(EPR_MILLING_TIME_SERVICE);
                        long tmp_service = uSecondsServicePrint/86400;
                        uSecondsServicePrint-=tmp_service*86400;
                        addInt(tmp_service,5);
                        addStringP(PSTR(UI_TEXT_PRINTTIME_DAYS));
                        tmp_service=uSecondsServicePrint/3600;
                        addInt(tmp_service,2);
                        addStringP(PSTR(UI_TEXT_PRINTTIME_HOURS));
                        uSecondsServicePrint-=tmp_service*3600;
                        tmp_service = uSecondsServicePrint/60;
                        addInt(tmp_service,2,'0');
                        addStringP(PSTR(UI_TEXT_PRINTTIME_MINUTES));
                    }
#endif // EEPROM_MODE
#endif // FEATURE_SERVICE_INTERVAL
                }
                else if(c2=='3')                                                                        // Shows text printed filament since last service
                {
#if FEATURE_SERVICE_INTERVAL
                    char    mode = OPERATING_MODE_PRINT;


#if FEATURE_MILLING_MODE
                    mode = Printer::operatingMode;
#endif // FEATURE_MILLING_MODE

                    if ( mode == OPERATING_MODE_PRINT )
                    {
                        addStringP(PSTR(UI_TEXT_PRINT_FILAMENT));
                    }
                    else if ( mode == OPERATING_MODE_MILL )
                    {
                        //addStringP( PSTR( "" )); //TODO: Nibbels: Ist leerer string nötig?? Glaube nicht.
                    }
#endif // FEATURE_SERVICE_INTERVAL
                }
                else if(c2=='4')                                                                        // Shows printed filament since last service
                {
#if FEATURE_SERVICE_INTERVAL
                    char    mode = OPERATING_MODE_PRINT;


#if FEATURE_MILLING_MODE
                    mode = Printer::operatingMode;
#endif // FEATURE_MILLING_MODE

                    if ( mode == OPERATING_MODE_PRINT )
                    {
#if EEPROM_MODE!=0
                        float dist_service = Printer::filamentPrinted*0.001+HAL::eprGetFloat(EPR_PRINTING_DISTANCE_SERVICE);
                        addFloat(dist_service,6,1);
                        addStringP( PSTR( " m" ));
#endif // EEPROM_MODE
                    }
                    else if ( mode == OPERATING_MODE_MILL )
                    {
                        //addStringP( PSTR( "" )); //TODO: Nibbels: Ist leerer string nötig?? Glaube nicht.
                    }
#endif // FEATURE_SERVICE_INTERVAL
                }
                else if(c2=='5')                                                                        // Shows text printing/milling time
                {
                    char    mode = OPERATING_MODE_PRINT;


#if FEATURE_MILLING_MODE
                    mode = Printer::operatingMode;
#endif // FEATURE_MILLING_MODE

                    if ( mode == OPERATING_MODE_PRINT )
                    {
                        addStringP(PSTR(UI_TEXT_PRINT_TIME));
                    }
                    else if ( mode == OPERATING_MODE_MILL )
                    {
                        addStringP(PSTR(UI_TEXT_MILL_TIME));
                    }
                }
                else if(c2=='6')                                                                        // Shows printing/milling time                                                  
                {
#if EEPROM_MODE!=0
                    char    mode = OPERATING_MODE_PRINT;


#if FEATURE_MILLING_MODE
                    mode = Printer::operatingMode;
#endif // FEATURE_MILLING_MODE

                    if ( mode == OPERATING_MODE_PRINT )
                    {
                        bool alloff = true;
                        for(uint8_t i=0; i<NUM_EXTRUDER; i++)
                            if(tempController[i]->targetTemperatureC>15) alloff = false;

                        long seconds = (alloff ? 0 : (HAL::timeInMilliseconds()-Printer::msecondsPrinting)/1000) + HAL::eprGetInt32(EPR_PRINTING_TIME);
                        long tmp = seconds/86400;
                        seconds-=tmp*86400;
                        addInt(tmp,5);
                        addStringP(PSTR(UI_TEXT_PRINTTIME_DAYS));
                        tmp=seconds/3600;
                        addInt(tmp,2);
                        addStringP(PSTR(UI_TEXT_PRINTTIME_HOURS));
                        seconds-=tmp*3600;
                        tmp = seconds/60;
                        addInt(tmp,2,'0');
                        addStringP(PSTR(UI_TEXT_PRINTTIME_MINUTES));
                    }
                    else
                    {
                        long seconds = (HAL::timeInMilliseconds()-Printer::msecondsMilling)/1000 + HAL::eprGetInt32(EPR_MILLING_TIME);
                        long tmp = seconds/86400;
                        seconds-=tmp*86400;
                        addInt(tmp,5);
                        addStringP(PSTR(UI_TEXT_PRINTTIME_DAYS));
                        tmp=seconds/3600;
                        addInt(tmp,2);
                        addStringP(PSTR(UI_TEXT_PRINTTIME_HOURS));
                        seconds-=tmp*3600;
                        tmp = seconds/60;
                        addInt(tmp,2,'0');
                        addStringP(PSTR(UI_TEXT_PRINTTIME_MINUTES));
                    }
#endif // EEPROM_MODE
                }
                else if(c2=='7')                                                                        // Shows text printed filament
                {
                    char    mode = OPERATING_MODE_PRINT;


#if FEATURE_MILLING_MODE
                    mode = Printer::operatingMode;
#endif // FEATURE_MILLING_MODE

                    if ( mode == OPERATING_MODE_PRINT )
                    {
                        addStringP(PSTR(UI_TEXT_PRINT_FILAMENT));
                    }
                    else if ( mode == OPERATING_MODE_MILL )
                    {
                        //addStringP( PSTR( "" )); //TODO: Nibbels: Ist leerer string nötig?? Glaube nicht.
                    }
                }
                else if(c2=='8')                                                                        // Shows printed filament
                {
                    char    mode = OPERATING_MODE_PRINT;


#if FEATURE_MILLING_MODE
                    mode = Printer::operatingMode;
#endif // FEATURE_MILLING_MODE

                    if ( mode == OPERATING_MODE_PRINT )
                    {
#if EEPROM_MODE!=0
                        float dist = Printer::filamentPrinted*0.001+HAL::eprGetFloat(EPR_PRINTING_DISTANCE);
                        addFloat(dist,6,1);
                        addStringP( PSTR( " m" ));
#endif // EEPROM_MODE
                    }
                    else if ( mode == OPERATING_MODE_MILL )
                    {
                        //addStringP( PSTR( "" )); //TODO: Nibbels: Ist leerer string nötig?? Glaube nicht.
                    }
                }
                break;
            }
        }
    }
    printCols[col] = 0;

} // parse


void UIDisplay::setStatusP(PGM_P txt,bool error)
{
    if( locked )
    {
        // we shall not update the display
        return;
    }
    if(!error && Printer::isUIErrorMessage()) 
    {
        return;
    }

    uint8_t i=0;
    while(i<UI_COLS)
    {
        uint8_t c = pgm_read_byte(txt++);
        if(!c) break;
        statusMsg[i++] = c;
    }
    statusMsg[i]=0;
    if(error)
        Printer::setUIErrorMessage(true);

} // setStatusP


void UIDisplay::setStatus(char *txt,bool error,bool force)
{
    if( locked && !force )
    {
        // we shall not update the display
        return;
    }
    if(!error && Printer::isUIErrorMessage()) return;

    uint8_t i=0;
    while(*txt && i< UI_COLS )
        statusMsg[i++] = *txt++;
    statusMsg[i]=0;

    if(error)
        Printer::setUIErrorMessage(true);

} // setStatus


const UIMenu * const ui_pages[UI_NUM_PAGES] PROGMEM = UI_PAGES;

#if SDSUPPORT
uint8_t nFilesOnCard;
void UIDisplay::updateSDFileCount()
{
    dir_t* p = NULL;
    //byte offset = menuTop[menuLevel];
    SdBaseFile *root = sd.fat.vwd();

    root->rewind();
    nFilesOnCard = 0;
    while ((p = root->getLongFilename(p, NULL, 0, NULL)))
    {
    if (! (DIR_IS_FILE(p) || DIR_IS_SUBDIR(p)))
            continue;
        if (folderLevel>=SD_MAX_FOLDER_DEPTH && DIR_IS_SUBDIR(p) && !(p->name[0]=='.' && p->name[1]=='.'))
            continue;
        nFilesOnCard++;
        if (nFilesOnCard==254)
            return;
    }

} // updateSDFileCount


void getSDFilenameAt(byte filePos,char *filename)
{
    dir_t* p = NULL;
    //byte c=0;
    SdBaseFile *root = sd.fat.vwd();

    root->rewind();
    while ((p = root->getLongFilename(p, tempLongFilename, 0, NULL)))
    {
        if (!DIR_IS_FILE(p) && !DIR_IS_SUBDIR(p)) continue;
        if(uid.folderLevel>=SD_MAX_FOLDER_DEPTH && DIR_IS_SUBDIR(p) && !(p->name[0]=='.' && p->name[1]=='.')) continue;
        if (filePos--)
            continue;
        strcpy(filename, tempLongFilename);
        if(DIR_IS_SUBDIR(p)) strcat(filename, "/"); // Set marker for directory
        break;
    }

} // getSDFilenameAt


bool UIDisplay::isDirname(char *name)
{
    while(*name) name++;
    name--;
    return *name=='/';

} // isDirname


void UIDisplay::goDir(char *name)
{
    char *p = cwd;
    while(*p)p++;
    if(name[0]=='.' && name[1]=='.')
    {
        if(folderLevel==0) return;
        p--;
        p--;
        while(*p!='/') p--;
        p++;
        *p = 0;
        folderLevel--;
    }
    else
    {
        if(folderLevel>=SD_MAX_FOLDER_DEPTH) return;
        while(*name) *p++ = *name++;
        *p = 0;
        folderLevel++;
    }
    sd.fat.chdir(cwd);
    updateSDFileCount();

} // goDir


void sdrefresh(uint8_t &r,char cache[UI_ROWS][MAX_COLS+1])
{
    dir_t* p = NULL;
    byte offset = uid.menuTop[uid.menuLevel];
    SdBaseFile *root;
    byte length, skip;

    sd.fat.chdir(uid.cwd);
    root = sd.fat.vwd();
    root->rewind();

    skip = (offset>0?offset-1:0);

    while (r+offset<nFilesOnCard+1 && r<UI_ROWS && (p = root->getLongFilename(p, tempLongFilename, 0, NULL)))
    {
    // done if past last used entry
        // skip deleted entry and entries for . and  ..
        // only list subdirectories and files
        if ((DIR_IS_FILE(p) || DIR_IS_SUBDIR(p)))
        {
            if(uid.folderLevel >= SD_MAX_FOLDER_DEPTH && DIR_IS_SUBDIR(p) && !(p->name[0]=='.' && p->name[1]=='.'))
                continue;
            if(skip>0)
            {
                skip--;
                continue;
            }

            printCols[0] = ' ';
            uid.col = 1;

            if(DIR_IS_SUBDIR(p))
                printCols[uid.col++] = 6; // Prepend folder symbol
            length = RMath::min( (int)strlen(tempLongFilename), (int)(MAX_COLS-uid.col) );
            memcpy(printCols+uid.col, tempLongFilename, length);
            uid.col += length;
            printCols[uid.col] = 0;

            uint8_t curShift = (uid.shift<=0 ? 0 : uid.shift);
            uint8_t curLen = strlen(printCols);

            if(curLen>UI_COLS)
            {
                // this file name is longer than the available width of the display
                curShift = RMath::min(curLen-UI_COLS,curShift);
            }
            else
            {
                curShift = 0;
            }

            if(r+offset == uid.menuPos[uid.menuLevel])
            {
                // the menu cursor is placed at this file name at the moment
                printCols[curShift] = (char)CHAR_SELECTOR;
            }
            else
            {
                // this file name is above/below the current menu cursor item
                printCols[curShift] = ' ';
            }

            if(DIR_IS_SUBDIR(p))
                printCols[curShift+1] = 6; // Prepend folder symbol

            strcpy(cache[r++],printCols);
        }
    }

} // sdrefresh
#endif // SDSUPPORT


// Refresh current menu page
void UIDisplay::refreshPage()
{
    uint8_t r;
    uint8_t mtype = 0;
    char cache[UI_ROWS][MAX_COLS+1];
    adjustMenuPos();

#if FEATURE_MILLING_MODE
    if( Printer::operatingMode == OPERATING_MODE_PRINT )
    {
#if UI_PRINT_AUTORETURN_TO_MENU_AFTER
        // Reset timeout on menu back when user active on menu
        if (uid.encoderLast != encoderStartScreen)
            g_nAutoReturnTime=HAL::timeInMilliseconds()+UI_PRINT_AUTORETURN_TO_MENU_AFTER;
#endif // UI_PRINT_AUTORETURN_TO_MENU_AFTER
    }
    else
    {
#if UI_MILL_AUTORETURN_TO_MENU_AFTER
        // Reset timeout on menu back when user active on menu
        if (uid.encoderLast != encoderStartScreen)
            g_nAutoReturnTime=HAL::timeInMilliseconds()+UI_MILL_AUTORETURN_TO_MENU_AFTER;
#endif // UI_MILL_AUTORETURN_TO_MENU_AFTER
    }
#else
#if UI_PRINT_AUTORETURN_TO_MENU_AFTER
    // Reset timeout on menu back when user active on menu
    if (uid.encoderLast != encoderStartScreen)
        g_nAutoReturnTime=HAL::timeInMilliseconds()+UI_PRINT_AUTORETURN_TO_MENU_AFTER;
#endif // UI_PRINT_AUTORETURN_TO_MENU_AFTER
#endif // FEATURE_MILLING_MODE

    encoderStartScreen = uid.encoderLast;

    // Copy result into cache
    if(menuLevel==0)
    {
        UIMenu *men = (UIMenu*)pgm_read_word(&(ui_pages[menuPos[0]]));
        uint16_t nr = pgm_read_word_near(&(men->numEntries));
        UIMenuEntry **entries = (UIMenuEntry**)pgm_read_word(&(men->entries));
        for(r=0; r<nr && r<UI_ROWS; r++)
        {
            UIMenuEntry *ent =(UIMenuEntry *)pgm_read_word(&(entries[r]));
            col=0;
            parse((char*)pgm_read_word(&(ent->text)),false);
            strcpy(cache[r],printCols);
        }
    }
    else
    {
        UIMenu *men = (UIMenu*)menu[menuLevel];
        uint16_t nr = pgm_read_word_near(&(men->numEntries));
        mtype = pgm_read_byte((void*)&(men->menuType));
        uint8_t offset = menuTop[menuLevel];
        UIMenuEntry **entries = (UIMenuEntry**)pgm_read_word(&(men->entries));

        for(r=0; r+offset<nr && r<UI_ROWS; )
        {
            UIMenuEntry *ent =(UIMenuEntry *)pgm_read_word(&(entries[r+offset]));
            if(!ent->showEntry())
            {
                offset++;
                continue;
            }

            unsigned char entType = pgm_read_byte(&(ent->menuType));
            int entAction = (int)pgm_read_word(&(ent->action));

            printCols[0] = ' ';
            col = 1;
            parse((char*)pgm_read_word(&(ent->text)),false);
            while(col<UI_COLS-1) printCols[col++] = ' ';
            printCols[col] = 0;

            if(entType>=2 && entType<=4)
            {
                // this is a menu item
                uint8_t curShift = (shift<=0 ? 0 : shift);
                uint8_t curLen = strlen(printCols);

                if(entType==2)
                {
                    // this menu item contains submenus
                    curLen ++;  // one additional character is needed for the submenu marker
                }

                if(curLen>UI_COLS)
                {
                    // this menu item is longer than the available width of the display
                    curShift = RMath::min(curLen-UI_COLS,curShift);
                }
                else
                {
                    curShift = 0;
                }

                if(r+offset==menuPos[menuLevel] && activeAction!=entAction)
                {
                    // the menu cursor is placed at this item at the moment
                    printCols[curShift] = (char)CHAR_SELECTOR;
                }
                else if(activeAction==entAction)
                {
                    // this menu item is selected (and can be changed) at the moment
                    printCols[curShift] = CHAR_SELECTED;
                }
                else
                {
                    // this menu item is above/below the current menu cursor item
                    printCols[curShift] = ' ';
                }

                if(entType==2)
                {
                    printCols[UI_COLS-1 + curShift] = (char)CHAR_RIGHT;   // arrow right
                    printCols[UI_COLS + curShift] = 0;              // arrow right
                }
            }

            strcpy(cache[r],printCols);
            r++;
        }
    }

#if SDSUPPORT
    if(mtype==1)
    {
        sdrefresh(r,cache);
    }
#endif // SDSUPPORT

    printCols[0]=0;
    while(r<UI_ROWS)
        strcpy(cache[r++],printCols);
    
    // Compute transition
    uint8_t transition = 0; // 0 = display, 1 = up, 2 = down, 3 = left, 4 = right

#if UI_ANIMATION
    if(menuLevel != oldMenuLevel && !PrintLine::hasLines())
    {
        if(oldMenuLevel == 0 || oldMenuLevel == -2)
            transition = 1;
        else if(menuLevel == 0)
            transition = 2;
        else if(menuLevel>oldMenuLevel)
            transition = 3;
        else
            transition = 4;
    }
#endif // UI_ANIMATION

    uint8_t loops = 1;
    uint8_t dt = 1,y;
    if(transition == 1 || transition == 2) loops = UI_ROWS;
    else if(transition>2)
    {
        dt = (UI_COLS+UI_COLS-1)/16;
        loops = UI_COLS+1/dt;
    }
    uint8_t off0 = (shift<=0 ? 0 : shift);
    uint8_t scroll = dt;
    uint8_t off[UI_ROWS];
    if(transition == 0)
    {
        for(y=0; y<UI_ROWS; y++)
            strcpy(displayCache[y],cache[y]);
    }
    for(y=0; y<UI_ROWS; y++)
    {
        uint8_t len = strlen(displayCache[y]);
        off[y] = len>UI_COLS ? RMath::min(len-UI_COLS,off0) : 0;

#if UI_ANIMATION
        if(transition == 3)
        {
            for(r=len; r<MAX_COLS; r++)
            {
                displayCache[y][r] = 32;
            }
            displayCache[y][MAX_COLS] = 0;
        }
        else if(transition == 4)
        {
            for(r=strlen(cache[y]); r<MAX_COLS; r++)
            {
                cache[y][r] = 32;
            }
            cache[y][MAX_COLS] = 0;
        }
#endif // UI_ANIMATION
    }
    for(uint8_t l=0; l<loops; l++)
    {
        if(uid.encoderLast != encoderStartScreen)
        {
            scroll = 200;
        }
        scroll += dt;

        if(transition == 0)
        {
            for(y=0; y<UI_ROWS; y++)
                printRow(y,&cache[y][off[y]],NULL,UI_COLS);
        }
#if UI_ANIMATION
        else
        {
            if(transition == 1)   // up
            {
                if(scroll > UI_ROWS)
                {
                    scroll = UI_ROWS;
                    l = loops;
                }
                for(y=0; y<UI_ROWS-scroll; y++)
                {
                    r = y+scroll;
                    printRow(y,&displayCache[r][off[r]],NULL,UI_COLS);
                }
                for(y=0; y<scroll; y++)
                {
                    printRow(UI_ROWS-scroll+y,cache[y],NULL,UI_COLS);
                }
            }
            else if(transition == 2)     // down
            {
                if(scroll > UI_ROWS)
                {
                    scroll = UI_ROWS;
                    l = loops;
                }
                for(y=0; y<scroll; y++)
                {
                    printRow(y,cache[UI_ROWS-scroll+y],NULL,UI_COLS);
                }
                for(y=0; y<UI_ROWS-scroll; y++)
                {
                    r = y+scroll;
                    printRow(y+scroll,&displayCache[y][off[y]],NULL,UI_COLS);
                }
            }
            else if(transition == 3)     // left
            {
                if(scroll > UI_COLS)
                {
                    scroll = UI_COLS;
                    l = loops;
                }
                for(y=0; y<UI_ROWS; y++)
                {
                    printRow(y,&displayCache[y][off[y]+scroll],cache[y],UI_COLS-scroll);
                }
            }
            else     // right
            {
                if(scroll > UI_COLS)
                {
                    scroll = UI_COLS;
                    l = loops;
                }
                for(y=0; y<UI_ROWS; y++)
                {
                    printRow(y,cache[y]+UI_COLS-scroll,&displayCache[y][off[y]],scroll);
                }
            }
#if DISPLAY_TYPE != 5
            HAL::delayMilliseconds(transition<3 ? 200 : 70);
#endif // DISPLAY_TYPE != 5
    }
#endif // UI_ANIMATION
    }

#if UI_ANIMATION
    // copy to last cache
    if(transition != 0)
        for(y=0; y<UI_ROWS; y++)
            strcpy(displayCache[y],cache[y]);
    oldMenuLevel = menuLevel;
#endif // UI_ANIMATION

} // refreshPage


void UIDisplay::showMessage(bool refresh)
{
    pushMenu( (void*)&ui_menu_message, refresh );

    // show this message until the user acknowledges it
    g_nAutoReturnTime = 0;

} // showMessage


void UIDisplay::pushMenu(void *men,bool refresh)
{
    if(men==menu[menuLevel])
    {
        refreshPage();
        return;
    }
    if( menuLevel == (MAX_MENU_LEVELS-1) )
    {
        return;
    }
    menuLevel++;
    menu[menuLevel]=men;
    menuTop[menuLevel] = menuPos[menuLevel] = 0;

#if SDSUPPORT
    UIMenu *men2 = (UIMenu*)menu[menuLevel];
    if(pgm_read_byte(&(men2->menuType))==1) // Open files list
        updateSDFileCount();
#endif // SDSUPPORT

    if(refresh)
        refreshPage();

} // pushMenu


void UIDisplay::okAction()
{
    if(Printer::isUIErrorMessage())
    {
        Printer::setUIErrorMessage(false);
        return;
    }

#if UI_HAS_KEYS==1
    if(menuLevel==0)   // Enter menu
    {
        menuLevel = 1;
        menuTop[1] = menuPos[1] = 0;
        menu[1] = (void*)&ui_menu_main;
        BEEP_SHORT
        return;
    }

    UIMenu *men = (UIMenu*)menu[menuLevel];
    uint8_t mtype = pgm_read_byte(&(men->menuType));
    UIMenuEntry **entries = (UIMenuEntry**)pgm_read_word(&(men->entries));
    UIMenuEntry *ent =(UIMenuEntry *)pgm_read_word(&(entries[menuPos[menuLevel]]));
    unsigned char entType = pgm_read_byte(&(ent->menuType));// 0 = Info, 1 = Headline, 2 = submenu ref, 3 = direct action command, 4 = modify action
    int action = pgm_read_word(&(ent->action));

    if(mtype==3)   // action menu
    {
        action = pgm_read_word(&(men->id));
        finishAction(action);
        executeAction(UI_ACTION_BACK);
        return;
    }
    if(mtype==2 && entType==4)   // Modify action
    {
        if(activeAction)   // finish action
        {
            finishAction(action);
            activeAction = 0;
        }
        else
            activeAction = action;
        return;
    }

#if SDSUPPORT
    if(mtype==1)
    {
        if(menuPos[menuLevel]==0)   // Selected back instead of file
        {
            executeAction(UI_ACTION_BACK);
            return;
        }
        if(!sd.sdactive)
            return;

        uint8_t filePos = menuPos[menuLevel]-1;
        char filename[LONG_FILENAME_LENGTH+1];

        getSDFilenameAt(filePos, filename);
        if(isDirname(filename))   // Directory change selected
        {
            goDir(filename);
            menuTop[menuLevel]=0;
            menuPos[menuLevel]=1;
            refreshPage();
            oldMenuLevel = -1;
            return;
        }

        int16_t action;
        if (Printer::isAutomount())
            action = UI_ACTION_SD_PRINT;
        else
        {
            men = (UIMenu*)menu[menuLevel-1];
            entries = (UIMenuEntry**)pgm_read_word(&(men->entries));
            ent =(UIMenuEntry *)pgm_read_word(&(entries[menuPos[menuLevel-1]]));
            action = pgm_read_word(&(ent->action));
        }
        sd.file.close();
        sd.fat.chdir(cwd);
        switch(action)
        {
            case UI_ACTION_SD_PRINT:
            {
                if (sd.selectFile(filename, false))
                {
                    sd.startPrint();
                    BEEP_START_PRINTING
                    uid.executeAction(UI_ACTION_TOP_MENU);
                }
                break;
            }
            case UI_ACTION_SD_DELETE:
            {
                if(sd.sdactive)
                {
                    if(Printer::isMenuMode(MENU_MODE_SD_PRINTING))
                    {
                        // we do not allow to delete a file while we are printing/milling from the SD card
                        if( Printer::debugErrors() )
                        {
                            Com::printFLN(PSTR("delete error: processing"));
                        }

                        showError( (void*)ui_text_delete_file, (void*)ui_text_operation_denied );
                        break;
                    }

                    sd.sdmode = false;
                    sd.file.close();
                    if(sd.fat.remove(filename))
                    {
                        if( Printer::debugInfo() )
                        {
                            Com::printFLN(Com::tFileDeleted);
                        }
                        BEEP_LONG
                    }
                    else
                    {
                        if( Printer::debugErrors() )
                        {
                            Com::printFLN(Com::tDeletionFailed);
                        }
                    }
                }
                break;
            }
        }
        return;
    }
#endif // SDSUPPORT

    if(entType==2)   // Enter submenu
    {
        pushMenu((void*)action,false);
        BEEP_SHORT
        return;
    }
    if(entType==3)
    {
        executeAction(action);
        return;
    }
    executeAction(UI_ACTION_BACK);
#endif // UI_HAS_KEYS==1

} // okAction


void UIDisplay::rightAction()
{
#if FEATURE_SENSIBLE_PRESSURE
    if( uid.menuLevel == 0 && uid.menuPos[0] == 1 ){ //wenn im Mod-Menü für Z-Offset/Matrix Sense-Offset/Limiter, dann anders!
        //we are in the Mod menu        
        if(g_nSensiblePressureDigits == EMERGENCY_PAUSE_DIGITS_MAX * 0.8 || g_nSensiblePressureDigits == 32767){
            //ist max, dann auf 0.
            g_nSensiblePressureDigits = 0;          
        }else if(g_nSensiblePressureDigits > EMERGENCY_PAUSE_DIGITS_MAX * 0.8 - 250 || g_nSensiblePressureDigits > 32767 - 250){    
            //stößt oben an, hier noch check auf overflow:
            if(EMERGENCY_PAUSE_DIGITS_MAX * 0.8 < 32767){
                g_nSensiblePressureDigits = EMERGENCY_PAUSE_DIGITS_MAX * 0.8; //maximalstellung, das ist aber schon irre hoch. abhängig von messdose und maximaltragkraft vs. genauigkeit der zelle.
            }else{
                g_nSensiblePressureDigits = 32767; //die variable ist short, nie mehr rein als nötig ^^.
            }
        }else{
            //TPE braucht mini werte, wenn es sinnvoll sein soll. Darum der Ternary, sodass man per Knopf auch kleinste Zahlen justieren kann.
            g_nSensiblePressureDigits += (g_nSensiblePressureDigits >= 2000) ? 250 : (g_nSensiblePressureDigits >= 500) ? 100 : 50 ; //decrement pro Knopfklick. Man kann ja auf der Taste bleiben.
            //g_nSensiblePressureDigits += 250; //decrement pro Knopfklick. Man kann ja auf der Taste bleiben.
#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            //Wir speichern nur Werte automatisch per Knopf, die im Alltag sinn machen können. Ab 500:
            short oldval = HAL::eprGetInt16(EPR_RF_MOD_SENSEOFFSET_DIGITS);
            if(g_nSensiblePressureDigits >= 500 && oldval != g_nSensiblePressureDigits){
                HAL::eprSetInt16( EPR_RF_MOD_SENSEOFFSET_DIGITS, g_nSensiblePressureDigits );
                EEPROM::updateChecksum(); //deshalb die prüfung
            }
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
        }
        beep(1,4);
    }else{
#endif
#if UI_HAS_KEYS==1
    if( menu[menuLevel] == &ui_menu_xpos )
    {
        Printer::moveMode[X_AXIS] ++;
        if( Printer::moveMode[X_AXIS] > MOVE_MODE_50_MM )
        {
            Printer::moveMode[X_AXIS] = MOVE_MODE_SINGLE_STEPS;
        }
        refreshPage();

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
        HAL::eprSetByte(EPR_RF_MOVE_MODE_X,Printer::moveMode[X_AXIS]);
        EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
    }
    else if( menu[menuLevel] == &ui_menu_ypos )
    {
        Printer::moveMode[Y_AXIS] ++;
        if( Printer::moveMode[Y_AXIS] > MOVE_MODE_50_MM )
        {
            Printer::moveMode[Y_AXIS] = MOVE_MODE_SINGLE_STEPS;
        }
        refreshPage();

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
        HAL::eprSetByte(EPR_RF_MOVE_MODE_Y,Printer::moveMode[Y_AXIS]);
        EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
    }
    else if( menu[menuLevel] == &ui_menu_zpos )
    {
        Printer::moveMode[Z_AXIS] ++;
        if( Printer::moveMode[Z_AXIS] > MOVE_MODE_50_MM )
        {
            Printer::moveMode[Z_AXIS] = MOVE_MODE_SINGLE_STEPS;
        }
        refreshPage();

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
        HAL::eprSetByte(EPR_RF_MOVE_MODE_Z,Printer::moveMode[Z_AXIS]);
        EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
    }
#endif // UI_HAS_KEYS==1    
#if FEATURE_SENSIBLE_PRESSURE   
    }
#endif
} // rightAction

//increment ist ne variable, global -> Knopfrichtung.
//#define INCREMENT_MIN_MAX(a,steps,_min,_max) if ( (increment<0) && (_min>=0) && (a<_min-increment*steps) ) {a=_min;} else { a+=increment*steps; if(a<_min) a=_min; else if(a>_max) a=_max;};
// this version not have single byte variable rollover bug
#define INCREMENT_MIN_MAX(a,steps,_min,_max) a = constrain((a + increment * steps), _min, _max);

#define INCREMENT_MAX(a,steps,_max) if ( increment < 0 && a <= -1*increment*steps ) a=0; else { a+=increment*steps; if(a>_max) a=_max;};
//#define INCREMENT_MAX(a,steps,_max) a = constrain((a + increment * steps), 0, _max); //Nibbels: ist für unsigned variablen gegen warning. 2x vorkommen

void UIDisplay::adjustMenuPos()
{
    if(menuLevel == 0) return;
    UIMenu *men = (UIMenu*)menu[menuLevel];
    UIMenuEntry **entries = (UIMenuEntry**)pgm_read_word(&(men->entries));
    uint8_t mtype = HAL::readFlashByte((const prog_char*)&(men->menuType));
    if(mtype != 2) return;

    while(menuPos[menuLevel]>0)
    {
        if(((UIMenuEntry *)pgm_read_word(&(entries[menuPos[menuLevel]])))->showEntry())
            break;

        menuPos[menuLevel]--;
    }
    while(1)
    {
        if(((UIMenuEntry *)pgm_read_word(&(entries[menuPos[menuLevel]])))->showEntry())
            break;

        menuPos[menuLevel]++;
    }
    uint8_t skipped = 0;
    bool modified;

    if(menuTop[menuLevel] > menuPos[menuLevel])
        menuTop[menuLevel] = menuPos[menuLevel];
    do
    {
        skipped = 0;
        modified = false;
        for(uint8_t r=menuTop[menuLevel]; r<menuPos[menuLevel]; r++)
        {
            UIMenuEntry *ent =(UIMenuEntry *)pgm_read_word(&(entries[r]));
            if(!ent->showEntry())
                skipped++;
        }
        if(menuTop[menuLevel] + skipped + UI_ROWS - 1 < menuPos[menuLevel]) {
            menuTop[menuLevel] = menuPos[menuLevel] + 1 - UI_ROWS;
            modified = true;
        }

    }while(modified);

} // adjustMenuPos


void UIDisplay::nextPreviousAction(int8_t next)
{
    if(Printer::isUIErrorMessage())
    {
        Printer::setUIErrorMessage(false);
        return;
    }
    millis_t actTime = HAL::timeInMilliseconds();
    millis_t dtReal;
    millis_t dt = dtReal = actTime-lastNextPrev;
    lastNextPrev = actTime;

    if(dt<SPEED_MAX_MILLIS) dt = SPEED_MAX_MILLIS;
    if(dt>SPEED_MIN_MILLIS)
    {
        dt = SPEED_MIN_MILLIS;
        lastNextAccumul = 1;
    }
    float f = (float)(SPEED_MIN_MILLIS-dt)/(float)(SPEED_MIN_MILLIS-SPEED_MAX_MILLIS);
    lastNextAccumul = 1.0f+(float)SPEED_MAGNIFICATION*f*f*f;

#if UI_HAS_KEYS==1
    if(menuLevel==0)
    {
        char    mode = OPERATING_MODE_PRINT;


#if FEATURE_MILLING_MODE
        mode = Printer::operatingMode;
#endif // FEATURE_MILLING_MODE

        lastSwitch = HAL::timeInMilliseconds();
        if((UI_INVERT_MENU_DIRECTION && next<0) || (!UI_INVERT_MENU_DIRECTION && next>0))
        {
            if ( mode == OPERATING_MODE_PRINT )
            {
                menuPos[0]++;
                if(menuPos[0]>=UI_NUM_PAGES)
                    menuPos[0]=0;
            }
            else if ( mode == OPERATING_MODE_MILL )
            {
                menuPos[0]++;
                if ( menuPos[0] == 1 || menuPos[0] == 3 ) //kein modmenü und kein temperaturmenü im Millingmode
                {
                    menuPos[0]++;
                }
                if(menuPos[0]>=UI_NUM_PAGES)
                {
                    menuPos[0]=0;
                }
            }
        }
        else
        {
            if ( mode == OPERATING_MODE_PRINT )
            {
                menuPos[0] = (menuPos[0]==0 ? UI_NUM_PAGES-1 : menuPos[0]-1);
            }
            else if ( mode == OPERATING_MODE_MILL )
            {
                menuPos[0] = (menuPos[0]==0 ? UI_NUM_PAGES-1 : menuPos[0]-1);
                if ( menuPos[0] == 1 || menuPos[0] == 3 ) //kein modmenü und kein temperaturmenü im Millingmode
                {
                    menuPos[0]--; //kann in diesem if nicht -1 werden, könnte es aber bei veränderung!
                }
            }
        }
        return;
    }

    UIMenu *men = (UIMenu*)menu[menuLevel];
    uint8_t nr = (uint8_t)pgm_read_word_near(&(men->numEntries));
    uint8_t mtype = HAL::readFlashByte((const prog_char*)&(men->menuType));
    UIMenuEntry **entries = (UIMenuEntry**)pgm_read_word(&(men->entries));
    UIMenuEntry *ent =(UIMenuEntry *)pgm_read_word(&(entries[menuPos[menuLevel]]));
    UIMenuEntry *testEnt;
    uint8_t entType = HAL::readFlashByte((const prog_char*)&(ent->menuType));   // 0 = Info, 1 = Headline, 2 = submenu ref, 3 = direct action command
    (void)entType; //ignore unused error Nibbels
    int action = pgm_read_word(&(ent->action));
    
    if(mtype==2 && activeAction==0)   // browse through menu items
    {
        if((UI_INVERT_MENU_DIRECTION && next < 0) || (!UI_INVERT_MENU_DIRECTION && next > 0))
        {
                //up-to-bottom-Patch
                uint8_t vorher = menuPos[menuLevel];
                if(menuPos[menuLevel] < nr-1) menuPos[menuLevel]++;
                else menuPos[menuLevel] = 0; 
                //gehe maximal einmal im kreis, auch wenn keins der menüpunkte sauber konfiguriert ist ^^.
                while(menuPos[menuLevel] != vorher)
                {                
                    testEnt = (UIMenuEntry *)pgm_read_word(&(entries[menuPos[menuLevel]]));
                    if(testEnt->showEntry()) break;
                
                    if(menuPos[menuLevel] < nr-1) menuPos[menuLevel]++;
                    else menuPos[menuLevel] = 0; //0..nr-1; nr ist anzahl submenüpunkte
                }
                //alle untermenüpunkte sind entweder nicht vorhanden oder falsch oder unzulässig: also geh einfach zurück auf das was anfangs dastand.
                testEnt = (UIMenuEntry *)pgm_read_word(&(entries[menuPos[menuLevel]]));
                if(!testEnt->showEntry())
                {
                    // this new chosen menu item shall not be displayed - revert the so-far used menu item
                    menuPos[menuLevel] = vorher;
                }
                //up-to-bottom-Patch
         }
         else 
         {
                //down-to-top-Patch
                uint8_t vorher = menuPos[menuLevel];
                if(menuPos[menuLevel] > 0) menuPos[menuLevel]--;
                else menuPos[menuLevel] = nr-1; //0..nr-1; nr ist anzahl submenüpunkte
                //gehe maximal einmal im kreis, auch wenn keins der menüpunkte sauber konfiguriert ist ^^.
                while(menuPos[menuLevel] != vorher)
                {                
                    testEnt = (UIMenuEntry *)pgm_read_word(&(entries[menuPos[menuLevel]]));
                    if(testEnt->showEntry()) break;
                    
                    if(menuPos[menuLevel] > 0) menuPos[menuLevel]--;
                    else menuPos[menuLevel] = nr-1; //0..nr-1; nr ist anzahl submenüpunkte
                }
                //alle untermenüpunkte sind entweder nicht vorhanden oder falsch oder unzulässig: also geh einfach zurück auf das was anfangs dastand.
                testEnt = (UIMenuEntry *)pgm_read_word(&(entries[menuPos[menuLevel]]));
                if(!testEnt->showEntry())
                {
                    // this new chosen menu item shall not be displayed - revert the so-far used menu item
                    menuPos[menuLevel] = vorher;
                }
                //down-to-top-Patch ende
        }
        adjustMenuPos();
        return;
    }

#if SDSUPPORT
    if(mtype==1)   // SD listing
    {
        if((UI_INVERT_MENU_DIRECTION && next<0) || (!UI_INVERT_MENU_DIRECTION && next>0))
        {
            if(menuPos[menuLevel]<nFilesOnCard) menuPos[menuLevel]++;
        }
        else if(menuPos[menuLevel]>0)
        {
            menuPos[menuLevel]--;

            if( Printer::debugInfo() )
            {
                Com::printF( PSTR( "SD listing: " ), menuPos[menuLevel] );
                Com::printFLN( PSTR( " / " ), menuLevel );
            }
        }
        if(menuTop[menuLevel]>menuPos[menuLevel])
            menuTop[menuLevel]=menuPos[menuLevel];
        else if(menuTop[menuLevel]+UI_ROWS-1<menuPos[menuLevel])
            menuTop[menuLevel]=menuPos[menuLevel]+1-UI_ROWS;
        return;
    }
#endif // SDSUPPORT

    if(mtype==3) action = pgm_read_word(&(men->id));
    else action=activeAction;

#if UI_INVERT_INCREMENT_DIRECTION
    int8_t increment = -next;
#else
    int8_t increment = next;
#endif // UI_INVERT_INCREMENT_DIRECTION

    switch(action)
    {
#if FAN_PIN>-1 && FEATURE_FAN_CONTROL
        case UI_ACTION_FANSPEED:
        {
            Commands::setFanSpeed(Printer::getFanSpeed()+increment,false);
            break;
        }
#endif // FAN_PIN>-1 && FEATURE_FAN_CONTROL
        case UI_ACTION_FAN_HZ:
        {
            if(increment > 0){
                Commands::adjustFanFrequency( (cooler_pwm_speed == COOLER_MODE_MAX ? 0 : cooler_pwm_speed + 1 ) ); //0 = ~15hz, ~1=30hz, ... 4=240hz.
            }else{
                Commands::adjustFanFrequency( (cooler_pwm_speed == 0 ? COOLER_MODE_MAX : cooler_pwm_speed - 1 ) ); //0 = ~15hz, ~1=30hz, ... 4=240hz.
            }
#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            HAL::eprSetByte( EPR_RF_FAN_SPEED, cooler_pwm_speed );
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
            break;
        }
        case UI_ACTION_XPOSITION:
        {
            /*
            XYZ_POSITION_BUTTON_DIRECTION = -1 : This fits to you if you want more intuitivity when choosing the Up-Down-Buttons.
            XYZ_POSITION_BUTTON_DIRECTION = 1 : This fits more if you stick to coordinates direction.
            see configuration.h
            */
            nextPreviousXAction( XYZ_POSITION_BUTTON_DIRECTION*increment );
            break;
        }
        case UI_ACTION_YPOSITION:
        {
            /*
            XYZ_POSITION_BUTTON_DIRECTION = -1 : This fits to you if you want more intuitivity when choosing the Up-Down-Buttons.
            XYZ_POSITION_BUTTON_DIRECTION = 1 : This fits more if you stick to coordinates direction.
            see configuration.h
            */
            nextPreviousYAction( XYZ_POSITION_BUTTON_DIRECTION*increment );
            break;
        }
        case UI_ACTION_ZPOSITION:
        {
            /*
            XYZ_POSITION_BUTTON_DIRECTION = -1 : This fits to you if you want more intuitivity when choosing the Up-Down-Buttons.
            XYZ_POSITION_BUTTON_DIRECTION = 1 : This fits more if you stick to coordinates direction.
            see configuration.h
            */
            nextPreviousZAction( XYZ_POSITION_BUTTON_DIRECTION*increment );
            break;
        }            
        case UI_ACTION_ZOFFSET:
        {           
            //INCREMENT_MIN_MAX(Printer::ZOffset,Z_OFFSET_STEP,-5000,5000);
            INCREMENT_MIN_MAX(Printer::ZOffset,Z_OFFSET_STEP,-(HEAT_BED_Z_COMPENSATION_MAX_MM * 1000),(HEAT_BED_Z_COMPENSATION_MAX_MM * 1000));     
        #if FEATURE_SENSIBLE_PRESSURE
            g_staticZSteps = ((Printer::ZOffset+g_nSensiblePressureOffset) * Printer::axisStepsPerMM[Z_AXIS]) / 1000;
        #else   
            g_staticZSteps = (Printer::ZOffset * Printer::axisStepsPerMM[Z_AXIS]) / 1000;
        #endif
#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            HAL::eprSetInt32( EPR_RF_Z_OFFSET, Printer::ZOffset );
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
            break;
        }
        case UI_ACTION_XPOSITION_FAST:
        {
            PrintLine::moveRelativeDistanceInStepsReal(Printer::axisStepsPerMM[X_AXIS]*increment,0,0,0,Printer::homingFeedrate[X_AXIS],true);
            Commands::printCurrentPosition();
            break;
        }
        case UI_ACTION_YPOSITION_FAST:
        {
            PrintLine::moveRelativeDistanceInStepsReal(0,Printer::axisStepsPerMM[Y_AXIS]*increment,0,0,Printer::homingFeedrate[Y_AXIS],true);
            Commands::printCurrentPosition();
            break;
        }
        case UI_ACTION_ZPOSITION_FAST:
        {
            PrintLine::moveRelativeDistanceInStepsReal(0,0,Printer::axisStepsPerMM[Z_AXIS]*increment,0,Printer::homingFeedrate[Z_AXIS],true);
            Commands::printCurrentPosition();
            break;
        }
        case UI_ACTION_EPOSITION:
        {
#if EXTRUDER_ALLOW_COLD_MOVE
            PrintLine::moveRelativeDistanceInSteps(0,0,0, Printer::axisStepsPerMM[E_AXIS]*increment / Printer::extrusionFactor, UI_SET_EXTRUDER_FEEDRATE, true, false); //klappt nicht in Pause!!
            Commands::printCurrentPosition();
#else
            if( Extruder::current->tempControl.targetTemperatureC > UI_SET_MIN_EXTRUDER_TEMP )
            {
                PrintLine::moveRelativeDistanceInSteps(0,0,0, Printer::axisStepsPerMM[E_AXIS]*increment / Printer::extrusionFactor, UI_SET_EXTRUDER_FEEDRATE, true, false);
                Commands::printCurrentPosition();
            }
            else
            {
                showError( (void*)ui_text_extruder, (void*)ui_text_operation_denied );
            }
#endif // EXTRUDER_ALLOW_COLD_MOVE
            break;
        }
/*
        case UI_ACTION_ZPOSITION_NOTEST:
        {
            Printer::setNoDestinationCheck(true);

#if UI_SPEEDDEPENDENT_POSITIONING
            float d = 0.01*(float)increment*lastNextAccumul;
            if(fabs(d)*2000>Printer::maxFeedrate[Z_AXIS]*dtReal)
                d *= Printer::maxFeedrate[Z_AXIS]*dtReal/(2000*fabs(d));
            long steps = (long)(d*Printer::axisStepsPerMM[Z_AXIS]);
            steps = ( increment<0 ? RMath::min(steps,(long)increment) : RMath::max(steps,(long)increment));
            PrintLine::moveRelativeDistanceInStepsReal(0,0,steps,0,Printer::maxFeedrate[Z_AXIS],true);
#else
            PrintLine::moveRelativeDistanceInStepsReal(0,0,increment,0,Printer::homingFeedrate[Z_AXIS],true);
#endif // UI_SPEEDDEPENDENT_POSITIONING

            Commands::printCurrentPosition();
            Printer::setNoDestinationCheck(false);
            break;
        }
        case UI_ACTION_ZPOSITION_FAST_NOTEST:
        {
            Printer::setNoDestinationCheck(true);
            PrintLine::moveRelativeDistanceInStepsReal(0,0,Printer::axisStepsPerMM[Z_AXIS]*increment,0,Printer::homingFeedrate[Z_AXIS],true);
            Commands::printCurrentPosition();
            Printer::setNoDestinationCheck(false);
            break;
        }
*/
        case UI_ACTION_HEATED_BED_TEMP:
        {
#if HAVE_HEATED_BED==true
            int tmp = (int)heatedBedController.targetTemperatureC;
            if(tmp<UI_SET_MIN_HEATED_BED_TEMP) tmp = 0;
            tmp+=increment;
            if(tmp==1) tmp = UI_SET_MIN_HEATED_BED_TEMP;
            if(tmp<UI_SET_MIN_HEATED_BED_TEMP) tmp = 0;
            else if(tmp>UI_SET_MAX_HEATED_BED_TEMP) tmp = UI_SET_MAX_HEATED_BED_TEMP;
            Extruder::setHeatedBedTemperature(tmp);
#endif // HAVE_HEATED_BED

            break;
        }
        case UI_ACTION_EXTRUDER0_TEMP:
        {
            int tmp = (int)extruder[0].tempControl.targetTemperatureC;
            if(tmp<UI_SET_MIN_EXTRUDER_TEMP) tmp = 0;
            tmp+=increment;
            if(tmp==1) tmp = UI_SET_MIN_EXTRUDER_TEMP;
            if(tmp<UI_SET_MIN_EXTRUDER_TEMP) tmp = 0;
            else if(tmp>UI_SET_MAX_EXTRUDER_TEMP) tmp = UI_SET_MAX_EXTRUDER_TEMP;
            Extruder::setTemperatureForExtruder(tmp,0);
            break;
        }
        case UI_ACTION_EXTRUDER1_TEMP:
        {
#if NUM_EXTRUDER>1
            int tmp = (int)extruder[1].tempControl.targetTemperatureC;
            tmp+=increment;
            if(tmp==1) tmp = UI_SET_MIN_EXTRUDER_TEMP;
            if(tmp<UI_SET_MIN_EXTRUDER_TEMP) tmp = 0;
            else if(tmp>UI_SET_MAX_EXTRUDER_TEMP) tmp = UI_SET_MAX_EXTRUDER_TEMP;
            Extruder::setTemperatureForExtruder(tmp,1);
#endif // NUM_EXTRUDER>1

            break;
        }

#if NUM_EXTRUDER>1
        case UI_ACTION_EXTRUDER_OFFSET_X:
        {
            float   fTemp = extruder[1].xOffset / Printer::axisStepsPerMM[X_AXIS];
            INCREMENT_MIN_MAX(fTemp,0.01,32,36);
            extruder[1].xOffset = int32_t(fTemp * Printer::axisStepsPerMM[X_AXIS]);

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            HAL::eprSetFloat(EEPROM::getExtruderOffset(1)+EPR_EXTRUDER_X_OFFSET,fTemp);
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

            break;
        }
        case UI_ACTION_EXTRUDER_OFFSET_Y:
        {
            float   fTemp = extruder[1].yOffset / Printer::axisStepsPerMM[Y_AXIS];
            INCREMENT_MIN_MAX(fTemp,0.01,-2,2);
            extruder[1].yOffset = int32_t(fTemp * Printer::axisStepsPerMM[Y_AXIS]);

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            HAL::eprSetFloat(EEPROM::getExtruderOffset(1)+EPR_EXTRUDER_Y_OFFSET,fTemp);
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

            break;
        }
        case UI_ACTION_EXTRUDER_OFFSET_Z:
        {
            //Das hier ist nur dazu gedacht, um eine Tip-Down-Nozzle auf per ToolChange auf die Korrekte Höhe zu justieren.
            float   fTemp = extruder[1].zOffset * Printer::invAxisStepsPerMM[Z_AXIS]; //mm negativ
            INCREMENT_MIN_MAX(fTemp,0.025,-2,0);
            extruder[1].zOffset = int32_t(fTemp * Printer::axisStepsPerMM[Z_AXIS]); //wieder zu steps negativ

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            HAL::eprSetFloat(EEPROM::getExtruderOffset(1)+EPR_EXTRUDER_Z_OFFSET,fTemp); //mm negativ
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

            if(extruder[1].id == Extruder::current->id){
                Printer::extruderOffset[Z_AXIS] = -Extruder::current->zOffset*Printer::invAxisStepsPerMM[Z_AXIS]; //+mm positiv
                if(Printer::areAxisHomed()) Printer::moveToReal(IGNORE_COORDINATE,IGNORE_COORDINATE,IGNORE_COORDINATE,IGNORE_COORDINATE,IGNORE_COORDINATE);
            }
            break;
        }
#endif // NUM_EXTRUDER>1

        case UI_ACTION_FEEDRATE_MULTIPLY:
        {
            int fr = Printer::feedrateMultiply;
            INCREMENT_MIN_MAX(fr,1,25,500);
            Commands::changeFeedrateMultiply(fr);
            break;
        }
        case UI_ACTION_FLOWRATE_MULTIPLY:
        {
            int er = Printer::extrudeMultiply;
            INCREMENT_MIN_MAX(er,1,25,200);
            Commands::changeFlowrateMultiply(static_cast<float>(er));
            break;
        }
        case UI_ACTION_STEPPER_INACTIVE:
        {
            stepperInactiveTime -= stepperInactiveTime % 1000;
            INCREMENT_MAX(stepperInactiveTime,60000UL,10080000UL);

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            HAL::eprSetInt32(EPR_STEPPER_INACTIVE_TIME,stepperInactiveTime);
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

            break;
        }
        case UI_ACTION_MAX_INACTIVE:
        {
            maxInactiveTime -= maxInactiveTime % 1000;
            INCREMENT_MAX(maxInactiveTime,60000UL,10080000UL);

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            HAL::eprSetInt32(EPR_MAX_INACTIVE_TIME,maxInactiveTime);
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

            break;
        }
        case UI_ACTION_PRINT_ACCEL_X:
        {
            INCREMENT_MIN_MAX(Printer::maxAccelerationMMPerSquareSecond[X_AXIS],100,0,10000);
            Printer::updateDerivedParameter();

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            HAL::eprSetFloat(EPR_X_MAX_ACCEL,Printer::maxAccelerationMMPerSquareSecond[X_AXIS]);
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

            break;
        }
        case UI_ACTION_PRINT_ACCEL_Y:
        {
            INCREMENT_MIN_MAX(Printer::maxAccelerationMMPerSquareSecond[Y_AXIS],100,0,10000);
            Printer::updateDerivedParameter();

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            HAL::eprSetFloat(EPR_Y_MAX_ACCEL,Printer::maxAccelerationMMPerSquareSecond[Y_AXIS]);
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

            break;
        }
        case UI_ACTION_PRINT_ACCEL_Z:
        {
            INCREMENT_MIN_MAX(Printer::maxAccelerationMMPerSquareSecond[Z_AXIS],100,0,10000);
            Printer::updateDerivedParameter();

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            HAL::eprSetFloat(EPR_Z_MAX_ACCEL,Printer::maxAccelerationMMPerSquareSecond[Z_AXIS]);
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

            break;
        }
        case UI_ACTION_MOVE_ACCEL_X:
        {
            INCREMENT_MIN_MAX(Printer::maxTravelAccelerationMMPerSquareSecond[X_AXIS],100,0,10000);
            Printer::updateDerivedParameter();

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            HAL::eprSetFloat(EPR_X_MAX_TRAVEL_ACCEL,Printer::maxTravelAccelerationMMPerSquareSecond[X_AXIS]);
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

            break;
        }
        case UI_ACTION_MOVE_ACCEL_Y:
        {
            INCREMENT_MIN_MAX(Printer::maxTravelAccelerationMMPerSquareSecond[Y_AXIS],100,0,10000);
            Printer::updateDerivedParameter();

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            HAL::eprSetFloat(EPR_Y_MAX_TRAVEL_ACCEL,Printer::maxTravelAccelerationMMPerSquareSecond[Y_AXIS]);
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

            break;
        }
        case UI_ACTION_MOVE_ACCEL_Z:
        {
            INCREMENT_MIN_MAX(Printer::maxTravelAccelerationMMPerSquareSecond[Z_AXIS],100,0,10000);
            Printer::updateDerivedParameter();

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            HAL::eprSetFloat(EPR_Z_MAX_TRAVEL_ACCEL,Printer::maxTravelAccelerationMMPerSquareSecond[Z_AXIS]);
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

            break;
        }
        case UI_ACTION_MAX_JERK:
        {
            INCREMENT_MIN_MAX(Printer::maxJerk,0.1,1,99.9);

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            HAL::eprSetFloat(EPR_MAX_JERK,Printer::maxJerk);
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

            break;
        }
        case UI_ACTION_MAX_ZJERK:
        {
            INCREMENT_MIN_MAX(Printer::maxZJerk,0.1,0.1,99.9);

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            HAL::eprSetFloat(EPR_MAX_ZJERK,Printer::maxZJerk);
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

            break;
        }
#if FEATURE_READ_CALIPER
        case UI_ACTION_CAL_STANDARD:
        {
            if(caliper_filament_standard >= 2600){
                INCREMENT_MIN_MAX(caliper_filament_standard,10,2590,3000);
                if(caliper_filament_standard == 2590){
                    caliper_filament_standard = 1750;
                }
            }else{
                INCREMENT_MIN_MAX(caliper_filament_standard,10,1600,1910);
                if(caliper_filament_standard == 1910){
                    caliper_filament_standard = 2850;
                }
            }
#if FEATURE_AUTOMATIC_EEPROM_UPDATE
             HAL::eprSetInt16( EPR_RF_CAL_STANDARD, caliper_filament_standard );
             EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
            break;
        }
        case UI_ACTION_CAL_CORRECT:
        {
            INCREMENT_MIN_MAX(caliper_collect_adjust,1,-127,127);
#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            HAL::eprSetByte( EPR_RF_CAL_ADJUST, caliper_collect_adjust );
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
            break;
        }
#endif //FEATURE_READ_CALIPER
        case UI_ACTION_MILL_ACCELERATION:
        {
#if FEATURE_MILLING_MODE
            if(!Printer::isPrinting()){
             INCREMENT_MIN_MAX(Printer::max_milling_all_axis_acceleration,2,1,200);
             Printer::updateDerivedParameter();
#if FEATURE_AUTOMATIC_EEPROM_UPDATE
             HAL::eprSetInt16(EPR_RF_MILL_ACCELERATION,Printer::max_milling_all_axis_acceleration);
             EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
            }
#endif // FEATURE_MILLING_MODE
            break;
        }
        case UI_ACTION_HOMING_FEEDRATE_X:
        {
            INCREMENT_MIN_MAX(Printer::homingFeedrate[X_AXIS],5,5,1000);

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
#if FEATURE_MILLING_MODE
            if( Printer::operatingMode == OPERATING_MODE_PRINT )
            {
                HAL::eprSetFloat(EPR_X_HOMING_FEEDRATE_PRINT,Printer::homingFeedrate[X_AXIS]);
            }
            else
            {
                HAL::eprSetFloat(EPR_X_HOMING_FEEDRATE_MILL,Printer::homingFeedrate[X_AXIS]);
            }
#else
            HAL::eprSetFloat(EPR_X_HOMING_FEEDRATE_PRINT,Printer::homingFeedrate[X_AXIS]);
#endif // FEATURE_MILLING_MODE
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

            break;
        }
        case UI_ACTION_HOMING_FEEDRATE_Y:
        {
            INCREMENT_MIN_MAX(Printer::homingFeedrate[Y_AXIS],5,5,1000);

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
#if FEATURE_MILLING_MODE
            if( Printer::operatingMode == OPERATING_MODE_PRINT )
            {
                HAL::eprSetFloat(EPR_Y_HOMING_FEEDRATE_PRINT,Printer::homingFeedrate[Y_AXIS]);
            }
            else
            {
                HAL::eprSetFloat(EPR_Y_HOMING_FEEDRATE_MILL,Printer::homingFeedrate[Y_AXIS]);
            }
#else
            HAL::eprSetFloat(EPR_Y_HOMING_FEEDRATE_PRINT,Printer::homingFeedrate[Y_AXIS]);
#endif // FEATURE_MILLING_MODE
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

            break;
        }
        case UI_ACTION_HOMING_FEEDRATE_Z:
        {
            INCREMENT_MIN_MAX(Printer::homingFeedrate[Z_AXIS],5,5,1000);

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
#if FEATURE_MILLING_MODE
            if( Printer::operatingMode == OPERATING_MODE_PRINT )
            {
                HAL::eprSetFloat(EPR_Z_HOMING_FEEDRATE_PRINT,Printer::homingFeedrate[Z_AXIS]);
            }
            else
            {
                HAL::eprSetFloat(EPR_Z_HOMING_FEEDRATE_MILL,Printer::homingFeedrate[Z_AXIS]);
            }
#else
            HAL::eprSetFloat(EPR_Z_HOMING_FEEDRATE_PRINT,Printer::homingFeedrate[Z_AXIS]);
#endif // FEATURE_MILLING_MODE
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

            break;
        }
        case UI_ACTION_MAX_FEEDRATE_X:
        {
            INCREMENT_MIN_MAX(Printer::maxFeedrate[X_AXIS],5,1,1000);

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            HAL::eprSetFloat(EPR_X_MAX_FEEDRATE,Printer::maxFeedrate[X_AXIS]);
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

            break;
        }
        case UI_ACTION_MAX_FEEDRATE_Y:
        {
            INCREMENT_MIN_MAX(Printer::maxFeedrate[Y_AXIS],5,1,1000);

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
        HAL::eprSetFloat(EPR_Y_MAX_FEEDRATE,Printer::maxFeedrate[Y_AXIS]);
        EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

            break;
        }
        case UI_ACTION_MAX_FEEDRATE_Z:
        {
            INCREMENT_MIN_MAX(Printer::maxFeedrate[Z_AXIS],5,1,1000);

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            HAL::eprSetFloat(EPR_Z_MAX_FEEDRATE,Printer::maxFeedrate[Z_AXIS]);
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

            break;
        }
        case UI_ACTION_BAUDRATE:
        {
#if EEPROM_MODE!=0
            unsigned char p=0;
            int32_t rate;
            do
            {
                rate = pgm_read_dword(&(baudrates[p]));
                if(rate==baudrate) break;
                p++;

            }while(rate!=0);

            if(rate==0 && p>=2) p-=2;
            else if(rate==0 && p==1) p=0;
            
            if(p+increment >= 0) p+=increment;
            else if(p+increment <= 0) p=0;
            
            //if(p<0) p = 0;
            rate = pgm_read_dword(&(baudrates[p]));
            if(rate==0 && p>=1) p--;
            baudrate = pgm_read_dword(&(baudrates[p]));

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            HAL::eprSetInt32(EPR_BAUDRATE,baudrate);
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
#endif // EEPROM_MODE!=0

            break;
        }

#if FEATURE_RGB_LIGHT_EFFECTS
        case UI_ACTION_RGB_LIGHT_MODE:
        {
            INCREMENT_MIN_MAX(Printer::RGBLightMode,1,0,3);

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            HAL::eprSetByte( EPR_RF_RGB_LIGHT_MODE, Printer::RGBLightMode );
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

            switch( Printer::RGBLightMode )
            {
                case RGB_MODE_OFF:
                {
                    Printer::RGBLightStatus         = RGB_STATUS_NOT_AUTOMATIC;
                    Printer::RGBLightModeForceWhite = 0;
                    setRGBTargetColors( 0, 0, 0 );
                    break;
                }
                case RGB_MODE_WHITE:
                {
                    Printer::RGBLightStatus         = RGB_STATUS_NOT_AUTOMATIC;
                    Printer::RGBLightModeForceWhite = 0;
                    setRGBTargetColors( 255, 255, 255 );
                    break;
                }
                case RGB_MODE_AUTOMATIC:
                {
                    // the firmware will determine the current RGB colors automatically
                    Printer::RGBLightStatus         = RGB_STATUS_AUTOMATIC;
                    Printer::RGBLightModeForceWhite = 0;
                    break;
                }
                case RGB_MODE_MANUAL:
                {
                    Printer::RGBLightStatus         = RGB_STATUS_NOT_AUTOMATIC;
                    Printer::RGBLightModeForceWhite = 0;
                    setRGBTargetColors( g_uRGBManualR, g_uRGBManualG, g_uRGBManualB );
                    break;
                }
            }
            break;
        }
#endif // FEATURE_RGB_LIGHT_EFFECTS
        case UI_ACTION_EXTR_STEPS_E0:
        {
           if( !Printer::isMenuMode(MENU_MODE_PAUSED) && !Printer::isPrinting() ){
            INCREMENT_MIN_MAX(extruder[0].stepsPerMM,1,1,9999);
            if(0 == Extruder::current->id) Extruder::selectExtruderById(Extruder::current->id); //übernehmen der werte

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            HAL::eprSetFloat(EEPROM::getExtruderOffset(0)+EPR_EXTRUDER_STEPS_PER_MM,extruder[0].stepsPerMM);
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
           }
           break;
        }
#if NUM_EXTRUDER > 1
        case UI_ACTION_EXTR_STEPS_E1:
        {
           if( !Printer::isMenuMode(MENU_MODE_PAUSED) && !Printer::isPrinting()){
            INCREMENT_MIN_MAX(extruder[1].stepsPerMM,1,1,9999);
            if(1 == Extruder::current->id) Extruder::selectExtruderById(Extruder::current->id); //übernehmen der werte

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            HAL::eprSetFloat(EEPROM::getExtruderOffset(1)+EPR_EXTRUDER_STEPS_PER_MM,extruder[1].stepsPerMM);
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
           }
           break;
        }
#endif //NUM_EXTRUDER > 1
#if USE_ADVANCE
        case UI_ACTION_ADVANCE_L_E0:
        {
            float step = (extruder[0].advanceL < 20.0f) ? 1.0f : ((extruder[0].advanceL < 50.0f) ? 5.0f : 10.0f);
            INCREMENT_MIN_MAX(extruder[0].advanceL,step,0.0f,250.0f); //Nibbels TODO gute Werte zulassen? Ist Step 1 ok? -> 60 war zu wenig.
             Printer::updateAdvanceFlags();
#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            HAL::eprSetFloat(EEPROM::getExtruderOffset(0)+EPR_EXTRUDER_ADVANCE_L,extruder[0].advanceL);
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
            break;
        }
#if NUM_EXTRUDER > 1
        case UI_ACTION_ADVANCE_L_E1:
        {
            float step = (extruder[1].advanceL < 20.0f) ? 1.0f : ((extruder[1].advanceL < 50.0f) ? 5.0f : 10.0f);
            INCREMENT_MIN_MAX(extruder[1].advanceL,step,0.0f,250.0f); //Nibbels TODO gute Werte zulassen? Ist Step 1 ok? -> 60 war zu wenig.
             Printer::updateAdvanceFlags();
#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            HAL::eprSetFloat(EEPROM::getExtruderOffset(1)+EPR_EXTRUDER_ADVANCE_L,extruder[1].advanceL);
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
            break;
        }
#endif //NUM_EXTRUDER > 1
#endif //USE_ADVANCE
        case UI_ACTION_EXTR_STEPS:
        {
           if( !Printer::isMenuMode(MENU_MODE_PAUSED) && !Printer::isPrinting()){
            INCREMENT_MIN_MAX(Extruder::current->stepsPerMM,1,1,9999);
            Extruder::selectExtruderById(Extruder::current->id);

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            HAL::eprSetFloat(EEPROM::getExtruderOffset(Extruder::current->id)+EPR_EXTRUDER_STEPS_PER_MM,Extruder::current->stepsPerMM);
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
           }
           break;
        }
        case UI_ACTION_EXTR_ACCELERATION:
        {
            INCREMENT_MIN_MAX(Extruder::current->maxAcceleration,10,10,99999);
            Extruder::selectExtruderById(Extruder::current->id);

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            HAL::eprSetFloat(EEPROM::getExtruderOffset(Extruder::current->id)+EPR_EXTRUDER_MAX_ACCELERATION,Extruder::current->maxAcceleration);
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

            break;
        }
        case UI_ACTION_EXTR_MAX_FEEDRATE:
        {
            INCREMENT_MIN_MAX(Extruder::current->maxFeedrate,1,1,999);
            Extruder::selectExtruderById(Extruder::current->id);

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            HAL::eprSetFloat(EEPROM::getExtruderOffset(Extruder::current->id)+EPR_EXTRUDER_MAX_FEEDRATE,Extruder::current->maxFeedrate);
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

            break;
        }
        case UI_ACTION_EXTR_START_FEEDRATE:
        {
            INCREMENT_MIN_MAX(Extruder::current->maxStartFeedrate,1,1,999);
            Extruder::selectExtruderById(Extruder::current->id);

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            HAL::eprSetFloat(EEPROM::getExtruderOffset(Extruder::current->id)+EPR_EXTRUDER_MAX_START_FEEDRATE,Extruder::current->maxStartFeedrate);
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

            break;
        }
        case UI_ACTION_EXTR_WATCH_PERIOD:
        {
            INCREMENT_MIN_MAX(Extruder::current->watchPeriod,1,0,999);

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            HAL::eprSetFloat(EEPROM::getExtruderOffset(Extruder::current->id)+EPR_EXTRUDER_WATCH_PERIOD,Extruder::current->watchPeriod);
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

            break;
        }

#if RETRACT_DURING_HEATUP
        case UI_ACTION_EXTR_WAIT_RETRACT_TEMP:
        {
            INCREMENT_MIN_MAX(Extruder::current->waitRetractTemperature,1,100,UI_SET_MAX_EXTRUDER_TEMP);

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            HAL::eprSetFloat(EEPROM::getExtruderOffset(Extruder::current->id)+EPR_EXTRUDER_WAIT_RETRACT_TEMP,Extruder::current->waitRetractTemperature);
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

            break;
        }
        case UI_ACTION_EXTR_WAIT_RETRACT_UNITS:
        {
            INCREMENT_MIN_MAX(Extruder::current->waitRetractUnits,1,0,99);

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            HAL::eprSetFloat(EEPROM::getExtruderOffset(Extruder::current->id)+EPR_EXTRUDER_WAIT_RETRACT_UNITS,Extruder::current->waitRetractUnits);
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

            break;
        }
#endif // RETRACT_DURING_HEATUP

#if FEATURE_WORK_PART_Z_COMPENSATION
        case UI_ACTION_RF_SET_Z_MATRIX_WORK_PART:
        {
            if( Printer::doWorkPartZCompensation )
            {
                // do not allow to change the current work part z-compensation matrix while the z-compensation is active
                showError( (void*)ui_text_z_compensation, (void*)ui_text_operation_denied );
                break;
            }

            INCREMENT_MIN_MAX(g_nActiveWorkPart,1,1,EEPROM_MAX_WORK_PART_SECTORS);
            switchActiveWorkPart(g_nActiveWorkPart);
            break;
        }
        case UI_ACTION_RF_SET_SCAN_DELTA_X:
        {
            INCREMENT_MIN_MAX(g_nScanXStepSizeMm,1,WORK_PART_SCAN_X_STEP_SIZE_MIN_MM,100);
            g_nScanXStepSizeSteps = (long)((float)g_nScanXStepSizeMm * Printer::axisStepsPerMM[X_AXIS]);
            break;
        }
        case UI_ACTION_RF_SET_SCAN_DELTA_Y:
        {
            INCREMENT_MIN_MAX(g_nScanYStepSizeMm,1,WORK_PART_SCAN_Y_STEP_SIZE_MIN_MM,100);
            g_nScanYStepSizeSteps = (long)((float)g_nScanYStepSizeMm * Printer::axisStepsPerMM[Y_AXIS]);
            break;
        }
#endif // FEATURE_WORK_PART_Z_COMPENSATION

#if FEATURE_HEAT_BED_Z_COMPENSATION
        case UI_ACTION_RF_SET_Z_MATRIX_HEAT_BED:
        {
            if( Printer::doHeatBedZCompensation )
            {
                // do not allow to change the current heat bed z-compensation matrix while the z-compensation is active
                showError( (void*)ui_text_z_compensation, (void*)ui_text_operation_denied );
                break;
            }

            INCREMENT_MIN_MAX(g_nActiveHeatBed,1,1,EEPROM_MAX_HEAT_BED_SECTORS);
            switchActiveHeatBed(g_nActiveHeatBed);
            break;
        }
#endif // FEATURE_HEAT_BED_Z_COMPENSATION

        case UI_ACTION_RF_RESET_ACK:
        case UI_ACTION_STOP_ACK:
        case UI_ACTION_RESTORE_DEFAULTS:
        case UI_ACTION_CHOOSE_CLASSICPID:
        case UI_ACTION_CHOOSE_LESSERINTEGRAL:
        case UI_ACTION_CHOOSE_SOME:
        case UI_ACTION_CHOOSE_NO:
        case UI_ACTION_CHOOSE_TYREUS_LYBEN:
        {
            INCREMENT_MIN_MAX(g_nYesNo,1,0,1);
            break;
        }

#if FEATURE_EMERGENCY_PAUSE
        case UI_ACTION_EMERGENCY_PAUSE_MIN:
        {
            INCREMENT_MIN_MAX(g_nEmergencyPauseDigitsMin,200,EMERGENCY_PAUSE_DIGITS_MIN,g_nEmergencyPauseDigitsMax);
#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            HAL::eprSetInt32( EPR_RF_EMERGENCYPAUSEDIGITSMIN, g_nEmergencyPauseDigitsMin );
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
            break;
        }
        case UI_ACTION_EMERGENCY_PAUSE_MAX:
        {
            INCREMENT_MIN_MAX(g_nEmergencyPauseDigitsMax,200,g_nEmergencyPauseDigitsMin,EMERGENCY_PAUSE_DIGITS_MAX);
#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            HAL::eprSetInt32( EPR_RF_EMERGENCYPAUSEDIGITSMAX, g_nEmergencyPauseDigitsMax );
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
            break;
        }
#endif //FEATURE_EMERGENCY_PAUSE

#if FEATURE_EMERGENCY_STOP_ALL
        case UI_ACTION_EMERGENCY_ZSTOP_MIN:
        {
            INCREMENT_MIN_MAX(g_nZEmergencyStopAllMin,200,EMERGENCY_STOP_DIGITS_MIN,g_nZEmergencyStopAllMax);
#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            HAL::eprSetInt16( EPR_RF_EMERGENCYZSTOPDIGITSMIN, g_nZEmergencyStopAllMin );
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
            break;
        }
        case UI_ACTION_EMERGENCY_ZSTOP_MAX:
        {
            INCREMENT_MIN_MAX(g_nZEmergencyStopAllMax,200,g_nZEmergencyStopAllMin,EMERGENCY_STOP_DIGITS_MAX);
#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            HAL::eprSetInt16( EPR_RF_EMERGENCYZSTOPDIGITSMAX, g_nZEmergencyStopAllMax );
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
            break;
        }
#endif //FEATURE_EMERGENCY_STOP_ALL

        case UI_ACTION_CHOOSE_DMIN:
        {
            if(uid.menuLevel == 4){ //identifikation des temperaturzyklus anhand der position im menü. Das ist nicht 100% sauber, aber funktioniert.
                uint8_t heater = uid.menuPos[uid.menuLevel-1]; //0..1..2 mit zwei extrudern und bett. passt zum autotunesystem, weil UI_MENU_PID_EXT0_COUNT + UI_MENU_PID_EXT1_COUNT + UI_MENU_PID_BED_COUNT
                if(heater < NUM_TEMPERATURE_LOOPS) {
                    int drive = tempController[heater]->pidDriveMin;
                    INCREMENT_MIN_MAX(drive,1,0,255);
                    tempController[heater]->pidDriveMin = drive;
#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                    if(UI_MENU_PID_BED_COUNT > 0 && UI_MENU_PID_EXT0_COUNT + UI_MENU_PID_EXT1_COUNT + UI_MENU_PID_BED_COUNT - 1 == heater){ 
                        //Das ist das Heizbett
                        HAL::eprSetByte( EPR_BED_DRIVE_MIN, (uint8_t)drive  );
                        EEPROM::updateChecksum();
                    }else{
                        //Extruder
                        if(heater <= NUM_EXTRUDER-1){ //paranoid doublecheck
                          HAL::eprSetByte( EEPROM::getExtruderOffset(heater)+EPR_EXTRUDER_DRIVE_MIN, (uint8_t)drive  );
                          EEPROM::updateChecksum();
                        }
                    }
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
                }
            }
            break;
        }
        case UI_ACTION_CHOOSE_DMAX:
        {
            if(uid.menuLevel == 4){ //identifikation des temperaturzyklus anhand der position im menü. Das ist nicht 100% sauber, aber funktioniert.
                uint8_t heater = uid.menuPos[uid.menuLevel-1]; //0..1..2 mit zwei extrudern und bett. passt zum autotunesystem, weil UI_MENU_PID_EXT0_COUNT + UI_MENU_PID_EXT1_COUNT + UI_MENU_PID_BED_COUNT
                if(heater < NUM_TEMPERATURE_LOOPS) {
                    int drive = tempController[heater]->pidDriveMax;
                    INCREMENT_MIN_MAX(drive,1,0,255);
                    tempController[heater]->pidDriveMax = drive;
#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                    if(UI_MENU_PID_BED_COUNT > 0 && UI_MENU_PID_EXT0_COUNT + UI_MENU_PID_EXT1_COUNT + UI_MENU_PID_BED_COUNT - 1 == heater){ 
                        //Das ist das Heizbett
                        HAL::eprSetByte( EPR_BED_DRIVE_MAX, (uint8_t)drive  );
                        EEPROM::updateChecksum();
                    }else{
                        //Extruder
                        if(heater <= NUM_EXTRUDER-1){ //paranoid doublecheck
                          HAL::eprSetByte( EEPROM::getExtruderOffset(heater)+EPR_EXTRUDER_DRIVE_MAX, (uint8_t)drive  );
                          EEPROM::updateChecksum();
                        }
                    }
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
                }
            }
            break;
        }
        case UI_ACTION_CHOOSE_PIDMAX:
        {
            if(uid.menuLevel == 4){ //identifikation des temperaturzyklus anhand der position im menü. Das ist nicht 100% sauber, aber funktioniert.
                uint8_t heater = uid.menuPos[uid.menuLevel-1]; //0..1..2 mit zwei extrudern und bett. passt zum autotunesystem, weil UI_MENU_PID_EXT0_COUNT + UI_MENU_PID_EXT1_COUNT + UI_MENU_PID_BED_COUNT
                if(heater < NUM_TEMPERATURE_LOOPS) {
                    int drive = tempController[heater]->pidMax;
                    INCREMENT_MIN_MAX(drive,1,0,255);
                    tempController[heater]->pidMax = drive;
#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                    if(UI_MENU_PID_BED_COUNT > 0 && UI_MENU_PID_EXT0_COUNT + UI_MENU_PID_EXT1_COUNT + UI_MENU_PID_BED_COUNT - 1 == heater){ 
                        //Das ist das Heizbett
                        HAL::eprSetByte( EPR_BED_PID_MAX, (uint8_t)drive  );
                        EEPROM::updateChecksum();
                    }else{
                        //Extruder
                        if(heater <= NUM_EXTRUDER-1){ //paranoid doublecheck
                          HAL::eprSetByte( EEPROM::getExtruderOffset(heater)+EPR_EXTRUDER_PID_MAX, (uint8_t)drive  );
                          EEPROM::updateChecksum();
                        }
                    }
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
                }
            }
            break;
        }
        case UI_ACTION_CHOOSE_SENSOR:
        {
            if(uid.menuLevel == 4){ //identifikation des temperaturzyklus anhand der position im menü. Das ist nicht 100% sauber, aber funktioniert.
                uint8_t heater = uid.menuPos[uid.menuLevel-1]; //0..1..2 mit zwei extrudern und bett. passt zum autotunesystem, weil UI_MENU_PID_EXT0_COUNT + UI_MENU_PID_EXT1_COUNT + UI_MENU_PID_BED_COUNT
                if(heater < NUM_TEMPERATURE_LOOPS) {
                    int drive = tempController[heater]->sensorType;
#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                    if(UI_MENU_PID_BED_COUNT > 0 && UI_MENU_PID_EXT0_COUNT + UI_MENU_PID_EXT1_COUNT + UI_MENU_PID_BED_COUNT - 1 == heater){ 
                        //Das ist das Heizbett
                        switch(drive){
                          case 3: { drive = 4; break; } //4 ist der 10K Thermistor vom hersteller der matten (? rf1k_mhj11)
                          default: { drive = 3; break; }
                        }
                        tempController[heater]->sensorType = drive;
                        HAL::eprSetByte( EPR_RF_HEATED_BED_SENSOR_TYPE, (uint8_t)drive  );
                        EEPROM::updateChecksum();
                    }else{
                        //Extruder
                        if(increment > 0){
                            switch(drive){
                              case 3: { drive = 8; break; }
                              case 8: { drive = 13; break; } //add more sensors for menu-tweaking here, those are the most common for RFx000
                              case 13: { drive = 1; break; }
                              default: { drive = 3; break; }
                            }
                        }else{ //== 0 gibts nicht, soweit ich weiß
                            switch(drive){
                              case 3: { drive = 1; break; }
                              case 1: { drive = 13; break; }
                              case 13: { drive = 8; break; } //add more sensors for menu-tweaking here, those are the most common for RFx000
                              default: { drive = 3; break; }
                            }
                        }
                        tempController[heater]->sensorType = drive;
                        if(heater <= NUM_EXTRUDER-1){ //paranoid doublecheck
                          HAL::eprSetByte( EEPROM::getExtruderOffset(heater)+EPR_EXTRUDER_SENSOR_TYPE, (uint8_t)drive  );
                          EEPROM::updateChecksum();
                        }
                    }
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
                }
            }
            break;
        }
        case UI_ACTION_CHOOSE_MOTOR_X:
        case UI_ACTION_CHOOSE_MOTOR_Y:
        case UI_ACTION_CHOOSE_MOTOR_Z:
        case UI_ACTION_CHOOSE_MOTOR_E0:
        case UI_ACTION_CHOOSE_MOTOR_E1:
        {
            uint8_t steppernr = uid.menuPos[uid.menuLevel];
            if(steppernr == 6) steppernr = 4; //das ist etwas stümperhaft, aber ich brauche die Zeilennummer um auszuwählen und die Einstellungen für die Extruderstepper gehören drüber, also muss Extruder 2 zwei Zeilen runter...
            if(steppernr < 5) { // aktuell gibts nur 5
                int drive = Printer::motorCurrent[steppernr];
                const short uMotorCurrentMax[] = MOTOR_CURRENT_MAX;
                INCREMENT_MIN_MAX(drive,1,MOTOR_CURRENT_MIN+1,uMotorCurrentMax[steppernr]); //von 40 bis maximal das was in der config steht.
                Printer::motorCurrent[steppernr] = drive;
#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                HAL::eprSetByte( EPR_RF_MOTOR_CURRENT+steppernr, (uint8_t)drive  );
                EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
                setMotorCurrent( steppernr+1, drive );
                Com::printF( PSTR( "Stepper" ), steppernr+1 );
                Com::printFLN( PSTR( " = " ), drive );
            }
            break;
        }
#if FEATURE_DIGIT_FLOW_COMPENSATION
        case UI_ACTION_FLOW_MIN:
        {
            INCREMENT_MIN_MAX(g_nDigitFlowCompensation_Fmin,200,0,g_nDigitFlowCompensation_Fmax);
            break;
        }
        case UI_ACTION_FLOW_MAX:
        {
            short max = (g_nEmergencyPauseDigitsMax ? abs(g_nEmergencyPauseDigitsMax) : abs(EMERGENCY_PAUSE_DIGITS_MAX));
            INCREMENT_MIN_MAX(g_nDigitFlowCompensation_Fmax,200,g_nDigitFlowCompensation_Fmin,max);
            break;
        }
        case UI_ACTION_FLOW_DF:
        {
            INCREMENT_MIN_MAX(g_nDigitFlowCompensation_intense,1,-99,99);
            //flowmulti wird zur laufzeit geändert, anhand von steigung intense -> aber je nach cache erst sehr spät.
            break;
        }
        case UI_ACTION_FLOW_DV:
        {
            INCREMENT_MIN_MAX(g_nDigitFlowCompensation_speed_intense,1,-90,99);
            //feedmulti wird zur laufzeit geändert, anhand von steigung intense
            break;
        }
#endif //FEATURE_DIGIT_FLOW_COMPENSATION
        case UI_ACTION_FREQ_DBL:
        {
            INCREMENT_MIN_MAX(Printer::stepsDoublerFrequency,500,5000,12000);
#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            HAL::eprSetInt16( EPR_RF_FREQ_DBL, Printer::stepsDoublerFrequency  );
            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
            break;
        }
    }

#if FEATURE_MILLING_MODE
    if( Printer::operatingMode == OPERATING_MODE_PRINT )
    {
#if UI_PRINT_AUTORETURN_TO_MENU_AFTER!=0
        g_nAutoReturnTime=HAL::timeInMilliseconds()+UI_PRINT_AUTORETURN_TO_MENU_AFTER;
#endif // UI_PRINT_AUTORETURN_TO_MENU_AFTER!=0
    }
    else
    {
#if UI_MILL_AUTORETURN_TO_MENU_AFTER!=0
        g_nAutoReturnTime=HAL::timeInMilliseconds()+UI_MILL_AUTORETURN_TO_MENU_AFTER;
#endif // UI_MILL_AUTORETURN_TO_MENU_AFTER!=0
    }
#else
#if UI_PRINT_AUTORETURN_TO_MENU_AFTER!=0
    g_nAutoReturnTime=HAL::timeInMilliseconds()+UI_PRINT_AUTORETURN_TO_MENU_AFTER;
#endif // UI_PRINT_AUTORETURN_TO_MENU_AFTER!=0
#endif // FEATURE_MILLING_MODE
#endif // UI_HAS_KEYS==1

} // nextPreviousAction


void UIDisplay::finishAction(int action)
{
    switch( action )
    {
#if FEATURE_RESET_VIA_MENU
        case UI_ACTION_RF_RESET_ACK:
        {
            if( g_nYesNo != 1 )
            {
                // continue only in case the user has chosen "Yes"
                break;
            }

            if( Printer::debugInfo() )
            {
                Com::printFLN( PSTR( "processButton(): restart" ) );
            }
            HAL::delayMilliseconds( 100 );
            Commands::emergencyStop();
            break;
        }
#endif // FEATURE_RESET_VIA_MENU

        case UI_ACTION_STOP_ACK:
        {
            if( g_nYesNo != 1 )
            {
                // continue only in case the user has chosen "Yes"
                break;
            }
            uid.executeAction(UI_ACTION_TOP_MENU);
            Printer::stopPrint();
            break;
        }

        case UI_ACTION_RESTORE_DEFAULTS:
            {
            if( g_nYesNo != 1 )
            {
                   // continue only in case the user has chosen "Yes"
                   break;
            }
            EEPROM::restoreEEPROMSettingsFromConfiguration();

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
            EEPROM::storeDataIntoEEPROM(false);
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

            EEPROM::initializeAllOperatingModes();
            uid.menuLevel = 0;
            UI_STATUS( UI_TEXT_RESTORE_DEFAULTS );
            break;
        }

        case UI_ACTION_CHOOSE_CLASSICPID:
        case UI_ACTION_CHOOSE_LESSERINTEGRAL:
        case UI_ACTION_CHOOSE_SOME:
        case UI_ACTION_CHOOSE_NO:
        case UI_ACTION_CHOOSE_TYREUS_LYBEN:
        {
            if( g_nYesNo != 1 )
            {
                // continue only in case the user has chosen "Yes"
                break;
            }
            unsigned char heater = uid.menuPos[uid.menuLevel-2]; //0..1..2 mit zwei extrudern und bett. passt zum autotunesystem, weil UI_MENU_PID_EXT0_COUNT + UI_MENU_PID_EXT1_COUNT + UI_MENU_PID_BED_COUNT
            int method = uid.menuPos[uid.menuLevel-1]; //0..1..2..3..4..5 passt zum J-Listing des M303
            /*
             Line 1059: #define PRECISE_HEAT_BED_SCAN_BED_TEMP_PLA          60                                                                  // [°C]
             Line 1062: #define PRECISE_HEAT_BED_SCAN_EXTRUDER_TEMP_PLA     230                                                                 // [°C]
            */
#if NUM_TEMPERATURE_LOOPS > 0
            int temperature = PRECISE_HEAT_BED_SCAN_EXTRUDER_TEMP_PLA;
            int cycles = 10;
            if(UI_MENU_PID_EXT0_COUNT + UI_MENU_PID_EXT1_COUNT + UI_MENU_PID_BED_COUNT - 1 == heater && UI_MENU_PID_BED_COUNT > 0){ 
                //Das ist das Heizbett und kein Extruder
                temperature = PRECISE_HEAT_BED_SCAN_BED_TEMP_PLA; //Bett nicht so hoch testen, wie Extruder.
                cycles = 16; //Bett ist erfahrungsgemäß viel träger. Mehr Zyklen erhöhen die Genauigkeit des Ergebnis.
            }
            bool writeeeprom = true;
            if(heater >= NUM_TEMPERATURE_LOOPS) heater = NUM_TEMPERATURE_LOOPS -1;
            //show menu and message to user: He cant do anything until autotune is over.
            uid.menuLevel = 0; 
            uid.menuPos[0] = 3; //show temps
            g_uStartOfIdle = 0;
            UI_STATUS_UPD(UI_TEXT_PID);
            tempController[heater]->autotunePID(temperature,heater,cycles,writeeeprom, method);  
#else
            Com::printFLN( PSTR( "PID Autotune Error: Noo Temperature-Loops defined!??" ) );
#endif // NUM_TEMPERATURE_LOOPS > 0
            break;
        }

    }

} // finishAction


// Actions are events from user input. Depending on the current state, each
// action can behave differently. Other actions do always the same like home, disable extruder etc.
void UIDisplay::executeAction(int action)
{
    if( Printer::blockAll )
    {
        if( action == UI_ACTION_OK || action == UI_ACTION_RF_CONTINUE ){
            Com::printFLN( PSTR( "Restart after Emergency-Stop" ) );
            HAL::delayMilliseconds( 100 );
            Commands::emergencyStop();
        }
        // do not allow any user inputs when we have been blocked
        return;
    }

#if UI_HAS_KEYS==1
    bool skipBeep = false;
    if(action & UI_ACTION_TOPMENU)   // Go to start menu
    {
        action -= UI_ACTION_TOPMENU;
        menuLevel = 0;
        if(uid.menuPos[0] == 1) uid.menuPos[0] = 0;
    }
    else if((action>=UI_ACTION_RF_MIN_REPEATABLE && action<=UI_ACTION_RF_MAX_REPEATABLE) ||
            (action>=UI_ACTION_RF_MIN_SINGLE && action<=UI_ACTION_RF_MAX_SINGLE))
    {
        processButton( action );
    }
    else
    {
        switch(action)
        {
            case UI_ACTION_OK:
            {
                okAction();
                skipBeep=true; // Prevent double beep
                g_nYesNo = 0;
                break;
            }
            case UI_ACTION_BACK:
            {
#if FEATURE_SENSIBLE_PRESSURE
                if( uid.menuLevel == 0 && uid.menuPos[0] == 1 ){ //wenn im Mod-Menü für Z-Offset/Matrix Sense-Offset/Limiter, dann anders!
                    //we are in the Mod menu
                    //verkleinern des Digit-Limits
                    if(g_nSensiblePressureDigits == 0){
                        if(EMERGENCY_PAUSE_DIGITS_MAX * 0.8 < 32767){
                            g_nSensiblePressureDigits = EMERGENCY_PAUSE_DIGITS_MAX * 0.8; //maximalstellung, das ist aber schon irre hoch. abhängig von messdose und maximaltragkraft vs. genauigkeit der zelle.
                        }else{
                            g_nSensiblePressureDigits = 32767; //die variable ist short, nie mehr rein als nötig ^^.
                        }
                    }else if(g_nSensiblePressureDigits < 50){ // ex 250
                        g_nSensiblePressureDigits = 0;
                    }else{
                        //TPE braucht mini werte, wenn es sinnvoll sein soll. Darum der Ternary, sodass man per Knopf auch kleinste Zahlen justieren kann.
                        //g_nSensiblePressureDigits -= 250; //decrement pro Knopfklick. Man kann ja auf der Taste bleiben.
                        g_nSensiblePressureDigits -= (g_nSensiblePressureDigits >= 2000) ? 250 : (g_nSensiblePressureDigits >= 500) ? 100 : 50 ; //decrement pro Knopfklick. Man kann ja auf der Taste bleiben.
#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                        //Wir speichern nur Werte automatisch per Knopf, die im Alltag sinn machen können. Ab 500:
                        short oldval = HAL::eprGetInt16(EPR_RF_MOD_SENSEOFFSET_DIGITS);
                        if(g_nSensiblePressureDigits >= 500 && oldval != g_nSensiblePressureDigits){
                            HAL::eprSetInt16( EPR_RF_MOD_SENSEOFFSET_DIGITS, g_nSensiblePressureDigits );
                            EEPROM::updateChecksum(); //deshalb die prüfung
                        }
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
                    }
                    beep(1,4);
                    //skipBeep=true;
                }else{
#endif

#if FEATURE_RGB_LIGHT_EFFECTS
                    if ( menuLevel == 0 )
                    {
                        Printer::RGBButtonBackPressed = 1;
                    }
#endif // FEATURE_RGB_LIGHT_EFFECTS
                    if ( menuLevel == 1 && uid.menuPos[0] == 1 ){
                        //der würde in das modmenü zurückgehen, da sind aber die rechtst links tasten anders belegt, daher nicht da rein! sonst evtl. verstellen von DigitLimit.
                        uid.menuPos[0] = 0;
                    }
                    if(menuLevel>0) menuLevel--;
                    Printer::setAutomount(false);
                    activeAction = 0;
                    g_nYesNo = 0;
                
#if FEATURE_SENSIBLE_PRESSURE
                }
#endif
                break;
            }
            case UI_ACTION_NEXT:
            {
                nextPreviousAction(1);
                break;
            }
            case UI_ACTION_RIGHT:
            {
                rightAction();
                break;
            }
            case UI_ACTION_PREVIOUS:
            {
                nextPreviousAction(-1);
                break;
            }
            case UI_ACTION_MENU_UP:
            {
                if(menuLevel>0) menuLevel--;
                break;
            }
            case UI_ACTION_TOP_MENU:
            {
                menuLevel = 0;
                if(uid.menuPos[0] == 1) uid.menuPos[0] = 0;
                break;
            }
            case UI_ACTION_EMERGENCY_STOP:
            {
                Commands::emergencyStop();
                break;
            }
            case UI_ACTION_HOME_ALL:
            {
                if( PrintLine::linesCount )
                {
                    // do not allow homing via the menu while we are printing
                    if( Printer::debugErrors() )
                    {
                        Com::printFLN( Com::tPrintingIsInProcessError );
                    }

                    showError( (void*)ui_text_home, (void*)ui_text_operation_denied );
                    break;
                }
                if( !isHomingAllowed( NULL, 1 ) )
                {
                    break;
                }

#if FEATURE_CONFIGURABLE_Z_ENDSTOPS
                if( Printer::ZEndstopUnknown )
                {
                    // in case the z-endstop is unknown, we home only in z-direction //Nibbels 08.02.2018 warum??
                    Printer::homeAxis(false,false,true);
                }
                else
                {
                    Printer::homeAxis(true,true,true);
                }
#else
                Printer::homeAxis(true,true,true);
#endif // FEATURE_CONFIGURABLE_Z_ENDSTOPS

                Commands::printCurrentPosition();
                break;
            }
            case UI_ACTION_HOME_X:
            {
                if( PrintLine::linesCount )
                {
                    // do not allow homing via the menu while we are printing
                    if( Printer::debugErrors() )
                    {
                        Com::printFLN( Com::tPrintingIsInProcessError );
                    }

                    showError( (void*)ui_text_home, (void*)ui_text_operation_denied );
                    break;
                }
                if( !isHomingAllowed( NULL, 1 ) )
                {
                    break;
                }

                Printer::homeAxis(true,false,false);
                Commands::printCurrentPosition();
                break;
            }
            case UI_ACTION_HOME_Y:
            {
                if( PrintLine::linesCount )
                {
                    // do not allow homing via the menu while we are printing
                    if( Printer::debugErrors() )
                    {
                        Com::printFLN( Com::tPrintingIsInProcessError );
                    }

                    showError( (void*)ui_text_home, (void*)ui_text_operation_denied );
                    break;
                }
                if( !isHomingAllowed( NULL, 1 ) )
                {
                    break;
                }

                Printer::homeAxis(false,true,false);
                Commands::printCurrentPosition();
                break;
            }
            case UI_ACTION_HOME_Z:
            {
                if( PrintLine::linesCount )
                {
                    // do not allow homing via the menu while we are printing
                    if( Printer::debugErrors() )
                    {
                        Com::printFLN( Com::tPrintingIsInProcessError );
                    }

                    showError( (void*)ui_text_home, (void*)ui_text_operation_denied );
                    break;
                }
                if( !isHomingAllowed( NULL, 1 ) )
                {
                    break;
                }

                Printer::homeAxis(false,false,true);
                Commands::printCurrentPosition();
                break;
            }
            case UI_ACTION_SET_XY_ORIGIN:
            {
                Printer::setOrigin(-Printer::queuePositionLastMM[X_AXIS],-Printer::queuePositionLastMM[Y_AXIS],Printer::originOffsetMM[Z_AXIS]);
                BEEP_ACCEPT_SET_POSITION
                break;
            }
            case UI_ACTION_DEBUG_ECHO:
            {
                if(Printer::debugEcho()) Printer::debugLevel-=1;
                else Printer::debugLevel+=1;
                break;
            }
            case UI_ACTION_DEBUG_INFO:
            {
                if(Printer::debugInfo()) Printer::debugLevel-=2;
                else Printer::debugLevel+=2;
                break;
            }
            case UI_ACTION_DEBUG_ERROR:
            {
                if(Printer::debugErrors()) Printer::debugLevel-=4;
                else Printer::debugLevel+=4;
                break;
            }
            case UI_ACTION_DEBUG_DRYRUN:
            {
                if(Printer::debugDryrun()) Printer::debugLevel-=8;
                else Printer::debugLevel+=8;
                if(Printer::debugDryrun())   // simulate movements without printing
                {
                    Extruder::setTemperatureForAllExtruders(0, false);
#if HAVE_HEATED_BED==true
                    Extruder::setHeatedBedTemperature(0);
#endif // HAVE_HEATED_BED==true
                }
                break;
            }
            case UI_ACTION_POWER:
            {
#if PS_ON_PIN>=0 // avoid compiler errors when the power supply pin is disabled
                Commands::waitUntilEndOfAllMoves();
                SET_OUTPUT(PS_ON_PIN); //GND
                TOGGLE(PS_ON_PIN);
#endif // PS_ON_PIN>=0

                break;
            }

#if FEATURE_CASE_LIGHT
            case UI_ACTION_LIGHTS_ONOFF:
            {
                if( Printer::enableCaseLight )  Printer::enableCaseLight = 0;
                else                            Printer::enableCaseLight = 1;
                WRITE(CASE_LIGHT_PIN, Printer::enableCaseLight);

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                HAL::eprSetByte( EPR_RF_CASE_LIGHT_MODE, Printer::enableCaseLight );
                EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

                break;
            }
#endif // FEATURE_CASE_LIGHT

#if FEATURE_230V_OUTPUT
            case UI_ACTION_230V_OUTPUT:
            {
                if( Printer::enable230VOutput ) Printer::enable230VOutput = 0;
                else                            Printer::enable230VOutput = 1;
                WRITE(OUTPUT_230V_PIN, Printer::enable230VOutput);

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                // after a power-on, the 230 V plug always shall be turned off - thus, we do not store this setting to the EEPROM
                // HAL::eprSetByte( EPR_RF_230V_OUTPUT_MODE, Printer::enable230VOutput );
                // EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

                break;
            }
#endif // FEATURE_230V_OUTPUT

#if FEATURE_24V_FET_OUTPUTS
            case UI_ACTION_FET1_OUTPUT:
            {
                if( Printer::enableFET1 )   Printer::enableFET1 = 0;
                else                        Printer::enableFET1 = 1;
                WRITE(FET1, Printer::enableFET1);

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                HAL::eprSetByte( EPR_RF_FET1_MODE , Printer::enableFET1 );
                EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
                break;
            }
            
            case UI_ACTION_FET2_OUTPUT:
            {
                if( Printer::enableFET2 )   Printer::enableFET2 = 0;
                else                        Printer::enableFET2 = 1;
                WRITE(FET2, Printer::enableFET2);

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                HAL::eprSetByte( EPR_RF_FET2_MODE , Printer::enableFET2 );
                EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
                break;
            }
#endif // FEATURE_24V_FET_OUTPUTS


#if FEATURE_EXTENDED_BUTTONS
            case UI_ACTION_CONFIG_SINGLE_STEPS:
            {   
                configureMANUAL_STEPS_Z( 1 );
                break;
            }
#endif // FEATURE_EXTENDED_BUTTONS

#if FEATURE_MILLING_MODE
            case UI_ACTION_OPERATING_MODE:
            {
                char    deny = 0;


                if( PrintLine::linesCount )     deny = 1;   // the operating mode can not be switched while the printing is in progress

#if FEATURE_HEAT_BED_Z_COMPENSATION
                if( g_nHeatBedScanStatus || g_nZOSScanStatus )       deny = 1;   // the operating mode can not be switched while a heat bed scan / ZOS is in progress
#endif // FEATURE_HEAT_BED_Z_COMPENSATION

#if FEATURE_WORK_PART_Z_COMPENSATION
                if( g_nWorkPartScanStatus )     deny = 1;   // the operating mode can not be switched while a work part scan is in progress
#endif // FEATURE_WORK_PART_Z_COMPENSATION

#if FEATURE_FIND_Z_ORIGIN
                if( g_nFindZOriginStatus )      deny = 1;   // the operating mode can not be switched while the z-origin is searched
#endif // FEATURE_FIND_Z_ORIGIN

                if( deny )
                {
                    if( Printer::debugErrors() )
                    {
                        Com::printFLN( Com::tPrintingIsInProcessError );
                    }

                    showError( (void*)ui_text_change_mode, (void*)ui_text_operation_denied );
                    break;
                }

                // disable and turn off everything before we switch the operating mode
                Printer::kill( false );

                if( Printer::operatingMode == OPERATING_MODE_PRINT )
                {
                    switchOperatingMode( OPERATING_MODE_MILL );
                }
                else
                {
                    switchOperatingMode( OPERATING_MODE_PRINT );
                }

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                HAL::eprSetByte( EPR_RF_OPERATING_MODE, Printer::operatingMode );
                EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

                 break;
            }
#endif // FEATURE_MILLING_MODE

#if FEATURE_CONFIGURABLE_Z_ENDSTOPS
            case UI_ACTION_Z_ENDSTOP_TYPE:
            {
                if( PrintLine::linesCount ) 
                {
                    // the z-endstop type can not be switched while the printing is in progress
                    if( Printer::debugErrors() )
                    {
                        Com::printFLN( Com::tPrintingIsInProcessError );
                    }

                    showError( (void*)ui_text_change_z_type, (void*)ui_text_operation_denied );
                    break;
                }

                Printer::lastZDirection = 0;
                Printer::endstopZMinHit = ENDSTOP_NOT_HIT;
                Printer::endstopZMaxHit = ENDSTOP_NOT_HIT;

                if( Printer::ZEndstopType == ENDSTOP_TYPE_SINGLE )
                {
                    Printer::ZEndstopType = ENDSTOP_TYPE_CIRCUIT;

                    if( Printer::isZMinEndstopHit() || Printer::isZMaxEndstopHit() )
                    {
                        // a z-endstop is active at the moment, but both z-endstops are within one circuit so we do not know which one is the pressed one
                        // in this situation we do not allow any moving into z-direction before a z-homing has been performed
                        Printer::ZEndstopUnknown = 1;
                    }
                }
                else
                {
                    Printer::ZEndstopType    = ENDSTOP_TYPE_SINGLE;
                    Printer::ZEndstopUnknown = 0;
                }

                // update our information about the currently active Z-endstop
                Printer::isZMinEndstopHit();
                Printer::isZMaxEndstopHit();

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                HAL::eprSetByte( EPR_RF_Z_ENDSTOP_TYPE, Printer::ZEndstopType );
                EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
                 break;
            }
#endif // FEATURE_CONFIGURABLE_Z_ENDSTOPS

            case UI_ACTION_ZMODE:
            {
                if( Printer::ZMode == 1 )        Printer::ZMode = 2;
                else if( Printer::ZMode == 2 )   Printer::ZMode = 3;
                else                             Printer::ZMode = 1;
#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                HAL::eprSetByte( EPR_RF_Z_MODE, Printer::ZMode );
                EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
                break;
            }

#if FEATURE_CONFIGURABLE_MILLER_TYPE
            case UI_ACTION_MILLER_TYPE:
            {
                if( PrintLine::linesCount ) 
                {
                    // the hotend type can not be switched while the printing is in progress
                    if( Printer::debugErrors() )
                    {
                        Com::printFLN( Com::tPrintingIsInProcessError );
                    }

                    showError( (void*)ui_text_change_miller_type, (void*)ui_text_operation_denied );
                    break;
                }

                if( Printer::MillerType == MILLER_TYPE_ONE_TRACK )
                {
                    Printer::MillerType          = MILLER_TYPE_TWO_TRACKS;
                    g_nScanContactPressureDelta  = MT2_WORK_PART_SCAN_CONTACT_PRESSURE_DELTA;
                    g_nScanRetryPressureDelta    = MT2_WORK_PART_SCAN_RETRY_PRESSURE_DELTA;
                }
                else
                {
                    Printer::MillerType          = MILLER_TYPE_ONE_TRACK;
                    g_nScanContactPressureDelta  = MT1_WORK_PART_SCAN_CONTACT_PRESSURE_DELTA;
                    g_nScanRetryPressureDelta    = MT1_WORK_PART_SCAN_RETRY_PRESSURE_DELTA;
                }

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                HAL::eprSetByte( EPR_RF_MILLER_TYPE, Printer::MillerType );
                EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
                 break;
            }
#endif // FEATURE_CONFIGURABLE_MILLER_TYPE

            case UI_ACTION_PREHEAT_PLA:
            {
                g_uStartOfIdle = 0;
                UI_STATUS_UPD( UI_TEXT_PREHEAT_PLA );
                Extruder::setTemperatureForAllExtruders(UI_SET_PRESET_EXTRUDER_TEMP_PLA, false);
#if HAVE_HEATED_BED==true
                Extruder::setHeatedBedTemperature(UI_SET_PRESET_HEATED_BED_TEMP_PLA);
#endif // HAVE_HEATED_BED==true
            
                break;
            }
            case UI_ACTION_PREHEAT_ABS:
            {
                g_uStartOfIdle = 0;
                UI_STATUS_UPD( UI_TEXT_PREHEAT_ABS );
                Extruder::setTemperatureForAllExtruders(UI_SET_PRESET_EXTRUDER_TEMP_ABS, false);
#if HAVE_HEATED_BED==true
                Extruder::setHeatedBedTemperature(UI_SET_PRESET_HEATED_BED_TEMP_ABS);
#endif // HAVE_HEATED_BED==true

                break;
            }
            case UI_ACTION_COOLDOWN:
            {
                UI_STATUS_UPD( UI_TEXT_COOLDOWN );                
                Extruder::setTemperatureForAllExtruders(0, false);
#if HAVE_HEATED_BED==true
                Extruder::setHeatedBedTemperature(0);
#endif // HAVE_HEATED_BED==true
            
                break;
            }
            case UI_ACTION_HEATED_BED_OFF:
            {
#if HAVE_HEATED_BED==true
                Extruder::setHeatedBedTemperature(0);
#endif // HAVE_HEATED_BED==true
                break;
            }
            case UI_ACTION_EXTRUDER0_OFF:
            {
                Extruder::setTemperatureForExtruder(0,0);
                break;
            }
            case UI_ACTION_EXTRUDER1_OFF:
            {
#if NUM_EXTRUDER>1
                Extruder::setTemperatureForExtruder(0,1);
#endif // NUM_EXTRUDER>1
                break;
            }
            case UI_ACTION_EXTRUDER2_OFF:
            {
#if NUM_EXTRUDER>2
                Extruder::setTemperatureForExtruder(0,2);
#endif // NUM_EXTRUDER>2
                break;
            }
            case UI_ACTION_DISABLE_STEPPER:
            {
                Printer::kill( true );
                break;
            }
            case UI_ACTION_UNMOUNT_FILAMENT:
            {
                if( Extruder::current->tempControl.targetTemperatureC == 0 )
                {
                    char    unlock = !uid.locked;

                    g_uStartOfIdle = 0;
                    uid.executeAction(UI_ACTION_TOP_MENU);
                    UI_STATUS_UPD( UI_TEXT_UNMOUNT_FILAMENT );
                    uid.lock();

                    GCode::executeFString(Com::tUnmountFilamentWithHeating);

                    if( unlock )
                    {
                        uid.unlock();
                    }
                    g_uStartOfIdle = HAL::timeInMilliseconds();
                }
                else
                {
#if !EXTRUDER_ALLOW_COLD_MOVE
                    if( Extruder::current->tempControl.currentTemperatureC < UI_SET_EXTRUDER_TEMP_UNMOUNT )
                    {
                        // we do not allow to move the extruder in case it is not heated up enough
                        if( Printer::debugErrors() )
                        {
                            Com::printFLN( PSTR( "Unload Filament: extruder output: aborted" ) );
                        }

                        showError( (void*)ui_text_extruder, (void*)ui_text_operation_denied );
                        break;
                    }
#endif // !EXTRUDER_ALLOW_COLD_MOVE

                    char    unlock = !uid.locked;

                    g_uStartOfIdle = 0;
                    uid.executeAction(UI_ACTION_TOP_MENU);
                    UI_STATUS_UPD( UI_TEXT_UNMOUNT_FILAMENT );
                    uid.lock();

                    GCode::executeFString(Com::tUnmountFilamentWithoutHeating);

                    if( unlock )
                    {
                        uid.unlock();
                    }
                    g_uStartOfIdle = HAL::timeInMilliseconds();
                }
                break;
            }
            case UI_ACTION_MOUNT_FILAMENT:
            {
                if( Extruder::current->tempControl.targetTemperatureC == 0 )
                {
                    char    unlock = !uid.locked;

                    g_uStartOfIdle = 0;
                    uid.executeAction(UI_ACTION_TOP_MENU);
                    UI_STATUS_UPD( UI_TEXT_MOUNT_FILAMENT );
                    uid.lock();

                    GCode::executeFString(Com::tMountFilamentWithHeating);

                    if( unlock )
                    {
                        uid.unlock();
                    }
                    g_uStartOfIdle = HAL::timeInMilliseconds();
                }
                else
                {
#if !EXTRUDER_ALLOW_COLD_MOVE
                    if( Extruder::current->tempControl.currentTemperatureC < UI_SET_MIN_EXTRUDER_TEMP )
                    {
                        // we do not allow to move the extruder in case it is not heated up enough
                        if( Printer::debugErrors() )
                        {
                            Com::printFLN( PSTR( "Load Filament: aborted" ) );
                        }

                        showError( (void*)ui_text_extruder, (void*)ui_text_operation_denied );
                        break;
                    }
#endif // !EXTRUDER_ALLOW_COLD_MOVE

                    char    unlock = !uid.locked;

                    g_uStartOfIdle = 0;
                    uid.executeAction(UI_ACTION_TOP_MENU);
                    UI_STATUS_UPD( UI_TEXT_MOUNT_FILAMENT );
                    uid.lock();

                    GCode::executeFString(Com::tMountFilamentWithoutHeating);

                    if( unlock )
                    {
                        uid.unlock();
                    }
                    g_uStartOfIdle = HAL::timeInMilliseconds();
                }
                break;
            }
            case UI_ACTION_SET_E_ORIGIN:
            {
                Printer::queuePositionLastSteps[E_AXIS] = 0;
                break;
            }
            case UI_ACTION_EXTRUDER_RELATIVE:
            {
                Printer::relativeExtruderCoordinateMode=!Printer::relativeExtruderCoordinateMode;
                break;
            }
            case UI_ACTION_SELECT_EXTRUDER0:
            {
                Extruder::selectExtruderById(0);
                break;
            }
            case UI_ACTION_SELECT_EXTRUDER1:
            {
#if NUM_EXTRUDER>1
                Extruder::selectExtruderById(1);
#endif // NUM_EXTRUDER>1

                break;
            }
            case UI_ACTION_SELECT_EXTRUDER2:
            {
#if NUM_EXTRUDER>2
                Extruder::selectExtruderById(2);
#endif // NUM_EXTRUDER>2
                
                break;
            }

#if NUM_EXTRUDER == 2
            case UI_ACTION_ACTIVE_EXTRUDER:
            {
                if( Extruder::current->id == 0 )    Extruder::selectExtruderById( 1 );
                else                                Extruder::selectExtruderById( 0 );
                break;
            }
#endif // NUM_EXTRUDER == 2

#if SDSUPPORT
            case UI_ACTION_SD_DELETE:
            {
                if(sd.sdactive)
                {
                    pushMenu((void*)&ui_menu_sd_fileselector,false);
                }
                else
                {
                    UI_ERROR(UI_TEXT_NOSDCARD);
                }
                break;
            }

#if FEATURE_RIGHT_BUTTON_MENU
            case UI_ACTION_RIGHT:   // fall through
#endif // FEATURE_RIGHT_BUTTON_MENU
            case UI_ACTION_SD_PRINT:
            {
                if(sd.sdactive)
                {
                    pushMenu((void*)&ui_menu_sd_fileselector,false);
                }
                break;
            }
            case UI_ACTION_SD_PAUSE:
            {
                uid.executeAction(UI_ACTION_TOP_MENU);
                pausePrint();
                break;
            }
            case UI_ACTION_SD_CONTINUE:
            {
                uid.executeAction(UI_ACTION_TOP_MENU);
                continuePrint();
                break;
            }

#if FEATURE_BEEPER
            case UI_ACTION_BEEPER:
            {
                if( Printer::enableBeeper ) Printer::enableBeeper = 0;
                else                        Printer::enableBeeper = 1;

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                HAL::eprSetByte( EPR_RF_BEEPER_MODE, Printer::enableBeeper );
                EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
                break;
            }
#endif // FEATURE_BEEPER

            case UI_ACTION_SD_UNMOUNT:
            {
                sd.unmount();
                break;
            }
            case UI_ACTION_SD_MOUNT:
            {
                sd.mount();
                break;
            }
            case UI_ACTION_MENU_SDCARD:
            {
                pushMenu((void*)&ui_menu_sd,false);
                break;
            }
#endif // SDSUPPORT

#if FEATURE_READ_CALIPER
            case UI_ACTION_CAL_RESET:
            {
                InterruptProtectedBlock noInts;
                caliper_collect_um = 0;
                caliper_collect_count = 0;
                noInts.unprotect();
                BEEP_ACCEPT_SET_POSITION
                break;
            }
            case UI_ACTION_CAL_SET:
            {
                if(caliper_collect_um && caliper_collect_count){
                    float dim = (float)caliper_filament_standard / (caliper_collect_um / caliper_collect_count);
                    float multi = 100.0f * dim*dim;
                    Commands::changeFlowrateMultiply(multi);
                    Com::printFLN( PSTR( "Set Flowrate Multiplier: " ), multi );
                    BEEP_ACCEPT_SET_POSITION
                }
                break;
            }
#endif //FEATURE_READ_CALIPER

#if FAN_PIN>-1 && FEATURE_FAN_CONTROL
            case UI_ACTION_FAN_OFF:
            {
                Commands::setFanSpeed(0,false);
                break;
            }
            case UI_ACTION_FAN_25:
            {
                Commands::setFanSpeed(64,false);
                break;
            }
            case UI_ACTION_FAN_50:
            {
                Commands::setFanSpeed(128,false);
                break;
            }
            case UI_ACTION_FAN_75:
            {
                Commands::setFanSpeed(192,false);
                break;
            }
            case UI_ACTION_FAN_FULL:
            {
                Commands::setFanSpeed(255,false);
                break;
            }
            case UI_ACTION_FAN_MODE:
            {
                Commands::adjustFanMode( (cooler_mode ? COOLER_MODE_PWM : COOLER_MODE_PDM) ); //0 = pwm, 1 = pdm
#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                HAL::eprSetByte( EPR_RF_FAN_MODE, cooler_mode );
                EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
                break;
            }
#endif // FAN_PIN>-1 && FEATURE_FAN_CONTROL

            case UI_ACTION_MENU_XPOS:
            {
                pushMenu((void*)&ui_menu_xpos,false);
                break;
            }
            case UI_ACTION_MENU_YPOS:
            {
                pushMenu((void*)&ui_menu_ypos,false);
                break;
            }
            case UI_ACTION_MENU_ZPOS:
            {
                pushMenu((void*)&ui_menu_zpos,false);
                break;
            }
            case UI_ACTION_MENU_XPOSFAST:
            {
                pushMenu((void*)&ui_menu_xpos_fast,false);
                break;
            }
            case UI_ACTION_MENU_YPOSFAST:
            {
                pushMenu((void*)&ui_menu_ypos_fast,false);
                break;
            }
            case UI_ACTION_MENU_ZPOSFAST:
            {
                pushMenu((void*)&ui_menu_zpos_fast,false);
                break;
            }
            case UI_ACTION_MENU_QUICKSETTINGS:
            {
                pushMenu((void*)&ui_menu_quick,false);
                break;
            }
            case UI_ACTION_MENU_EXTRUDER:
            {
                pushMenu((void*)&ui_menu_extruder,false);
                break;
            }
            case UI_ACTION_MENU_POSITIONS:
            {
                pushMenu((void*)&ui_menu_positions,false);
                break;
            }

#ifdef UI_USERMENU1
            case UI_ACTION_SHOW_USERMENU1:
            {
                pushMenu((void*)&UI_USERMENU1,false);
                break;
            }
#endif // UI_USERMENU1

#ifdef UI_USERMENU2
            case UI_ACTION_SHOW_USERMENU2:
            {
                pushMenu((void*)&UI_USERMENU2,false);
                break;
            }
#endif // UI_USERMENU2

#ifdef UI_USERMENU3
            case UI_ACTION_SHOW_USERMENU3:
            {
                pushMenu((void*)&UI_USERMENU3,false);
                break;
            }
#endif // UI_USERMENU3

#ifdef UI_USERMENU4
            case UI_ACTION_SHOW_USERMENU4:
            {
                pushMenu((void*)&UI_USERMENU4,false);
                break;
            }
#endif // UI_USERMENU4

#ifdef UI_USERMENU5
            case UI_ACTION_SHOW_USERMENU5:
            {
                pushMenu((void*)&UI_USERMENU5,false);
                break;
            }
#endif // UI_USERMENU5

#ifdef UI_USERMENU6
            case UI_ACTION_SHOW_USERMENU6:
            {
                pushMenu((void*)&UI_USERMENU6,false);
                break;
            }
#endif // UI_USERMENU6

#ifdef UI_USERMENU7
            case UI_ACTION_SHOW_USERMENU7:
            {
                pushMenu((void*)&UI_USERMENU7,false);
                break;
            }
#endif // UI_USERMENU7

#ifdef UI_USERMENU8
            case UI_ACTION_SHOW_USERMENU8:
            {
                pushMenu((void*)&UI_USERMENU8,false);
                break;
            }
#endif // UI_USERMENU8

#ifdef UI_USERMENU9
            case UI_ACTION_SHOW_USERMENU9:
            {
                pushMenu((void*)&UI_USERMENU9,false);
                break;
            }
#endif // UI_USERMENU9

#ifdef UI_USERMENU10
            case UI_ACTION_SHOW_USERMENU10:
            {
                pushMenu((void*)&UI_USERMENU10,false);
                break;
            }
#endif // UI_USERMENU10
            
#if MAX_HARDWARE_ENDSTOP_Z
            case UI_ACTION_SET_Z_ORIGIN:
            {
                setZOrigin();
                break;
            }
#endif // MAX_HARDWARE_ENDSTOP_Z

#ifdef DEBUG_PRINT
            case UI_ACTION_WRITE_DEBUG:
            {
                Com::printF(PSTR("Buf. Read Idx:"),(int)GCode::bufferReadIndex);
                Com::printF(PSTR(" Buf. Write Idx:"),(int)GCode::bufferWriteIndex);
                Com::printF(PSTR(" Comment:"),(int)GCode::commentDetected);
                Com::printF(PSTR(" Buf. Len:"),(int)GCode::bufferLength);
                Com::printF(PSTR(" Wait resend:"),(int)GCode::waitingForResend);
                Com::printFLN(PSTR(" Recv. Write Pos:"),(int)GCode::commandsReceivingWritePosition);
                //Com::printF(PSTR("Min. XY Speed:"),Printer::minimumSpeed);
                //Com::printF(PSTR(" Min. Z Speed:"),Printer::minimumZSpeed);
                Com::printF(PSTR(" Buffer:"),PrintLine::linesCount);
                Com::printF(PSTR(" Lines pos:"),(int)PrintLine::linesPos);
                Com::printFLN(PSTR(" Write Pos:"),(int)PrintLine::linesWritePos);
                Com::printFLN(PSTR("Wait loop:"),debugWaitLoop);
                Com::printF(PSTR("sd mode:"),(int)sd.sdmode);
                Com::printF(PSTR(" pos:"),sd.sdpos);
                Com::printFLN(PSTR(" of "),sd.filesize);
                break;
            }
#endif // DEBUG_PRINT

    #if FEATURE_ZERO_DIGITS
            case UI_ACTION_FEATURE_ZERO_DIGITS:
            {
                Printer::g_pressure_offset_active = (Printer::g_pressure_offset_active ? false : true);
    #if FEATURE_AUTOMATIC_EEPROM_UPDATE
                HAL::eprSetByte( EPR_RF_ZERO_DIGIT_STATE, (Printer::g_pressure_offset_active ? 1 : 2) ); //2 ist false, < 1 ist true
                EEPROM::updateChecksum();
    #endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
                break;
            }
    #endif // FEATURE_ZERO_DIGITS
    #if FEATURE_DIGIT_Z_COMPENSATION
            case UI_ACTION_DIGIT_COMPENSATION:
            {
                g_nDigitZCompensationDigits_active = (g_nDigitZCompensationDigits_active ? false : true);
    #if FEATURE_AUTOMATIC_EEPROM_UPDATE
                HAL::eprSetByte( EPR_RF_DIGIT_CMP_STATE, (g_nDigitZCompensationDigits_active ? 1 : 2) ); //2 ist false, < 1 ist true
                EEPROM::updateChecksum();
    #endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
                break;
            }
    #endif // FEATURE_DIGIT_Z_COMPENSATION

#if FEATURE_HEAT_BED_Z_COMPENSATION
            case UI_ACTION_RF_DO_MHIER_BED_SCAN:
            {           
                //macht an, wenn an, macht aus:         
                startZOScan();
                //gehe zurück und zeige dem User was passiert.
                uid.menuLevel = 0; 
                uid.menuPos[0] = 0;
                //wartet nur wenn an:
                //Commands::waitUntilEndOfZOS(); -> Nein, weil der Nutzer das aktiv steuern und abbrechen können soll. Ist ja hier kein M-code in Reihe.
                break;
            }
            case UI_ACTION_RF_DO_MHIER_AUTO_MATRIX_LEVELING:
            {           
                //macht an, wenn an, macht aus:         
                startZOScan(true); //Scan aber an vielen Punkten und Gewichtet.
                //gehe zurück und zeige dem User was passiert.
                uid.menuLevel = 0; 
                uid.menuPos[0] = 0;
                //wartet nur wenn an:
                //Commands::waitUntilEndOfZOS(); -> Nein, weil der Nutzer das aktiv steuern und abbrechen können soll. Ist ja hier kein M-code in Reihe.
                break;
            }
            case UI_ACTION_RF_DO_SAVE_ACTIVE_ZMATRIX:
            {
                // save the determined values to the EEPROM        
                if(g_ZMatrixChangedInRam){
                    uid.executeAction(UI_ACTION_TOP_MENU);
                    saveCompensationMatrix( (unsigned int)(EEPROM_SECTOR_SIZE * g_nActiveHeatBed) );
                    if( Printer::debugInfo() )
                    {
                        Com::printFLN( PSTR( "Manual Input: the heat bed compensation matrix has been saved" ) );
                    }                     
                    showInformation( (void*)ui_text_manual, (void*)ui_text_saving_success );
                }else{
                    showInformation( (void*)ui_text_manual, (void*)ui_text_saving_needless );
                }
                break;
            }
#endif // FEATURE_HEAT_BED_Z_COMPENSATION
#if FEATURE_ALIGN_EXTRUDERS
            case UI_ACTION_ALIGN_EXTRUDERS:
            {
                uid.menuPos[0] = 0;
                uid.menuLevel = 0;
                startAlignExtruders();
                break;
            }
#endif // FEATURE_ALIGN_EXTRUDERS
        }
    }

    refreshPage();
    if(!skipBeep)
        BEEP_SHORT

#if FEATURE_MILLING_MODE
    if( Printer::operatingMode == OPERATING_MODE_PRINT )
    {
#if UI_PRINT_AUTORETURN_TO_MENU_AFTER!=0
        g_nAutoReturnTime=HAL::timeInMilliseconds()+UI_PRINT_AUTORETURN_TO_MENU_AFTER;
#endif // UI_PRINT_AUTORETURN_TO_MENU_AFTER!=0
    }
    else
    {
#if UI_MILL_AUTORETURN_TO_MENU_AFTER!=0
        g_nAutoReturnTime=HAL::timeInMilliseconds()+UI_MILL_AUTORETURN_TO_MENU_AFTER;
#endif // UI_MILL_AUTORETURN_TO_MENU_AFTER!=0
    }
#else
#if UI_PRINT_AUTORETURN_TO_MENU_AFTER!=0
    g_nAutoReturnTime=HAL::timeInMilliseconds()+UI_PRINT_AUTORETURN_TO_MENU_AFTER;
#endif // UI_PRINT_AUTORETURN_TO_MENU_AFTER!=0
#endif // FEATURE_MILLING_MODE
#endif // UI_HAS_KEYS==1

} // executeAction

void UIDisplay::slowAction()
{
    millis_t  time    = HAL::timeInMilliseconds();
    uint8_t   refresh = 0;

#if UI_HAS_KEYS==1
    // Update key buffer
    InterruptProtectedBlock noInts; //HAL::forbidInterrupts();
    if( (flags & (UI_FLAG_FAST_KEY_ACTION + UI_FLAG_KEY_TEST_RUNNING)) == 0 )
    {
        flags|=UI_FLAG_KEY_TEST_RUNNING;

            noInts.unprotect(); //HAL::allowInterrupts();

            int16_t nextAction = 0;
            ui_check_slow_keys(nextAction);  //Nibbels: Das macht garnix.
            if(lastButtonAction!=nextAction)
            {
                lastButtonStart = time;
                lastButtonAction = nextAction;
                noInts.protect(); //HAL::forbidInterrupts();
                flags|=UI_FLAG_SLOW_KEY_ACTION;
            }else{
                noInts.protect(); //HAL::forbidInterrupts();
            }

        flags &= ~UI_FLAG_KEY_TEST_RUNNING;
    }

    if( (flags & UI_FLAG_SLOW_ACTION_RUNNING) == 0 )
    {
        flags |= UI_FLAG_SLOW_ACTION_RUNNING;
        
        // Reset click encoder
        //HAL::forbidInterrupts(); //_-> Ist schon protected!! in jedem aller fälle. entweder ist das mist oder vorher n bug.
        int8_t epos = encoderPos;
        encoderPos=0;
        noInts.unprotect(); //HAL::allowInterrupts();
        if(epos)
        {
            nextPreviousAction(epos); //Nibbels: Funktion, die rechts links auswertet oder Errors wegklicken lässt. abhängig von this->encoderPos .. sonst verwendet mit -1 oder 1
            BEEP_SHORT
            refresh=1;
        }
        if(lastAction!=lastButtonAction)
        {
#if FEATURE_UNLOCK_MOVEMENT
            //Bei beliebiger user interaktion oder Homing soll G1 etc. erlaubt werden. Dann ist der Drucker nicht abgestürzt, sondern bedient worden.
            Printer::g_unlock_movement = 1;
#endif //FEATURE_UNLOCK_MOVEMENT
            if(lastButtonAction==0)
            {
                lastAction = 0;
                noInts.protect(); //HAL::forbidInterrupts();
                flags &= ~3;
            }
            else if(time-lastButtonStart>UI_KEY_BOUNCETIME)     // New key pressed
            {
                lastAction = lastButtonAction;
                executeAction(lastAction);
                nextRepeat = time+UI_KEY_FIRST_REPEAT;
                repeatDuration = UI_KEY_FIRST_REPEAT;
            }
        }
        else if(lastAction<1000 && lastAction)     // Repeatable key
        {
            if(time-nextRepeat<10000)
            {
                executeAction(lastAction);
                repeatDuration -= UI_KEY_REDUCE_REPEAT;
                if(repeatDuration<UI_KEY_MIN_REPEAT) repeatDuration = UI_KEY_MIN_REPEAT;
                nextRepeat = time+repeatDuration;
            }
        }
        noInts.protect(); //HAL::forbidInterrupts();
        flags &= ~UI_FLAG_SLOW_ACTION_RUNNING;
    }
    noInts.unprotect(); //HAL::allowInterrupts();
#endif // UI_HAS_KEYS==1

#if UI_PRINT_AUTORETURN_TO_MENU_AFTER || UI_MILL_AUTORETURN_TO_MENU_AFTER
    if(menuLevel > 0 && g_nAutoReturnTime && g_nAutoReturnTime<time)
    {
        if( menu[menuLevel] != &ui_menu_message || g_nAutoReturnMessage )
        {
            lastSwitch = time;
            menuLevel = 0;
            activeAction = 0;
            g_nAutoReturnMessage = false;
        }
        g_nAutoReturnTime = 0;
    }
#endif // UI_PRINT_AUTORETURN_TO_MENU_AFTER || UI_MILL_AUTORETURN_TO_MENU_AFTER

    if(menuLevel == 0 && time > 4000)
    {
        if(time - lastSwitch > UI_PAGES_DURATION)
        {
            lastSwitch = time;
#if !defined(UI_DISABLE_AUTO_PAGESWITCH) || !UI_DISABLE_AUTO_PAGESWITCH
            menuPos[0]++;
            if(menuPos[0] >= UI_NUM_PAGES)
                menuPos[0] = 0;
#endif // !defined(UI_DISABLE_AUTO_PAGESWITCH) || !UI_DISABLE_AUTO_PAGESWITCH
            refresh = 1;
        }
        else if(time - lastRefresh >= 1000) refresh=1;
    }
    else if(time - lastRefresh >= 800)
    {
        //UIMenu *men = (UIMenu*)menu[menuLevel];
        //uint8_t mtype = pgm_read_byte((void*)&(men->menuType));
        //(void)mtype; //ignore unused error Nibbels -> ignore all by repetier  https://github.com/repetier/Repetier-Firmware/commit/57c21814d8f8946852fa27af2a7c31831b493ec1
        refresh=1;
    }

    if(refresh)
    {
        if (menuLevel > 1 || Printer::isAutomount())
        {
            shift++;
            if(shift+UI_COLS>MAX_COLS+1)
                shift = -2;
        }
        else
            shift = -2;

        refreshPage();
        lastRefresh = time;
    }

} // slowAction


void UIDisplay::fastAction()
{
#if UI_HAS_KEYS==1
    // Check keys
    InterruptProtectedBlock noInts; //HAL::forbidInterrupts();

    if( (flags & (UI_FLAG_KEY_TEST_RUNNING + UI_FLAG_SLOW_KEY_ACTION)) == 0 )
    {
        flags |= UI_FLAG_KEY_TEST_RUNNING;

            int16_t nextAction = 0;
            ui_check_keys(nextAction);

            if(lastButtonAction!=nextAction)
            {
                lastButtonStart = HAL::timeInMilliseconds();
                lastButtonAction = nextAction;
                flags |= UI_FLAG_FAST_KEY_ACTION;
                if( nextAction == UI_ACTION_RF_CONTINUE )
                {
                    g_nContinueButtonPressed = 1;
                }
            }

            if(!nextAction)
            {
                // no key is pressed at the moment
                if(PrintLine::direct.task == TASK_MOVE_FROM_BUTTON)
                {
                    // the current direct movement has been started via a hardware or menu button - these movements shall be stopped as soon as the button is released
                    PrintLine::stopDirectMove();
                }
            }  

        flags &= ~UI_FLAG_KEY_TEST_RUNNING;
    }
    noInts.unprotect(); //HAL::allowInterrupts();
#endif //  UI_HAS_KEYS==1

} // fastAction


void UIDisplay::lock()
{
    locked = 1;
    return;

} // lock


void UIDisplay::unlock()
{
    locked = 0;
    return;

} // unlock


#if UI_ENCODER_SPEED==0
const int8_t encoder_table[16] PROGMEM = {0,1,-1,0,-1,0,0,1,1,0,0,-1,0,-1,1,0};     // Full speed
#elif UI_ENCODER_SPEED==1
const int8_t encoder_table[16] PROGMEM = {0,0,-1,0,0,0,0,1,1,0,0,0,0,-1,0,0};       // Half speed
#else
const int8_t encoder_table[16] PROGMEM = {0,0,0,0,0,0,0,0,0,0,0,-1,0,0,1,0};        // Quart speed
#endif // UI_ENCODER_SPEED==0

#endif //  UI_DISPLAY_TYPE!=0
