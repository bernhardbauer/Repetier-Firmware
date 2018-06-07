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


#ifndef HAL_H
#define HAL_H

/**
  This is the main Hardware Abstraction Layer (HAL).
  To make the firmware work with different processors and toolchains,
  all hardware related code should be packed into the hal files.
*/

#include <avr/pgmspace.h>
#include <avr/io.h>
#define prog_char   char

#define INLINE __attribute__((always_inline))

#define PACK

#define FSTRINGVALUE(var,value) const char var[] PROGMEM = value;
#define FSTRINGVAR(var) static const char var[] PROGMEM;
#define FSTRINGPARAM(var) PGM_P var

#include <avr/eeprom.h>
#include <avr/wdt.h>
/** \brief Prescale factor, timer0 runs at.

All known arduino boards use 64. This value is needed for the extruder timing. */
#define TIMER0_PRESCALE 64

#define ANALOG_PRESCALER _BV(ADPS0)|_BV(ADPS1)|_BV(ADPS2)

#ifndef EXTERNALSERIAL
#define  HardwareSerial_h // Don't use standard serial console
#endif

#include <inttypes.h>
#include "Stream.h"
#include "Print.h"

#include "Arduino.h"
#define COMPAT_PRE1

#include "fastio.h"

#if FEATURE_WATCHDOG
extern  unsigned char g_bPingWatchdog;
extern  unsigned long g_uLastCommandLoop;
#endif // FEATURE_WATCHDOG

// > 4 should not happen, only if you change code or really want that. 
// Never set this > 7!
// Note that > 4 is highly experimental
#define COOLER_MODE_MAX 4

#define COOLER_MODE_PWM 0
#define COOLER_MODE_PDM 1
extern  uint8_t cooler_pwm_speed;
extern  uint8_t cooler_pwm_step;
extern  uint8_t cooler_pwm_mask;
extern  uint8_t cooler_mode;
 
#if !defined(MAX_FAN_PWM) || MAX_FAN_PWM >= 255 || MAX_FAN_PWM <= 0
#define TRIM_FAN_PWM(x) x
#undef MAX_FAN_PWM
#define MAX_FAN_PWM 255
#else
#define TRIM_FAN_PWM(x) static_cast<uint8_t>(static_cast<unsigned int>(x) * MAX_FAN_PWM / 255)
#endif

class InterruptProtectedBlock
{
private:
    uint8_t sreg;
public:
    inline void protect(bool save = false)
    {
        if(save) sreg = SREG;
        cli();
    }

    inline void unprotect()
    {
        SREG = sreg;
    }

    inline InterruptProtectedBlock(bool later = false)
    {
        sreg = SREG;
        if(!later)
            cli();
    }

    inline ~InterruptProtectedBlock()
    {
        SREG = sreg;
    }
};

#define EEPROM_OFFSET               0
#define SECONDS_TO_TICKS(s)         (unsigned long)(s*(float)F_CPU)
#define ANALOG_REDUCE_BITS          0
#define ANALOG_REDUCE_FACTOR        1

#define MAX_RAM                     8192

#define bit_clear(x,y)              x&= ~(1<<y)
#define bit_set(x,y)                x|= (1<<y)

/** defines the data direction (reading from I2C device) in i2cStart(),i2cRepStart() */
#define I2C_READ                    1

/** defines the data direction (writing to I2C device) in i2cStart(),i2cRepStart() */
#define I2C_WRITE                   0

#define LIMIT_INTERVAL              ((F_CPU/40000)+1)

#ifndef cbi
  #define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
  #define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

typedef uint16_t    speed_t;
typedef uint32_t    ticks_t;
typedef uint32_t    millis_t;
typedef uint8_t     flag8_t;
typedef int8_t      fast8_t;
typedef uint8_t     ufast8_t;

#define FAST_INTEGER_SQRT

#if FEATURE_USER_INT3
extern volatile unsigned long g_uCOUNT_INT3; //dummy variable for dummy function!

void USER_INTERRUPT3_HOOK(); //dummy function!
#endif //FEATURE_USER_INT3

#if FEATURE_READ_CALIPER
extern volatile long caliper_um; //last valid readout

//auswertevariablen:
extern uint16_t caliper_filament_standard;
extern uint32_t caliper_collect_um;
extern uint32_t caliper_collect_count;
extern int8_t   caliper_collect_adjust;

void FEATURE_READ_CALIPER_HOOK(); //read in calipers 48bit protocol bitwise!
#endif //FEATURE_READ_CALIPER

#ifndef EXTERNALSERIAL
// Implement serial communication for one stream only!
/*
  HardwareSerial.h - Hardware serial library for Wiring
  Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  Modified 28 September 2010 by Mark Sproul

  Modified to use only 1 queue with fixed length by Repetier
*/

#undef SERIAL_RX_BUFFER_SIZE
//undef fix gegen compiler warning   
#define SERIAL_RX_BUFFER_SIZE   128
#define SERIAL_RX_BUFFER_MASK   127

#undef SERIAL_TX_BUFFER_SIZE
#undef SERIAL_TX_BUFFER_MASK
#ifdef BIG_OUTPUT_BUFFER
  #define SERIAL_TX_BUFFER_SIZE 128
  #define SERIAL_TX_BUFFER_MASK 127
#else
  #define SERIAL_TX_BUFFER_SIZE 64
  #define SERIAL_TX_BUFFER_MASK 63
#endif

struct ring_buffer_rx
{
    unsigned char buffer[SERIAL_RX_BUFFER_SIZE];
    volatile uint8_t head;
    volatile uint8_t tail;
};

struct ring_buffer_tx
{
    unsigned char buffer[SERIAL_TX_BUFFER_SIZE];
    volatile uint8_t head;
    volatile uint8_t tail;
};

class RFHardwareSerial : public Print
{
public:
    ring_buffer_rx      *_rx_buffer;
    ring_buffer_tx      *_tx_buffer;
    volatile uint8_t    *_ubrrh;
    volatile uint8_t    *_ubrrl;
    volatile uint8_t    *_ucsra;
    volatile uint8_t    *_ucsrb;
    volatile uint8_t    *_udr;
    uint8_t             _rxen;
    uint8_t             _txen;
    uint8_t             _rxcie;
    uint8_t             _udrie;
    uint8_t             _u2x;
public:
    RFHardwareSerial(ring_buffer_rx *rx_buffer, ring_buffer_tx *tx_buffer,
                     volatile uint8_t *ubrrh, volatile uint8_t *ubrrl,
                     volatile uint8_t *ucsra, volatile uint8_t *ucsrb,
                     volatile uint8_t *udr,
                     uint8_t rxen, uint8_t txen, uint8_t rxcie, uint8_t udrie, uint8_t u2x);
    void begin(unsigned long);
    void end();
    virtual int available(void);
    virtual int peek(void);
    virtual int read(void);
    virtual void flush(void);
    virtual size_t write(uint8_t);
    using Print::write; // pull in write(str) and write(buf, size) from Print
    operator bool();
    int outputUnused(void); // Used for output in interrupts
};

extern RFHardwareSerial RFSerial;
#define RFSERIAL RFSerial
#define WAIT_OUT_EMPTY while(tx_buffer.head != tx_buffer.tail) {}
#else // EXTERNALSERIAL
#define RFSERIAL Serial
#endif // EXTERNALSERIAL

#define OUT_P_I(p,i)        Com::printF(PSTR(p),(int)(i))
#define OUT_P_I_LN(p,i)     Com::printFLN(PSTR(p),(int)(i))
#define OUT_P_L(p,i)        Com::printF(PSTR(p),(long)(i))
#define OUT_P_L_LN(p,i)     Com::printFLN(PSTR(p),(long)(i))
#define OUT_P_F(p,i)        Com::printF(PSTR(p),(float)(i))
#define OUT_P_F_LN(p,i)     Com::printFLN(PSTR(p),(float)(i))
#define OUT_P_FX(p,i,x)     Com::printF(PSTR(p),(float)(i),x)
#define OUT_P_FX_LN(p,i,x)  Com::printFLN(PSTR(p),(float)(i),x)
#define OUT_P(p)            Com::printF(PSTR(p))
#define OUT_P_LN(p)         Com::printFLN(PSTR(p))
#define OUT_ERROR_P(p)      Com::printErrorF(PSTR(p))
#define OUT_ERROR_P_LN(p)   {Com::printErrorF(PSTR(p));Com::println();}
#define OUT(v)              Com::print(v)
#define OUT_LN              Com::println()



class HAL
{
public:
    HAL();
    virtual ~HAL();

    // return val'val
    static uint16_t integerSqrt(int32_t a);

    /** \brief Optimized division
    Normally the C compiler will compute a long/long division, which takes ~670 Ticks.
    This version is optimized for a 16 bit dividend and recognises the special cases
    of a 24 bit and 16 bit dividend, which offen, but not always occur in updating the
    interval.
    */
    static inline int32_t Div4U2U(uint32_t a,uint16_t b)
    {
        // r14/r15 remainder
        // r16 counter
        __asm__ __volatile__ (
            "clr r14 \n\t"
            "sub r15,r15 \n\t"
            "tst %D0 \n\t"
            "brne do32%= \n\t"
            "tst %C0 \n\t"
            "breq donot24%= \n\t"
            "rjmp do24%= \n\t"
            "donot24%=:" "ldi r16,17 \n\t" // 16 Bit divide
            "d16u_1%=:" "rol %A0 \n\t"
            "rol %B0 \n\t"
            "dec r16 \n\t"
            "brne   d16u_2%= \n\t"
            "rjmp end%= \n\t"
            "d16u_2%=:" "rol r14 \n\t"
            "rol r15 \n\t"
            "sub r14,%A2 \n\t"
            "sbc r15,%B2 \n\t"
            "brcc   d16u_3%= \n\t"
            "add r14,%A2 \n\t"
            "adc r15,%B2 \n\t"
            "clc \n\t"
            "rjmp d16u_1%= \n\t"
            "d16u_3%=:" "sec \n\t"
            "rjmp d16u_1%= \n\t"
            "do32%=:" // divide full 32 bit
            "rjmp do32B%= \n\t"
            "do24%=:" // divide 24 bit

            "ldi r16,25 \n\t" // 24 Bit divide
            "d24u_1%=:" "rol %A0 \n\t"
            "rol %B0 \n\t"
            "rol %C0 \n\t"
            "dec r16 \n\t"
            "brne   d24u_2%= \n\t"
            "rjmp end%= \n\t"
            "d24u_2%=:" "rol r14 \n\t"
            "rol r15 \n\t"
            "sub r14,%A2 \n\t"
            "sbc r15,%B2 \n\t"
            "brcc   d24u_3%= \n\t"
            "add r14,%A2 \n\t"
            "adc r15,%B2 \n\t"
            "clc \n\t"
            "rjmp d24u_1%= \n\t"
            "d24u_3%=:" "sec \n\t"
            "rjmp d24u_1%= \n\t"

            "do32B%=:" // divide full 32 bit

            "ldi r16,33 \n\t" // 32 Bit divide
            "d32u_1%=:" "rol %A0 \n\t"
            "rol %B0 \n\t"
            "rol %C0 \n\t"
            "rol %D0 \n\t"
            "dec r16 \n\t"
            "brne   d32u_2%= \n\t"
            "rjmp end%= \n\t"
            "d32u_2%=:" "rol r14 \n\t"
            "rol r15 \n\t"
            "sub r14,%A2 \n\t"
            "sbc r15,%B2 \n\t"
            "brcc   d32u_3%= \n\t"
            "add r14,%A2 \n\t"
            "adc r15,%B2 \n\t"
            "clc \n\t"
            "rjmp d32u_1%= \n\t"
            "d32u_3%=:" "sec \n\t"
            "rjmp d32u_1%= \n\t"

            "end%=:" // end
            :"=&r"(a)
            :"0"(a),"r"(b)
            :"r14","r15","r16"
        );
        return a;

    } // Div4U2U

    static inline unsigned long U16SquaredToU32(unsigned int val)
    {
        long res;


        __asm__ __volatile__ ( // 15 Ticks
            "mul %A1,%A1 \n\t"
            "movw %A0,r0 \n\t"
            "mul %B1,%B1 \n\t"
            "movw %C0,r0 \n\t"
            "mul %A1,%B1 \n\t"
            "clr %A1 \n\t"
            "add %B0,r0 \n\t"
            "adc %C0,r1 \n\t"
            "adc %D0,%A1 \n\t"
            "add %B0,r0 \n\t"
            "adc %C0,r1 \n\t"
            "adc %D0,%A1 \n\t"
            "clr r1 \n\t"
            : "=&r"(res),"=r"(val)
            : "1"(val)
        );
        return res;

    } // U16SquaredToU32

    static inline speed_t ComputeV(long timer,long accel)
    {
        unsigned int res;


        // 38 Ticks
        __asm__ __volatile__ ( // 0 = res, 1 = timer, 2 = accel %D2=0 ,%A1 are unused is free
            // Result LSB first: %A0, %B0, %A1
            "mul %B1,%A2 \n\t"
            "mov %A0,r1 \n\t"
            "mul %B1,%C2 \n\t"
            "mov %B0,r0 \n\t"
            "mov %A1,r1 \n\t"
            "mul %B1,%B2 \n\t"
            "add %A0,r0 \n\t"
            "adc %B0,r1 \n\t"
            "adc %A1,%D2 \n\t"
            "mul %C1,%A2 \n\t"
            "add %A0,r0 \n\t"
            "adc %B0,r1 \n\t"
            "adc %A1,%D2 \n\t"
            "mul %C1,%B2 \n\t"
            "add %B0,r0 \n\t"
            "adc %A1,r1 \n\t"
            "mul %D1,%A2 \n\t"
            "add %B0,r0 \n\t"
            "adc %A1,r1 \n\t"
            "mul %C1,%C2 \n\t"
            "add %A1,r0 \n\t"
            "mul %D1,%B2 \n\t"
            "add %A1,r0 \n\t"
            "lsr %A1 \n\t"
            "ror %B0 \n\t"
            "ror %A0 \n\t"
            "lsr %A1 \n\t"
            "ror %B0 \n\t"
            "ror %A0 \n\t"
            "clr r1 \n\t"
            :"=&r"(res),"=r"(timer),"=r"(accel)
            :"1"(timer),"2"(accel)
            : );
        // unsigned int v = ((timer>>8)*cur->accel)>>10;
        return res;

    } // ComputeV

    // Multiply two 16 bit values and return 32 bit result
    static inline uint32_t mulu16xu16to32(unsigned int a,unsigned int b)
    {
        uint32_t res;


        // 18 Ticks = 1.125 us
        __asm__ __volatile__ ( // 0 = res, 1 = timer, 2 = accel %D2=0 ,%A1 are unused is free
            // Result LSB first: %A0, %B0, %A1
            "clr r18 \n\t"
            "mul %B2,%B1 \n\t" // mul hig bytes
            "movw %C0,r0 \n\t"
            "mul %A1,%A2 \n\t" // mul low bytes
            "movw %A0,r0 \n\t"
            "mul %A1,%B2 \n\t"
            "add %B0,r0 \n\t"
            "adc %C0,r1 \n\t"
            "adc %D0,r18 \n\t"
            "mul %B1,%A2 \n\t"
            "add %B0,r0 \n\t"
            "adc %C0,r1 \n\t"
            "adc %D0,r18 \n\t"
            "clr r1 \n\t"
            :"=&r"(res),"=r"(a),"=r"(b)
            :"1"(a),"2"(b)
            :"r18" );
        // return (long)a*b;
        return res;

    } // mulu16xu16to32

    // Multiply two 16 bit values and return 16 bit result
    static inline unsigned int mulu6xu16shift16(unsigned int a,unsigned int b)
    {
        unsigned int res;


        // 18 Ticks = 1.125 us
        __asm__ __volatile__ ( // 0 = res, 1 = timer, 2 = accel %D2=0 ,%A1 are unused is free
            // Result LSB first: %A0, %B0, %A1
            "clr r18 \n\t"
            "mul %B2,%B1 \n\t" // mul hig bytes
            "movw %A0,r0 \n\t"
            "mul %A1,%A2 \n\t" // mul low bytes
            "mov r19,r1 \n\t"
            "mul %A1,%B2 \n\t"
            "add r19,r0 \n\t"
            "adc %A0,r1 \n\t"
            "adc %B0,r18 \n\t"
            "mul %B1,%A2 \n\t"
            "add r19,r0 \n\t"
            "adc %A0,r1 \n\t"
            "adc %B0,r18 \n\t"
            "clr r1 \n\t"
            :"=&r"(res),"=r"(a),"=r"(b)
            :"1"(a),"2"(b)
            :"r18","r19" );
        return res;

    } // mulu6xu16shift16

    static inline void digitalWrite(uint8_t pin,uint8_t value)
    {
        ::digitalWrite(pin,value);

    } // digitalWrite

    static inline uint8_t digitalRead(uint8_t pin)
    {
        return ::digitalRead(pin);

    } // digitalRead

    static inline void pinMode(uint8_t pin,uint8_t mode)
    {
        ::pinMode(pin,mode);

    } // pinMode

    static int32_t CPUDivU2(unsigned int divisor);

    static inline void delayMicroseconds(unsigned int delayUs) //see https://www.arduino.cc/en/Reference/AttachInterrupt for interrupt-warning.
    {
        ::delayMicroseconds(delayUs);
    } // delayMicroseconds

    static inline void delayMilliseconds(unsigned int delayMs)
    {
        ::delay(delayMs);
    } // delayMilliseconds

    static inline void tone(uint8_t pin,int duration)
    {
        ::tone(pin,duration);

    } // tone

    static inline void noTone(uint8_t pin)
    {
        ::noTone(pin);

    } // noTone

    static inline void eprSetByte(unsigned int pos,uint8_t value)
    {
        uint8_t oldval = eprGetByte(pos);
        if(oldval != value) eeprom_write_byte((unsigned char *)(EEPROM_OFFSET+pos), value);

    } // eprSetByte

    static inline void eprSetInt16(unsigned int pos,int16_t value)
    {
        int16_t oldval = eprGetInt16(pos);
        if(oldval != value) eeprom_write_word((unsigned int*)(EEPROM_OFFSET+pos),value);

    } // eprSetInt16

    static inline void eprSetInt32(unsigned int pos,int32_t value)
    {
        int32_t oldval = eprGetInt32(pos);
        if(oldval != value) eeprom_write_dword((uint32_t*)(EEPROM_OFFSET+pos),value);

    } // eprSetInt32

    static inline void eprSetFloat(unsigned int pos,float value)
    {
        float oldval = eprGetFloat(pos);
        if(oldval != value) eeprom_write_block(&value,(void*)(EEPROM_OFFSET+pos), 4);

    } // eprSetFloat

    static inline uint8_t eprGetByte(unsigned int pos)
    {
        return eeprom_read_byte ((unsigned char *)(EEPROM_OFFSET+pos));

    } // eprGetByte

    static inline int16_t eprGetInt16(unsigned int pos)
    {
        return eeprom_read_word((uint16_t *)(EEPROM_OFFSET+pos));

    } // eprGetInt16

    static inline int32_t eprGetInt32(unsigned int pos)
    {
        return eeprom_read_dword((uint32_t*)(EEPROM_OFFSET+pos));

    } // eprGetInt32

    static inline float eprGetFloat(unsigned int pos)
    {
        float v;
        eeprom_read_block(&v,(void *)(EEPROM_OFFSET+pos),4); // newer gcc have eeprom_read_block but not arduino 22
        return v;

    } // eprGetFloat

    // Faster version of InterruptProtectedBlock.
    // For safety it may only be called from within an
    // interrupt handler.
    static inline void allowInterrupts()
    {
        sei();
    } // allowInterrupts

    static inline void forbidInterrupts()
    {
        cli();
    } // forbidInterrupts

    static inline unsigned long timeInMilliseconds()
    {
        return millis();
    } // timeInMilliseconds

    static inline unsigned long timeInMicroseconds()
    {
        return micros();
    } // timeInMicroseconds

    static inline char readFlashByte(PGM_P ptr)
    {
        return pgm_read_byte(ptr);

    } // readFlashByte

    static inline void serialSetBaudrate(long baud)
    {
        RFSERIAL.begin(baud);

    } // serialSetBaudrate

    static inline bool serialByteAvailable()
    {
        return RFSERIAL.available()>0;

    } // serialByteAvailable

    static inline uint8_t serialReadByte()
    {
        return RFSERIAL.read();

    } // serialReadByte

    static inline void serialWriteByte(char b)
    {
        RFSERIAL.write(b);

    } // serialWriteByte

    static inline void serialFlush()
    {
        RFSERIAL.flush();

    } // serialFlush

    static void setupTimer();
    static void showStartReason();
    static int getFreeRam();
    static void resetHardware();

    // SPI related functions
    static void spiBegin()
    {
#if SDSS>=0
        SET_INPUT(MISO_PIN);
        SET_OUTPUT(MOSI_PIN);
        SET_OUTPUT(SCK_PIN);
        
        // SS must be in output mode even it is not chip select
        SET_OUTPUT(SDSS);

#if SDSSORIG >- 1
        SET_OUTPUT(SDSSORIG);
#endif // SDSSORIG >- 1

        // set SS high - may be chip select for another SPI device
#if SET_SPI_SS_HIGH
        WRITE(SDSS, HIGH);
#endif // SET_SPI_SS_HIGH
#endif // SDSS>=0

    } // spiBegin

    static inline void spiInit(uint8_t spiRate) //TODO ?? : see https://github.com/repetier/Repetier-Firmware/commit/5c139806139d738419e666147c28b51971e1bab5 ??
    {
        //spiRate = spiRate > 12 ? 6 : spiRate/2;
        uint8_t r = 0;
        for (uint8_t b = 2; spiRate > b && r < 6; b <<= 1, r++);

        SET_OUTPUT(SS);
        WRITE(SS,HIGH);
        SET_OUTPUT(SCK);
        SET_OUTPUT(MOSI_PIN);
        SET_INPUT(MISO_PIN);

#ifdef  PRR
        PRR &= ~(1<<PRSPI);
#elif defined PRR0
        PRR0 &= ~(1<<PRSPI);
#endif // PRR

        // See avr processor documentation
        //SPCR = (1 << SPE) | (1 << MSTR) | (spiRate >> 1);
        //SPSR = spiRate & 1 || spiRate == 6 ? 0 : 1 << SPI2X;

        SPCR = (1 << SPE) | (1 << MSTR) | (r >> 1);
        SPSR = (r & 1 || r == 6 ? 0 : 1) << SPI2X;
    } // spiInit

    static inline uint8_t spiReceive(uint8_t send=0xff)
    {
        SPDR = send;
        while (!(SPSR & (1 << SPIF)));
        return SPDR;
    } // spiReceive

    static inline void spiReadBlock(uint8_t*buf,size_t nbyte)
    {
        if (nbyte-- == 0) return;
        SPDR = 0XFF;
        for (size_t i = 0; i < nbyte; i++)
        {
            while (!(SPSR & (1 << SPIF)));
            buf[i] = SPDR;
            SPDR = 0XFF;
        }
        while (!(SPSR & (1 << SPIF)));
        buf[nbyte] = SPDR;

    } // spiReadBlock

    static inline void spiSend(uint8_t b)
    {
        SPDR = b;
        while (!(SPSR & (1 << SPIF)));

    } // spiSend

    static inline void spiSend(const uint8_t* buf , size_t n)
    {
        if (n == 0) return;
        SPDR = buf[0];
        if (n > 1)
        {
            uint8_t b = buf[1];
            size_t i = 2;
            while (1)
            {
                while (!(SPSR & (1 << SPIF)));
                SPDR = b;
                if (i == n) break;
                b = buf[i++];
            }
        }
        while (!(SPSR & (1 << SPIF)));

    } // spiSend

    static inline __attribute__((always_inline))
    void spiSendBlock(uint8_t token, const uint8_t* buf)
    {
        SPDR = token;
        for (uint16_t i = 0; i < 512; i += 2)
        {
            while (!(SPSR & (1 << SPIF)));
            SPDR = buf[i];
            while (!(SPSR & (1 << SPIF)));
            SPDR = buf[i + 1];
        }
        while (!(SPSR & (1 << SPIF)));

    } // spiSendBlock

    // I2C Support
    static void i2cInit(unsigned long clockSpeedHz);
    static unsigned char i2cStart(unsigned char address);
    static void i2cStartWait(unsigned char address);
    static void i2cStop(void);
    static void i2cWrite( unsigned char data );
    static unsigned char i2cReadAck(void);
    static unsigned char i2cReadNak(void);

    //Nibbels: Use watchdog to trigger external watchdog with constant time function according to command loop.
    static void WDT_Init(void);

    // Watchdog support
    inline static void startWatchdog()
    {
#if FEATURE_WATCHDOG && WATCHDOG_PIN>-1
        // external watchdog
        SET_OUTPUT(WATCHDOG_PIN);
        g_bPingWatchdog = 1; //allow pinging
        tellWatchdogOk(); //Nibbels: Init für g_nCommandloop
        pingWatchdog(); //Nibbels: Hier macht der mehr sinn!        
#endif // FEATURE_WATCHDOG && WATCHDOG_PIN>-1

        HAL::WDT_Init(); //Nibbels: use watchdogtimer to test var and trigger
    } // startWatchdog

    inline static void stopWatchdog()
    {
        g_bPingWatchdog = 0; //disallow pinging

#if FEATURE_WATCHDOG && WATCHDOG_PIN>-1
        // external watchdog
        WRITE(WATCHDOG_PIN,LOW); //Nibbels: In case you stop the watchdog it has to be set floating. thatwhy disable internal pullup as this will make the state like OUTPUT-high. This is not really needed as this function does not do anything when starting the arduino. rightnow at start it sets a pin to input which is already input.
        SET_INPUT(WATCHDOG_PIN);
#else
        // internal watchdog
        //wdt_disable();
#endif // FEATURE_WATCHDOG && WATCHDOG_PIN>-1
    } // stopWatchdog

    inline static void tellWatchdogOk()
    {
#if FEATURE_WATCHDOG && WATCHDOG_PIN>-1
        g_uLastCommandLoop = HAL::timeInMilliseconds();    
#else

#endif // FEATURE_WATCHDOG && WATCHDOG_PIN>-1
    } // pingWatchdog

    inline static void pingWatchdog()
    {
        if( !g_bPingWatchdog )
        {
            // do not trigger the watchdog in case it is not enabled
            return;
        }
#if FEATURE_WATCHDOG && WATCHDOG_PIN>-1
        // external watchdog
        if( (HAL::timeInMilliseconds() - g_uLastCommandLoop) < WATCHDOG_MAIN_LOOP_TIMEOUT )
        {
            // ping the watchdog only in case the mainloop is still being called
            WRITE(WATCHDOG_PIN,READ(WATCHDOG_PIN) ? 0 : 1);
        }
#else
        // internal watchdog
        //wdt_reset();
#endif // FEATURE_WATCHDOG && WATCHDOG_PIN>-1

    } // pingWatchdog

    inline static void testWatchdog()
    {
        // start the watchdog
        //startWatchdog();

        // force the watchdog to fire
        //InterruptProtectedBlock noInts; //HAL::forbidInterrupts();
        while( 1 )
        {
        }
        //noInts.unprotect(); //Davor ist ende bin mir aber wegen destruct nicht ganz sicher und optimierungen.

    } // testWatchdog

    inline static float maxExtruderTimerFrequency()
    {
        return (float)F_CPU/TIMER0_PRESCALE;

    } // maxExtruderTimerFrequency

#if FEATURE_SERVO && MOTHERBOARD == DEVICE_TYPE_RF1000
    static unsigned int     servoTimings[4];
    static void             servoMicroseconds(uint8_t servo,int ms);
#endif // FEATURE_SERVO && MOTHERBOARD == DEVICE_TYPE_RF1000

    static void analogStart();
#if USE_ADVANCE
    static void resetExtruderDirection();
#endif // USE_ADVANCE
protected:
private:
};

#define EXTRUDER_TIMER_VECTOR   TIMER0_COMPA_vect
#define EXTRUDER_OCR            OCR0A
#define EXTRUDER_TCCR           TCCR0A
#define EXTRUDER_TIMSK          TIMSK0
#define EXTRUDER_OCIE           OCIE0A
#define PWM_TIMER_VECTOR        TIMER0_COMPB_vect
#define PWM_OCR                 OCR0B
#define PWM_TCCR                TCCR0A
#define PWM_TIMSK               TIMSK0
#define PWM_OCIE                OCIE0B

#endif // HAL_H
