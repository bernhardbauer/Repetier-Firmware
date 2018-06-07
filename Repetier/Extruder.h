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


#ifndef EXTRUDER_H
#define EXTRUDER_H


#define CELSIUS_EXTRA_BITS 3


// Updates the temperature of all extruders and heated bed if it's time.
// Toggels the heater power if necessary.
extern bool     reportTempsensorError(); ///< Report defect sensors
extern uint8_t  manageMonitor;

#define TEMPERATURE_CONTROLLER_FLAG_ALARM  1
#define TEMPERATURE_CONTROLLER_FLAG_DEFECT 2
/** TemperatureController manages one heater-temperature sensore loop. You can have up to
4 loops allowing pid for up to 3 extruder and the heated bed.

*/
class TemperatureController
{
public:
    uint8_t     pwmIndex;               ///< pwm index for output control. 0-2 = Extruder, 3 = Fan, 4 = Heated Bed
    uint8_t     sensorType;             ///< Type of temperature sensor.
    uint8_t     sensorPin;              ///< Pin to read extruder temperature.
    int16_t     currentTemperature;     ///< Currenttemperature value read from sensor.
    float       currentTemperatureC;    ///< Current temperature in degC.
    float       targetTemperatureC;     ///< Target temperature in degC.

#if FEATURE_HEAT_BED_TEMP_COMPENSATION
    float       offsetC;                ///< offset in degC.
#endif // FEATURE_HEAT_BED_TEMP_COMPENSATION

    uint32_t    lastTemperatureUpdate;  ///< Time in millis of the last temperature update.

    float       tempIState;             ///< Temp. var. for PID computation.
    uint8_t     pidDriveMax;            ///< Used for windup in PID calculation.
    uint8_t     pidDriveMin;            ///< Used for windup in PID calculation.
    float       pidPGain;               ///< Pgain (proportional gain) for PID temperature control [0,01 Units].
    float       pidIGain;               ///< Igain (integral) for PID temperature control [0,01 Units].
    float       pidDGain;               ///< Dgain (damping) for PID temperature control [0,01 Units].
    uint8_t     pidMax;                 ///< Maximum PWM value, the heater should be set.
    float       tempIStateLimitMax;
    float       tempIStateLimitMin;
    uint8_t     tempPointer;
    float       tempArray[16];

    uint8_t     flags;

    void setTargetTemperature(float target, float offset);
    void updateCurrentTemperature();
    void updateTempControlVars();
    inline bool isAlarm() {return flags & TEMPERATURE_CONTROLLER_FLAG_ALARM;}
    inline void setAlarm(bool on) {if(on) flags |= TEMPERATURE_CONTROLLER_FLAG_ALARM; else flags &= ~TEMPERATURE_CONTROLLER_FLAG_ALARM;}
    inline bool isDefect() {return flags & TEMPERATURE_CONTROLLER_FLAG_DEFECT;}
    inline void setDefect(bool on) {if(on) flags |= TEMPERATURE_CONTROLLER_FLAG_DEFECT; else flags &= ~TEMPERATURE_CONTROLLER_FLAG_DEFECT;}
    void waitForTargetTemperature(uint8_t plus_temp_tolerance = 0);
    void autotunePID(float temp, uint8_t controllerId, int maxCycles, bool storeResult, int method);
}; // TemperatureController


class Extruder;
extern Extruder extruder[];

/** \brief Data to drive one extruder.

This structure contains all definitions for an extruder and all
current state variables, like current temperature, feeder position etc.
*/
class Extruder   // Size: 12*1 Byte+12*4 Byte+4*2Byte = 68 Byte
{
    public:
    static      Extruder*   current;

#if FEATURE_DITTO_PRINTING
    static      uint8_t     dittoMode;
#endif // FEATURE_DITTO_PRINTING

    uint8_t     id;
    int32_t     xOffset;
    int32_t     yOffset;
    int32_t     zOffset;
    float       stepsPerMM;                 ///< Steps per mm.
    int8_t      enablePin;                  ///< Pin to enable extruder stepper motor.
    uint8_t     enableOn;
    float       maxFeedrate;                ///< Maximum feedrate in mm/s.
    float       maxAcceleration;            ///< Maximum acceleration in mm/s^2.
    float       maxStartFeedrate;           ///< Maximum start feedrate in mm/s.
    int32_t     extrudePosition;            ///< Current extruder position in steps.
    int16_t     watchPeriod;                ///< Time in seconds, a M109 command will wait to stabalize temperature
    int16_t     waitRetractTemperature;     ///< Temperature to retract the filament when waiting for heatup
    int16_t     waitRetractUnits;           ///< Units to retract the filament when waiting for heatup
    volatile int8_t stepperDirection;

#if USE_ADVANCE
#ifdef ENABLE_QUADRATIC_ADVANCE
    float       advanceK;                   ///< Koefficient for advance algorithm. 0 = off
#endif // ENABLE_QUADRATIC_ADVANCE

    float       advanceL;
    int16_t     advanceBacklash;
#endif // USE_ADVANCE

    TemperatureController   tempControl;
    const char * PROGMEM    selectCommands;
    const char * PROGMEM    deselectCommands;
    uint8_t     coolerSpeed;                ///< Speed to use when enabled
    uint8_t     coolerPWM;                  ///< current PWM setting

#if STEPPER_ON_DELAY
    char        enabled;
#endif // STEPPER_ON_DELAY

#if FEATURE_PAUSE_PRINTING
    uint8_t     paused;
#endif // FEATURE_PAUSE_PRINTING

    /** \brief Sends the high-signal to the stepper for next extruder step.
    Call this function only, if interrupts are disabled.
    */
    static INLINE void step()
    {
#if NUM_EXTRUDER==1
        WRITE(EXT0_STEP_PIN,HIGH);
#else
        switch(Extruder::current->id)
        {
            case 0:
            {
#if NUM_EXTRUDER>0
                WRITE(EXT0_STEP_PIN,HIGH);

#if FEATURE_DITTO_PRINTING
                if(Extruder::dittoMode)
                {
                    WRITE(EXT1_STEP_PIN,HIGH);
                }
#endif // FEATURE_DITTO_PRINTING
#endif // NUM_EXTRUDER>0
                break;
            }

#if defined(EXT1_STEP_PIN) && NUM_EXTRUDER>1
            case 1:
            {
                WRITE(EXT1_STEP_PIN,HIGH);
                break;
            }
#endif // defined(EXT1_STEP_PIN) && NUM_EXTRUDER>1
        }
#endif
    } // step

    /** \brief Sets stepper signal to low for current extruder.
    Call this function only, if interrupts are disabled.
    */
    static INLINE void unstep()
    {
#if NUM_EXTRUDER==1
        WRITE(EXT0_STEP_PIN,LOW);
#else
        switch(Extruder::current->id)
        {
            case 0:
            {
#if NUM_EXTRUDER>0
                WRITE(EXT0_STEP_PIN,LOW);

#if FEATURE_DITTO_PRINTING
                if(Extruder::dittoMode)
                {
                    WRITE(EXT1_STEP_PIN,LOW);
                }
#endif // FEATURE_DITTO_PRINTING
#endif // NUM_EXTRUDER>0
                break;
            }

#if defined(EXT1_STEP_PIN) && NUM_EXTRUDER>1
            case 1:
            {
                WRITE(EXT1_STEP_PIN,LOW);
                break;
            }
#endif // defined(EXT1_STEP_PIN) && NUM_EXTRUDER>1
        }
#endif // NUM_EXTRUDER==1

    } // unstep


    /** \brief Activates the extruder stepper and sets the direction. */
    static inline void setDirection(uint8_t dir)
    {
#if NUM_EXTRUDER==1
        if(dir)
        {
            if( Extruder::current->stepperDirection != 1 )
            {
                WRITE(EXT0_DIR_PIN,!EXT0_INVERSE);
                Extruder::current->stepperDirection = 1;
            }
        }
        else
        {
            if( Extruder::current->stepperDirection != -1 )
            {
                WRITE(EXT0_DIR_PIN,EXT0_INVERSE);
                Extruder::current->stepperDirection = -1;
            }
        }
#else
        switch(Extruder::current->id)
        {
#if NUM_EXTRUDER>0
            case 0:
            {
                if(dir)
                    WRITE(EXT0_DIR_PIN,!EXT0_INVERSE);
                else
                    WRITE(EXT0_DIR_PIN,EXT0_INVERSE);

#if FEATURE_DITTO_PRINTING
                if(Extruder::dittoMode) {
                    if(dir)
                        WRITE(EXT1_DIR_PIN,!EXT1_INVERSE);
                    else
                        WRITE(EXT1_DIR_PIN,EXT1_INVERSE);
                }
#endif // FEATURE_DITTO_PRINTING
                break;
            }
#endif // NUM_EXTRUDER>0

#if defined(EXT1_DIR_PIN) && NUM_EXTRUDER>1
            case 1:
            {
                if(dir)
                    WRITE(EXT1_DIR_PIN,!EXT1_INVERSE);
                else
                    WRITE(EXT1_DIR_PIN,EXT1_INVERSE);
                break;
            }
#endif // defined(EXT1_DIR_PIN) && NUM_EXTRUDER>1
        }
#endif // NUM_EXTRUDER>0

    } // setDirection


    static inline void enable()
    {
#if NUM_EXTRUDER==1
#if EXT0_ENABLE_PIN>-1
        WRITE(EXT0_ENABLE_PIN,EXT0_ENABLE_ON );
#endif // EXT0_ENABLE_PIN>-1
#else
        if(Extruder::current->enablePin > -1)
            HAL::digitalWrite(Extruder::current->enablePin,Extruder::current->enableOn);

#if FEATURE_DITTO_PRINTING
        if(Extruder::dittoMode)
        {
            if(extruder[1].enablePin > -1)
                HAL::digitalWrite(extruder[1].enablePin,extruder[1].enableOn);
        }
#endif // FEATURE_DITTO_PRINTING
#endif // NUM_EXTRUDER==1

#if STEPPER_ON_DELAY
        if( !Extruder::current->enabled )
        {
            Extruder::current->enabled = 1;
            HAL::delayMilliseconds( STEPPER_ON_DELAY );
        }
#endif // STEPPER_ON_DELAY

    } // enable
    static void manageTemperatures();
    static void disableCurrentExtruderMotor();
    static void disableAllExtruders();
    static void selectExtruderById(uint8_t extruderId);
    static void disableAllHeater();
    static void initExtruder();
    static void initHeatedBed();
    static void setHeatedBedTemperature(float temperatureInCelsius,bool beep = false);
    static float getHeatedBedTemperature();
    static void setTemperatureForExtruder(float temperatureInCelsius,uint8_t extr,bool beep = false);
    static void setTemperatureForAllExtruders(float temperatureInCelsius, bool beep);

}; // Extruder


#if HAVE_HEATED_BED
#define NUM_TEMPERATURE_LOOPS NUM_EXTRUDER+1
extern TemperatureController heatedBedController;
#else
#define NUM_TEMPERATURE_LOOPS NUM_EXTRUDER
#endif // HAVE_HEATED_BED

#if RESERVE_ANALOG_INPUTS
extern TemperatureController optTempController;
#endif // RESERVE_ANALOG_INPUTS


#define TEMP_INT_TO_FLOAT(temp)     ((float)(temp)/(float)(1<<CELSIUS_EXTRA_BITS))
#define TEMP_FLOAT_TO_INT(temp)     ((int)((temp)*(1<<CELSIUS_EXTRA_BITS)))

extern TemperatureController *tempController[NUM_TEMPERATURE_LOOPS];
extern uint8_t autotuneIndex;


#endif // EXTRUDER_H
