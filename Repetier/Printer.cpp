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


#include "Repetier.h"
#include <Wire.h>

#if USE_ADVANCE
uint8_t         Printer::maxExtruderSpeed;                              ///< Timer delay for end extruder speed
volatile int    Printer::extruderStepsNeeded;                           ///< This many extruder steps are still needed, <0 = reverse steps needed.
#endif // USE_ADVANCE

uint8_t         Printer::unitIsInches = 0;                              ///< 0 = Units are mm, 1 = units are inches.

//Stepper Movement Variables
float           Printer::axisStepsPerMM[4] = {XAXIS_STEPS_PER_MM,YAXIS_STEPS_PER_MM,ZAXIS_STEPS_PER_MM,1};                   ///< Number of steps per mm needed.
float           Printer::invAxisStepsPerMM[4] = {1.0f/XAXIS_STEPS_PER_MM,1.0f/YAXIS_STEPS_PER_MM,1.0f/ZAXIS_STEPS_PER_MM,1}; ///< Inverse of axisStepsPerMM for faster conversion
float           Printer::maxFeedrate[4] = {MAX_FEEDRATE_X, MAX_FEEDRATE_Y, MAX_FEEDRATE_Z, DIRECT_FEEDRATE_E};               ///< Maximum allowed feedrate. //DIRECT_FEEDRATE_E added by nibbels, wird aber überschrieben.
float           Printer::homingFeedrate[3] = {HOMING_FEEDRATE_X_PRINT,HOMING_FEEDRATE_Y_PRINT,HOMING_FEEDRATE_Z_PRINT};      ///< dass zumindest etwas sinnvolles drin steht, wird überschrieben.

float           Printer::maxAccelerationMMPerSquareSecond[4] = {MAX_ACCELERATION_UNITS_PER_SQ_SECOND_X,MAX_ACCELERATION_UNITS_PER_SQ_SECOND_Y,MAX_ACCELERATION_UNITS_PER_SQ_SECOND_Z}; ///< X, Y, Z and E max acceleration in mm/s^2 for printing moves or retracts
float           Printer::maxTravelAccelerationMMPerSquareSecond[4] = {MAX_TRAVEL_ACCELERATION_UNITS_PER_SQ_SECOND_X,MAX_TRAVEL_ACCELERATION_UNITS_PER_SQ_SECOND_Y,MAX_TRAVEL_ACCELERATION_UNITS_PER_SQ_SECOND_Z}; ///< X, Y, Z max acceleration in mm/s^2 for travel moves
#if FEATURE_MILLING_MODE
short           Printer::max_milling_all_axis_acceleration = MILLER_ACCELERATION; //miller min speed is limited to too high speed because of acceleration-formula. We use this value in milling mode and make it adjustable within small numbers like 5 to 100 or something like that.
#endif // FEATURE_MILLING_MODE

/** Acceleration in steps/s^2 in printing mode.*/
unsigned long   Printer::maxPrintAccelerationStepsPerSquareSecond[4];
/** Acceleration in steps/s^2 in movement mode.*/
unsigned long   Printer::maxTravelAccelerationStepsPerSquareSecond[4];

uint8_t         Printer::relativeCoordinateMode = false;                ///< Determines absolute (false) or relative Coordinates (true).
uint8_t         Printer::relativeExtruderCoordinateMode = false;        ///< Determines Absolute or Relative E Codes while in Absolute Coordinates mode. E is always relative in Relative Coordinates mode.

volatile long   Printer::queuePositionLastSteps[4]               = {0,0,0,0};
volatile float  Printer::queuePositionLastMM[3]                  = {0,0,0};
volatile float  Printer::queuePositionCommandMM[3]               = {0,0,0};
volatile long   Printer::queuePositionTargetSteps[4]             = {0,0,0,0};
float           Printer::originOffsetMM[3] = {0,0,0};
uint8_t         Printer::flag0 = 0;
uint8_t         Printer::flag1 = 0;
uint8_t         Printer::flag2 = 0;
uint8_t         Printer::flag3 = 0;

#if ALLOW_EXTENDED_COMMUNICATION < 2
uint8_t         Printer::debugLevel = 0; ///< Bitfield defining debug output. 1 = echo, 2 = info, 4 = error, 8 = dry run., 16 = Only communication, 32 = No moves
#else
uint8_t         Printer::debugLevel = 6; ///< Bitfield defining debug output. 1 = echo, 2 = info, 4 = error, 8 = dry run., 16 = Only communication, 32 = No moves
#endif // ALLOW_EXTENDED_COMMUNICATION < 2

uint8_t         Printer::stepsPerTimerCall = 1;
uint16_t        Printer::stepsDoublerFrequency = STEP_DOUBLER_FREQUENCY;
uint8_t         Printer::menuMode = 0;

volatile unsigned long   Printer::interval;                             ///< Last step duration in ticks.
volatile float  Printer::v = 0;                                         ///< Last planned printer speed.
unsigned long   Printer::timer;                                         ///< used for acceleration/deceleration timing
unsigned long   Printer::stepNumber;                                    ///< Step number in current move.
#if FEATURE_DIGIT_FLOW_COMPENSATION
unsigned short  Printer::interval_mod = 0;                              ///< additional step duration in ticks to slow the printer down live
#endif // FEATURE_DIGIT_FLOW_COMPENSATION

#if USE_ADVANCE
#ifdef ENABLE_QUADRATIC_ADVANCE
long            Printer::advanceExecuted;                               ///< Executed advance steps
#endif // ENABLE_QUADRATIC_ADVANCE

volatile int    Printer::advanceStepsSet;
#endif // USE_ADVANCE

long            Printer::maxSteps[3] = {0};                             ///< For software endstops, limit of move in positive direction.
long            Printer::minSteps[3] = {0};                             ///< For software endstops, limit of move in negative direction.
float           Printer::lengthMM[3] = {0};                             ///< Maximale Achskoordinate soll
float           Printer::minMM[2] = {0};                                ///< Minimale Achskoordinate -> normal ist das 0, ausser geänderte config.
float           Printer::feedrate;                                      ///< Last requested feedrate.
int             Printer::feedrateMultiply = 1;                          ///< Multiplier for feedrate in percent (factor 1 = 100)
int             Printer::extrudeMultiply = 1;                           ///< Flow multiplier in percdent (factor 1 = 100)
float           Printer::extrudeMultiplyError = 0;
float           Printer::extrusionFactor = 1.0;
float           Printer::maxJerk;                                       ///< Maximum allowed jerk in mm/s
float           Printer::maxZJerk;                                      ///< Maximum allowed jerk in z direction in mm/s
float           Printer::extruderOffset[3] = {0};                       ///< offset for different extruder positions.
unsigned int    Printer::vMaxReached;                                   ///< Maximum reached speed
unsigned long   Printer::msecondsPrinting;                              ///< Milliseconds of printing time (means time with heated extruder)
unsigned long   Printer::msecondsMilling;                               ///< Milliseconds of milling time
float           Printer::filamentPrinted;                               ///< mm of filament printed since counting started
long            Printer::ZOffset;                                       ///< Z Offset in um
char            Printer::ZMode               = DEFAULT_Z_SCALE_MODE;    ///< Z Scale  1 = show the z-distance to z-min (print) or to the z-origin (mill), 2 = show the z-distance to the surface of the heat bed (print) or work part (mill)
char            Printer::moveMode[3];                                   ///< move mode which is applied within the Position X/Y/Z menus

#if ENABLE_BACKLASH_COMPENSATION
float           Printer::backlash[3];
uint8_t         Printer::backlashDir;
#endif // ENABLE_BACKLASH_COMPENSATION

#if FEATURE_MEMORY_POSITION
float           Printer::memoryX;
float           Printer::memoryY;
float           Printer::memoryZ;
float           Printer::memoryE;
float           Printer::memoryF;
#endif // FEATURE_MEMORY_POSITION

#if FEATURE_HEAT_BED_Z_COMPENSATION
volatile char   Printer::doHeatBedZCompensation = false;
#endif // FEATURE_HEAT_BED_Z_COMPENSATION

#if FEATURE_WORK_PART_Z_COMPENSATION
volatile char   Printer::doWorkPartZCompensation = false;
volatile long   Printer::staticCompensationZ = 0;
#endif // FEATURE_WORK_PART_Z_COMPENSATION

volatile long   Printer::queuePositionCurrentSteps[3] = {0, 0, 0};
volatile char   Printer::stepperDirection[3]          = {0, 0, 0};
volatile char   Printer::blockAll = 0;

volatile long   Printer::currentZSteps = 0;  //das ist der Z-Zähler der GCodes zum Zählen des tiefsten Schalterdruckpunkts /Schaltercrash.
uint16_t        Printer::ZOverrideMax  = uint16_t(ZAXIS_STEPS_PER_MM * Z_ENDSTOP_DRIVE_OVER);  //das ist der Z-Zähler der GCodes zum Zählen des tiefsten Schalterdruckpunkts /Schaltercrash.

#if FEATURE_HEAT_BED_Z_COMPENSATION || FEATURE_WORK_PART_Z_COMPENSATION
volatile long   Printer::compensatedPositionTargetStepsZ = 0;
volatile long   Printer::compensatedPositionCurrentStepsZ = 0;

volatile float  Printer::compensatedPositionOverPercE = 0.0f;
volatile float  Printer::compensatedPositionCollectTinyE = 0.0f;

volatile long   Printer::queuePositionZLayerCurrent_cand = 0;
volatile long   Printer::queuePositionZLayerCurrent = 0;
volatile long   Printer::queuePositionZLayerLast = 0;

volatile char   Printer::endZCompensationStep = 0;
#endif // FEATURE_HEAT_BED_Z_COMPENSATION || FEATURE_WORK_PART_Z_COMPENSATION

#if FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING
volatile long   Printer::directPositionTargetSteps[4]  = {0, 0, 0, 0};
volatile long   Printer::directPositionCurrentSteps[4] = {0, 0, 0, 0};
long            Printer::directPositionLastSteps[4]    = {0, 0, 0, 0};
char            Printer::waitMove;
#endif // FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING

#if FEATURE_MILLING_MODE
char            Printer::operatingMode;
float           Printer::drillFeedrate;
float           Printer::drillZDepth;
#endif // FEATURE_MILLING_MODE

#if FEATURE_CONFIGURABLE_Z_ENDSTOPS
char            Printer::ZEndstopType;
char            Printer::ZEndstopUnknown;
char            Printer::lastZDirection;
char            Printer::endstopZMinHit;
char            Printer::endstopZMaxHit;
#endif // FEATURE_CONFIGURABLE_Z_ENDSTOPS

#if FEATURE_CONFIGURABLE_MILLER_TYPE
char            Printer::MillerType;
#endif // FEATURE_CONFIGURABLE_MILLER_TYPE

#if STEPPER_ON_DELAY
char            Printer::enabledStepper[3] = {0, 0, 0};
#endif // STEPPER_ON_DELAY

#if FEATURE_BEEPER
char            Printer::enableBeeper;
#endif // FEATURE_BEEPER

#if FEATURE_CASE_LIGHT
char            Printer::enableCaseLight;
#endif // FEATURE_CASE_LIGHT

#if FEATURE_RGB_LIGHT_EFFECTS
char            Printer::RGBLightMode;
char            Printer::RGBLightStatus;
unsigned long   Printer::RGBLightIdleStart;
char            Printer::RGBButtonBackPressed;
char            Printer::RGBLightModeForceWhite;
#endif // FEATURE_RGB_LIGHT_EFFECTS

#if FEATURE_230V_OUTPUT
char            Printer::enable230VOutput;
#endif // FEATURE_230V_OUTPUT

#if FEATURE_24V_FET_OUTPUTS
char            Printer::enableFET1;
char            Printer::enableFET2;
char            Printer::enableFET3;
#endif // FEATURE_24V_FET_OUTPUTS

#if FEATURE_CASE_FAN
bool            Printer::ignoreFanOn = false;
unsigned long   Printer::prepareFanOff = 0;
unsigned long   Printer::fanOffDelay = 0;
#endif // FEATURE_CASE_FAN

#if FEATURE_TYPE_EEPROM
unsigned char   Printer::wrongType;
#endif // FEATURE_TYPE_EEPROM

#if FEATURE_UNLOCK_MOVEMENT
//When the printer resets, we should not do movement, because it would not be homed. At least some interaction with buttons or temperature-commands are needed to allow movement.
unsigned char   Printer::g_unlock_movement = 0;
#endif //FEATURE_UNLOCK_MOVEMENT

#if FEATURE_SENSIBLE_PRESSURE
bool            Printer::g_senseoffset_autostart = false;
#endif //FEATURE_SENSIBLE_PRESSURE

uint8_t         Printer::motorCurrent[DRV8711_NUM_CHANNELS] = {0};

#if FEATURE_ZERO_DIGITS
bool            Printer::g_pressure_offset_active = true;
short           Printer::g_pressure_offset = 0;
#endif // FEATURE_ZERO_DIGITS

#if FEATURE_ADJUSTABLE_MICROSTEPS
//RF_MICRO_STEPS_ have values 0=FULL 1=2MS, 2=4MS, 3=8MS, 4=16MS, 5=32MS, 6=64MS, 7=128MS, 8=256MS
uint8_t         Printer::motorMicroStepsModeValue[DRV8711_NUM_CHANNELS] = {0}; //init later because of recalculation of value
#endif // FEATURE_ADJUSTABLE_MICROSTEPS

#if FEATURE_Kurt67_WOBBLE_FIX

int8_t Printer::wobblePhaseXY = 0; //+100 = +PI | -100 = -PI
//int8_t Printer::wobblePhaseZ  = 0; //+100 = +PI | -100 = -PI
int16_t Printer::wobbleAmplitudes[3/*4*/] = {0}; //X, Y(X_0), Y(X_max), /*Z*/
float   Printer::wobblefixOffset[2/*3*/] = {0};                       ///< last calculated target wobbleFixOffsets for display output.

#endif // FEATURE_Kurt67_WOBBLE_FIX


void Printer::constrainQueueDestinationCoords()
{
    if(isNoDestinationCheck()) return;

#if FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING
#if max_software_endstop_x == true
    if (queuePositionTargetSteps[X_AXIS] + directPositionTargetSteps[X_AXIS] > Printer::maxSteps[X_AXIS]) Printer::queuePositionTargetSteps[X_AXIS] = Printer::maxSteps[X_AXIS] - directPositionTargetSteps[X_AXIS];
#endif // max_software_endstop_x == true

#if max_software_endstop_y == true
    if (queuePositionTargetSteps[Y_AXIS] + directPositionTargetSteps[Y_AXIS] > Printer::maxSteps[Y_AXIS]) Printer::queuePositionTargetSteps[Y_AXIS] = Printer::maxSteps[Y_AXIS] - directPositionTargetSteps[Y_AXIS];
#endif // max_software_endstop_y == true

#if max_software_endstop_z == true
    if (queuePositionTargetSteps[Z_AXIS] + directPositionTargetSteps[Z_AXIS] > Printer::maxSteps[Z_AXIS]) Printer::queuePositionTargetSteps[Z_AXIS] = Printer::maxSteps[Z_AXIS] - directPositionTargetSteps[Z_AXIS];
#endif // max_software_endstop_z == true
#else
#if max_software_endstop_x == true
    if (queuePositionTargetSteps[X_AXIS] > Printer::maxSteps[X_AXIS]) Printer::queuePositionTargetSteps[X_AXIS] = Printer::maxSteps[X_AXIS];
#endif // max_software_endstop_x == true

#if max_software_endstop_y == true
    if (queuePositionTargetSteps[Y_AXIS] > Printer::maxSteps[Y_AXIS]) Printer::queuePositionTargetSteps[Y_AXIS] = Printer::maxSteps[Y_AXIS];
#endif // max_software_endstop_y == true

#if max_software_endstop_z == true
    if (queuePositionTargetSteps[Z_AXIS] > Printer::maxSteps[Z_AXIS]) Printer::queuePositionTargetSteps[Z_AXIS] = Printer::maxSteps[Z_AXIS];
#endif // max_software_endstop_z == true
#endif //FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING

} // constrainQueueDestinationCoords


void Printer::constrainDirectDestinationCoords()
{
    if(isNoDestinationCheck()){
        return;
    }
    if(g_pauseStatus){
        return; 
        // the pause-and-continue functionality must calculate the constrains by itself
    }
#if FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING
#if max_software_endstop_x == true
    if (queuePositionTargetSteps[X_AXIS] + directPositionTargetSteps[X_AXIS] > Printer::maxSteps[X_AXIS]) Printer::directPositionTargetSteps[X_AXIS] = Printer::maxSteps[X_AXIS] - queuePositionTargetSteps[X_AXIS];
#endif // max_software_endstop_x == true

#if max_software_endstop_y == true
    if (queuePositionTargetSteps[Y_AXIS] + directPositionTargetSteps[Y_AXIS] > Printer::maxSteps[Y_AXIS]) Printer::directPositionTargetSteps[Y_AXIS] = Printer::maxSteps[Y_AXIS] - queuePositionTargetSteps[Y_AXIS];
#endif // max_software_endstop_y == true

#if max_software_endstop_z == true
    if (queuePositionTargetSteps[Z_AXIS] + directPositionTargetSteps[Z_AXIS] > Printer::maxSteps[Z_AXIS]) Printer::directPositionTargetSteps[Z_AXIS] = Printer::maxSteps[Z_AXIS] - queuePositionTargetSteps[Z_AXIS];
#endif // max_software_endstop_z == true
#endif //FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING
} // constrainDirectDestinationCoords

void Printer::updateDerivedParameter()
{
    maxSteps[X_AXIS] = (long)(axisStepsPerMM[X_AXIS]*(minMM[X_AXIS]+lengthMM[X_AXIS]));
    maxSteps[Y_AXIS] = (long)(axisStepsPerMM[Y_AXIS]*(minMM[Y_AXIS]+lengthMM[Y_AXIS]));
    maxSteps[Z_AXIS] = (long)(axisStepsPerMM[Z_AXIS]*lengthMM[Z_AXIS]);
    minSteps[X_AXIS] = (long)(axisStepsPerMM[X_AXIS]*minMM[X_AXIS]);
    minSteps[Y_AXIS] = (long)(axisStepsPerMM[Y_AXIS]*minMM[Y_AXIS]);
    minSteps[Z_AXIS] = (long)0;

    // For which directions do we need backlash compensation
#if ENABLE_BACKLASH_COMPENSATION
    backlashDir &= 7;
    if(backlashX!=0) backlashDir |= 8;
    if(backlashY!=0) backlashDir |= 16;
    if(backlashZ!=0) backlashDir |= 32;
#endif // ENABLE_BACKLASH_COMPENSATION

    for(uint8_t i = 0; i < 4; i++)
    {
        invAxisStepsPerMM[i] = 1.0f / axisStepsPerMM[i];

#if FEATURE_MILLING_MODE
        if( Printer::operatingMode == OPERATING_MODE_PRINT )
        {
#endif // FEATURE_MILLING_MODE
            /** Acceleration in steps/s^2 in printing mode.*/
            maxPrintAccelerationStepsPerSquareSecond[i] = maxAccelerationMMPerSquareSecond[i] * axisStepsPerMM[i];
            /** Acceleration in steps/s^2 in movement mode.*/
            maxTravelAccelerationStepsPerSquareSecond[i] = maxTravelAccelerationMMPerSquareSecond[i] * axisStepsPerMM[i];
#if FEATURE_MILLING_MODE
        }
        else
        {
            /** Acceleration in steps/s^2 in milling mode.*/
            maxPrintAccelerationStepsPerSquareSecond[i] = MILLER_ACCELERATION * axisStepsPerMM[i];
            /** Acceleration in steps/s^2 in milling-movement mode.*/
            maxTravelAccelerationStepsPerSquareSecond[i] = MILLER_ACCELERATION * axisStepsPerMM[i];
        }
#endif  // FEATURE_MILLING_MODE
    }

    // 07 11 2017 https://github.com/repetier/Repetier-Firmware/commit/e76875ec2d04bd0dbfdd9a157270ee03f4731d5f#diff-dbe11559ff43e09563388a4968911e40L1973
    // For numeric stability we need to start accelerations at a minimum speed and hence ensure that the
    // jerk is at least 2 * minimum speed.

    // For xy moves the minimum speed is multiplied with 1.41 to enforce the condition also for diagonals since the
    // driving axis is the problematic speed.

    float accel;
#if FEATURE_MILLING_MODE
    if( Printer::operatingMode == OPERATING_MODE_PRINT )
    {
#endif // FEATURE_MILLING_MODE
      accel = RMath::max(maxAccelerationMMPerSquareSecond[X_AXIS], maxTravelAccelerationMMPerSquareSecond[X_AXIS]);
#if FEATURE_MILLING_MODE
    }
    else{
      accel = MILLER_ACCELERATION;
    }
#endif  // FEATURE_MILLING_MODE
    
    float minimumSpeed = 1.41 * accel * sqrt(2.0f / (axisStepsPerMM[X_AXIS] * accel));
#if FEATURE_MILLING_MODE
    if( Printer::operatingMode == OPERATING_MODE_PRINT )
    {
#endif // FEATURE_MILLING_MODE
      accel = RMath::max(maxAccelerationMMPerSquareSecond[Y_AXIS], maxTravelAccelerationMMPerSquareSecond[Y_AXIS]);
#if FEATURE_MILLING_MODE
    }
    else{
      accel = MILLER_ACCELERATION;
    }
#endif  // FEATURE_MILLING_MODE    
    float minimumSpeed2 = 1.41 * accel * sqrt(2.0f / (axisStepsPerMM[Y_AXIS] * accel));
        if(minimumSpeed2 > minimumSpeed) {
            minimumSpeed = minimumSpeed2;
        }

    if(maxJerk < 2 * minimumSpeed) {// Enforce minimum start speed if target is faster and jerk too low
        maxJerk = 2 * minimumSpeed;
        Com::printFLN(PSTR("XY jerk was too low, setting to "), maxJerk);
    }

#if FEATURE_MILLING_MODE
    if( Printer::operatingMode == OPERATING_MODE_PRINT )
    {
#endif // FEATURE_MILLING_MODE
      accel = RMath::max(maxAccelerationMMPerSquareSecond[Z_AXIS], maxTravelAccelerationMMPerSquareSecond[Z_AXIS]);
#if FEATURE_MILLING_MODE
    }
#endif  // FEATURE_MILLING_MODE
    float minimumZSpeed = 0.5 * accel * sqrt(2.0f / (axisStepsPerMM[Z_AXIS] * accel));
    if(maxZJerk < 2 * minimumZSpeed) {
        maxZJerk = 2 * minimumZSpeed;
        Com::printFLN(PSTR("Z jerk was too low, setting to "), maxZJerk);
    }

    Printer::updateAdvanceFlags();
} // updateDerivedParameter


/** \brief Stop heater and stepper motors. Disable power,if possible. */
void Printer::kill(uint8_t only_steppers)
{
    if(areAllSteppersDisabled() && only_steppers) return;
    if(Printer::isAllKilled()) return;

#if FAN_PIN>-1 && FEATURE_FAN_CONTROL
    // disable the fan
    Commands::setFanSpeed(0);
#endif // FAN_PIN>-1 && FEATURE_FAN_CONTROL

    disableAllSteppersNow();

#if FAN_BOARD_PIN>-1
    pwm_pos[NUM_EXTRUDER+1] = 0;
#endif // FAN_BOARD_PIN

    if(!only_steppers)
    {
        Extruder::setTemperatureForAllExtruders(0, false);
        Extruder::setHeatedBedTemperature(0);
        UI_STATUS_UPD(UI_TEXT_KILLED);

#if defined(PS_ON_PIN) && PS_ON_PIN>-1
        //pinMode(PS_ON_PIN,INPUT);
        SET_OUTPUT(PS_ON_PIN); //GND
        WRITE(PS_ON_PIN, (POWER_INVERTING ? LOW : HIGH));
#endif // defined(PS_ON_PIN) && PS_ON_PIN>-1

        Printer::setAllKilled(true);
    }
    else
    {
        UI_STATUS_UPD(UI_TEXT_STEPPER_DISABLED);
    }

} // kill


void Printer::updateAdvanceFlags()
{
    Printer::setAdvanceActivated(false);
#if USE_ADVANCE
    for(uint8_t i = 0; i < NUM_EXTRUDER; i++) {
        if(extruder[i].advanceL != 0) {
            Printer::setAdvanceActivated(true);
        }
#ifdef ENABLE_QUADRATIC_ADVANCE
        if(extruder[i].advanceK != 0) Printer::setAdvanceActivated(true);
#endif // ENABLE_QUADRATIC_ADVANCE
    }
#endif // USE_ADVANCE
} // updateAdvanceFlags

/** Move to transformed Cartesian coordinates, mapping real (model) space to printer space.
*/
void Printer::moveToReal(float x,float y,float z,float e,float feedrate)
{
    if(x == IGNORE_COORDINATE)        x = queuePositionLastMM[X_AXIS];
    else queuePositionLastMM[X_AXIS] = x;
    if(y == IGNORE_COORDINATE)        y = queuePositionLastMM[Y_AXIS];
    else queuePositionLastMM[Y_AXIS] = y;
    if(z == IGNORE_COORDINATE)        z = queuePositionLastMM[Z_AXIS];
    else queuePositionLastMM[Z_AXIS] = z;

    x += Printer::extruderOffset[X_AXIS];
    y += Printer::extruderOffset[Y_AXIS];
    z += Printer::extruderOffset[Z_AXIS];

    queuePositionTargetSteps[X_AXIS] = static_cast<int32_t>(floor(x * axisStepsPerMM[X_AXIS] + 0.5));
    queuePositionTargetSteps[Y_AXIS] = static_cast<int32_t>(floor(y * axisStepsPerMM[Y_AXIS] + 0.5));
    queuePositionTargetSteps[Z_AXIS] = static_cast<int32_t>(floor(z * axisStepsPerMM[Z_AXIS] + 0.5));
    if(e != IGNORE_COORDINATE)
        queuePositionTargetSteps[E_AXIS] = e * axisStepsPerMM[E_AXIS];
    if(feedrate == IGNORE_COORDINATE) feedrate = Printer::feedrate;

    PrintLine::prepareQueueMove(ALWAYS_CHECK_ENDSTOPS, true, feedrate);

} // moveToReal


void Printer::setOrigin(float xOff,float yOff,float zOff)
{
    Com::printF( PSTR("setOrigin():") );
    if(isAxisHomed(X_AXIS)){
        originOffsetMM[X_AXIS] = xOff;
        Com::printF( PSTR("x="), originOffsetMM[X_AXIS] );
    }else{
        Com::printF( PSTR("x-fail") );
    }
    if(isAxisHomed(Y_AXIS)){
        originOffsetMM[Y_AXIS] = yOff;
        Com::printF( PSTR(" y="), originOffsetMM[Y_AXIS] );
    }else{
        Com::printF( PSTR(" y-fail") );
    }
    if(isAxisHomed(Z_AXIS)){
        originOffsetMM[Z_AXIS] = zOff;
        Com::printF( PSTR(" z="), originOffsetMM[Z_AXIS] );
    }else{
        Com::printF( PSTR(" z-fail") );
    }
    Com::println();
    
    if( !areAxisHomed() )
    {
        // we can not set the origin when we do not know the home position
        Com::printFLN( PSTR("WARNING: home positions were unknown and ignored!") );
        showError( (void*)ui_text_set_origin, (void*)ui_text_home_unknown );
    }
} // setOrigin


void Printer::updateCurrentPosition(bool copyLastCmd)
{
    queuePositionLastMM[X_AXIS] = (float)(queuePositionLastSteps[X_AXIS])*invAxisStepsPerMM[X_AXIS];
    queuePositionLastMM[Y_AXIS] = (float)(queuePositionLastSteps[Y_AXIS])*invAxisStepsPerMM[Y_AXIS];
    queuePositionLastMM[Z_AXIS] = (float)(queuePositionLastSteps[Z_AXIS])*invAxisStepsPerMM[Z_AXIS];
    queuePositionLastMM[X_AXIS] -= Printer::extruderOffset[X_AXIS];
    queuePositionLastMM[Y_AXIS] -= Printer::extruderOffset[Y_AXIS];
    queuePositionLastMM[Z_AXIS] -= Printer::extruderOffset[Z_AXIS];

    if(copyLastCmd)
    {
        queuePositionCommandMM[X_AXIS] = queuePositionLastMM[X_AXIS];
        queuePositionCommandMM[Y_AXIS] = queuePositionLastMM[Y_AXIS];
        queuePositionCommandMM[Z_AXIS] = queuePositionLastMM[Z_AXIS];
    }
} // updateCurrentPosition


/**
  \brief Sets the destination coordinates to values stored in com.

  For the computation of the destination, the following facts are considered:
  - Are units inches or mm.
  - Relative or absolute positioning with special case only extruder relative.
  - Offset in x and y direction for multiple extruder support.
*/
uint8_t Printer::setDestinationStepsFromGCode(GCode *com)
{
    register int32_t p;
    float           x, y, z;

    if(!relativeCoordinateMode)
    {
        if(com->hasX()) queuePositionCommandMM[X_AXIS] = queuePositionLastMM[X_AXIS] = convertToMM(com->X) - originOffsetMM[X_AXIS];
        if(com->hasY()) queuePositionCommandMM[Y_AXIS] = queuePositionLastMM[Y_AXIS] = convertToMM(com->Y) - originOffsetMM[Y_AXIS];
        if(com->hasZ()) queuePositionCommandMM[Z_AXIS] = queuePositionLastMM[Z_AXIS] = convertToMM(com->Z) - originOffsetMM[Z_AXIS];
    }
    else
    {
        if(com->hasX()) queuePositionLastMM[X_AXIS] = (queuePositionCommandMM[X_AXIS] += convertToMM(com->X));
        if(com->hasY()) queuePositionLastMM[Y_AXIS] = (queuePositionCommandMM[Y_AXIS] += convertToMM(com->Y));
        if(com->hasZ()) queuePositionLastMM[Z_AXIS] = (queuePositionCommandMM[Z_AXIS] += convertToMM(com->Z));
    }

    x = queuePositionCommandMM[X_AXIS] + Printer::extruderOffset[X_AXIS];
    y = queuePositionCommandMM[Y_AXIS] + Printer::extruderOffset[Y_AXIS];
    z = queuePositionCommandMM[Z_AXIS] + Printer::extruderOffset[Z_AXIS];

#if FEATURE_Kurt67_WOBBLE_FIX

    /*
    Zielkoordinate in Z ist: "z"
    Wir fügen beim Umrechnen in Steps vorher noch ein Offset in XYZ ein.
    offsetZ = amplitude des hubs                     * drehposition spindel
    offsetY = amplitude in Richtung Y linke Spindel  * drehposition spindel
    offsetY = amplitude in Richtung Y rechte Spindel * drehposition spindel
    offsetX = amplitude in Richtung X                * drehposition spindel

    "sinOffset = amplitude * sin( 2*Pi*(Z/5mm + zStartOffset) );"
    */
    
    //vorausgerechnete konstanten.
    float zweiPi              = 6.2832f;   //2*Pi
    float hundertstelPi       = 0.031428f; //Pi/100
    float spindelSteigung     = 5.0f;      //[mm]
    
    //wobble durch Bauchtanz der Druckplatte
    //wobbleX oder wobbleY(x0) oder wobbleX(x245)
    if(Printer::wobbleAmplitudes[0]){
        float anglePositionWobble = cos(zweiPi*(z/spindelSteigung - (float)Printer::wobblePhaseXY * hundertstelPi ));
        //für das wobble in Richtung X-Achse gilt immer dieselbe Amplitude, weil im extremfall beide gegeneinander arbeiten und sich aufheben könnten, oder die Spindeln arbeiten zusammen.
        Printer::wobblefixOffset[X_AXIS] = Printer::wobbleAmplitudes[0] * anglePositionWobble;
        x += Printer::wobblefixOffset[X_AXIS]/1000;  //offset in [um] -> kosys in [mm]
    }
    if(Printer::wobbleAmplitudes[1] || Printer::wobbleAmplitudes[2]){
        float anglePositionWobble = sin(zweiPi*(z/spindelSteigung - (float)Printer::wobblePhaseXY * hundertstelPi ));
        //gilt eher die Y-Achsen-Richtung-Amplitude links (x=0) oder rechts (x=achsenlänge)? (abhängig von der ziel-x-position wird anteilig verrechnet.)
        float xPosPercent = x/Printer::lengthMM[X_AXIS];
        Printer::wobblefixOffset[Y_AXIS] = ((1-xPosPercent) * Printer::wobbleAmplitudes[1] + (xPosPercent) * Printer::wobbleAmplitudes[2]) * anglePositionWobble;
        y += Printer::wobblefixOffset[Y_AXIS]/1000;  //offset in [um] -> kosys in [mm]
    }
/*
    //wobble durch Kippeln mit Hebel -> Z-Hub
    //wobbleZ
    if(Printer::wobbleAmplitudes[3]){
        float anglePositionLift   = sin(zweiPi*(z/spindelSteigung + (float)Printer::wobblePhaseZ  * hundertstelPi )); //phase von +-100% -> +-Pi
        Printer::wobblefixOffset[Z_AXIS] = Printer::wobbleAmplitudes[3] * anglePositionLift;
        //z kippel-wobble nur mit etwas Abstand überhalb senseoffset ausgleichen. Drunter könnte das stören.
        if(z > fabs(Printer::wobblefixOffset[Z_AXIS])+AUTOADJUST_STARTMADEN_AUSSCHLUSS){
            z += Printer::wobblefixOffset[Z_AXIS]/1000;  //offset in [um] -> kosys in [mm]
        }
    }
*/
#endif // FEATURE_Kurt67_WOBBLE_FIX

    long xSteps = static_cast<long>(floor(x * axisStepsPerMM[X_AXIS] + 0.5f));
    long ySteps = static_cast<long>(floor(y * axisStepsPerMM[Y_AXIS] + 0.5f));
    long zSteps = static_cast<long>(floor(z * axisStepsPerMM[Z_AXIS] + 0.5f));
    
    if(com->hasX())
    {
        queuePositionTargetSteps[X_AXIS] = xSteps;
    }
    else
    {
        queuePositionTargetSteps[X_AXIS] = queuePositionLastSteps[X_AXIS];
    }

    if(com->hasY())
    {
        queuePositionTargetSteps[Y_AXIS] = ySteps;
    }
    else
    {
        queuePositionTargetSteps[Y_AXIS] = queuePositionLastSteps[Y_AXIS];
    }

    if(com->hasZ())
    {
        queuePositionTargetSteps[Z_AXIS] = zSteps;
    }
    else
    {
        queuePositionTargetSteps[Z_AXIS] = queuePositionLastSteps[Z_AXIS];
    }

    if(com->hasE() && !Printer::debugDryrun())
    {
        if(relativeCoordinateMode || relativeExtruderCoordinateMode)
        {
            if(
#if MIN_EXTRUDER_TEMP > 30
                Extruder::current->tempControl.currentTemperatureC < MIN_EXTRUDER_TEMP ||
#endif // MIN_EXTRUDER_TEMP > 30
                fabs(com->E) * extrusionFactor
 #if FEATURE_DIGIT_FLOW_COMPENSATION
                            * g_nDigitFlowCompensation_flowmulti
 #endif // FEATURE_DIGIT_FLOW_COMPENSATION
                > EXTRUDE_MAXLENGTH)
            {
                queuePositionTargetSteps[E_AXIS] = queuePositionLastSteps[E_AXIS]; //p = 0;
            }else{
                Printer::extrudeMultiplyError += convertToMM(com->E * axisStepsPerMM[E_AXIS]);
                p = static_cast<int32_t>(Printer::extrudeMultiplyError);
                Printer::extrudeMultiplyError -= p;
                queuePositionTargetSteps[E_AXIS] = queuePositionLastSteps[E_AXIS] + p;
            }
        }
        else
        {
            p = convertToMM(com->E * axisStepsPerMM[E_AXIS]);
            if(
#if MIN_EXTRUDER_TEMP > 30
                Extruder::current->tempControl.currentTemperatureC < MIN_EXTRUDER_TEMP ||
#endif // MIN_EXTRUDER_TEMP > 30
                fabs(p - queuePositionLastSteps[E_AXIS]) * extrusionFactor
 #if FEATURE_DIGIT_FLOW_COMPENSATION
                            * g_nDigitFlowCompensation_flowmulti
 #endif // FEATURE_DIGIT_FLOW_COMPENSATION
                > EXTRUDE_MAXLENGTH * axisStepsPerMM[E_AXIS])
                    {
                        queuePositionLastSteps[E_AXIS] = p;
                    }
            queuePositionTargetSteps[E_AXIS] = p;
        }
    }
    else
    {
        queuePositionTargetSteps[E_AXIS] = queuePositionLastSteps[E_AXIS];
    }

    if(com->hasF())
    {
        if(unitIsInches)
            Printer::feedrate = com->F * (float)feedrateMultiply * 0.0042333f;  // Factor is 25.5/60/100
        else
            Printer::feedrate = com->F * (float)feedrateMultiply * 0.00016666666f;
    }

    return !com->hasNoXYZ() || (com->hasE() && queuePositionTargetSteps[E_AXIS] != queuePositionLastSteps[E_AXIS]); // ignore unproductive moves

} // setDestinationStepsFromGCode


/**
  \brief Sets the destination coordinates to the passed values.
*/
uint8_t Printer::setDestinationStepsFromMenu( float relativeX, float relativeY, float relativeZ )
{
    float   x, y, z;


    if( relativeX ) queuePositionLastMM[X_AXIS] = (queuePositionCommandMM[X_AXIS] += relativeX);
    if( relativeY ) queuePositionLastMM[Y_AXIS] = (queuePositionCommandMM[Y_AXIS] += relativeY);
    if( relativeZ ) queuePositionLastMM[Z_AXIS] = (queuePositionCommandMM[Z_AXIS] += relativeZ);

    x = queuePositionCommandMM[X_AXIS] + Printer::extruderOffset[X_AXIS];
    y = queuePositionCommandMM[Y_AXIS] + Printer::extruderOffset[Y_AXIS];
    z = queuePositionCommandMM[Z_AXIS] + Printer::extruderOffset[Z_AXIS];

    long xSteps = static_cast<long>(floor(x * axisStepsPerMM[X_AXIS] + 0.5));
    long ySteps = static_cast<long>(floor(y * axisStepsPerMM[Y_AXIS] + 0.5));
    long zSteps = static_cast<long>(floor(z * axisStepsPerMM[Z_AXIS] + 0.5));
    
    if( relativeX )
    {
        queuePositionTargetSteps[X_AXIS] = xSteps;
    }
    else
    {
        queuePositionTargetSteps[X_AXIS] = queuePositionLastSteps[X_AXIS];
    }

    if( relativeY )
    {
        queuePositionTargetSteps[Y_AXIS] = ySteps;
    }
    else
    {
        queuePositionTargetSteps[Y_AXIS] = queuePositionLastSteps[Y_AXIS];
    }

    if( relativeZ )
    {
        queuePositionTargetSteps[Z_AXIS] = zSteps;
    }
    else
    {
        queuePositionTargetSteps[Z_AXIS] = queuePositionLastSteps[Z_AXIS];
    }

    PrintLine::prepareQueueMove(ALWAYS_CHECK_ENDSTOPS, true, Printer::feedrate);
    return true;

} // setDestinationStepsFromMenu


void Printer::setup()
{
    HAL::stopWatchdog();

    for(uint8_t i=0; i<NUM_EXTRUDER+3; i++) pwm_pos[i]=0;

#if FEATURE_USER_INT3
    SET_INPUT( RESERVE_DIGITAL_PIN_PD3 );
    PULLUP( RESERVE_DIGITAL_PIN_PD3, HIGH );
#endif //FEATURE_USER_INT3

#if FEATURE_READ_CALIPER
// read for using pins : https://www.arduino.cc/en/Tutorial/DigitalPins
//where the clock comes in and triggers an interrupt which reads data then:
    SET_INPUT( FEATURE_READ_CALIPER_INT_PIN ); //input as default already this is here for explaination more than really having an input.
    PULLUP( FEATURE_READ_CALIPER_INT_PIN, HIGH ); //do I need this pullup??
//where data is to read when Int triggers because of clock from caliper:
    SET_INPUT( FEATURE_READ_CALIPER_DATA_PIN ); //input as default already this is here for explaination more than really having an input.
    PULLUP( FEATURE_READ_CALIPER_DATA_PIN, HIGH ); //do I need this pullup??
#endif //FEATURE_READ_CALIPER

    HAL::allowInterrupts();

#if FEATURE_USER_INT3
    attachInterrupt( digitalPinToInterrupt(RESERVE_DIGITAL_PIN_PD3) , USER_INTERRUPT3_HOOK, FALLING );
#endif //FEATURE_USER_INT3

#if FEATURE_READ_CALIPER
    attachInterrupt( digitalPinToInterrupt(FEATURE_READ_CALIPER_INT_PIN) , FEATURE_READ_CALIPER_HOOK, FALLING );
#endif //FEATURE_READ_CALIPER

#if FEATURE_BEEPER
    enableBeeper = BEEPER_MODE;
#endif // FEATURE_BEEPER

    Wire.begin();

#if FEATURE_TYPE_EEPROM
    determineHardwareType();

    if( wrongType )
    {
        // this firmware is not for this hardware
        while( 1 )
        {
            // this firmware shall not try to do anything at this hardware
        }
    }
#endif // FEATURE_TYPE_EEPROM

#if defined(ENABLE_POWER_ON_STARTUP) && PS_ON_PIN>-1
    SET_OUTPUT(PS_ON_PIN); //GND
    WRITE(PS_ON_PIN, (POWER_INVERTING ? HIGH : LOW));
#endif // defined(ENABLE_POWER_ON_STARTUP) && PS_ON_PIN>-1

    //Initialize Step Pins
    SET_OUTPUT(X_STEP_PIN);
    SET_OUTPUT(Y_STEP_PIN);
    SET_OUTPUT(Z_STEP_PIN);

    //Initialize Dir Pins
#if X_DIR_PIN>-1
    SET_OUTPUT(X_DIR_PIN);
#endif // X_DIR_PIN>-1

#if Y_DIR_PIN>-1
    SET_OUTPUT(Y_DIR_PIN);
#endif // Y_DIR_PIN>-1

#if Z_DIR_PIN>-1
    SET_OUTPUT(Z_DIR_PIN);
#endif // Z_DIR_PIN>-1

    //Steppers default to disabled.
#if X_ENABLE_PIN > -1
    SET_OUTPUT(X_ENABLE_PIN);
    if(!X_ENABLE_ON) WRITE(X_ENABLE_PIN,HIGH);
    disableXStepper();
#endif // X_ENABLE_PIN > -1

#if Y_ENABLE_PIN > -1
    SET_OUTPUT(Y_ENABLE_PIN);
    if(!Y_ENABLE_ON) WRITE(Y_ENABLE_PIN,HIGH);
    disableYStepper();
#endif // Y_ENABLE_PIN > -1

#if Z_ENABLE_PIN > -1
    SET_OUTPUT(Z_ENABLE_PIN);
    if(!Z_ENABLE_ON) WRITE(Z_ENABLE_PIN,HIGH);
    disableZStepper();
#endif // Z_ENABLE_PIN > -1

#if FEATURE_TWO_XSTEPPER
    SET_OUTPUT(X2_STEP_PIN);
    SET_OUTPUT(X2_DIR_PIN);

#if X2_ENABLE_PIN > -1
    SET_OUTPUT(X2_ENABLE_PIN);
    if(!X_ENABLE_ON) WRITE(X2_ENABLE_PIN,HIGH);
#endif // X2_ENABLE_PIN > -1
#endif // FEATURE_TWO_XSTEPPER

#if FEATURE_TWO_YSTEPPER
    SET_OUTPUT(Y2_STEP_PIN);
    SET_OUTPUT(Y2_DIR_PIN);

#if Y2_ENABLE_PIN > -1
    SET_OUTPUT(Y2_ENABLE_PIN);
    if(!Y_ENABLE_ON) WRITE(Y2_ENABLE_PIN,HIGH);
#endif // Y2_ENABLE_PIN > -1
#endif // FEATURE_TWO_YSTEPPER

#if FEATURE_TWO_ZSTEPPER
    SET_OUTPUT(Z2_STEP_PIN);
    SET_OUTPUT(Z2_DIR_PIN);

#if X2_ENABLE_PIN > -1
    SET_OUTPUT(Z2_ENABLE_PIN);
    if(!Z_ENABLE_ON) WRITE(Z2_ENABLE_PIN,HIGH);
#endif // X2_ENABLE_PIN > -1
#endif // FEATURE_TWO_ZSTEPPER

    //endstop pullups
#if MIN_HARDWARE_ENDSTOP_X
#if X_MIN_PIN>-1
    SET_INPUT(X_MIN_PIN);

#if ENDSTOP_PULLUP_X_MIN
    PULLUP(X_MIN_PIN,HIGH);
#endif // ENDSTOP_PULLUP_X_MIN
#else
#error You have defined hardware x min endstop without pin assignment. Set pin number for X_MIN_PIN
#endif // X_MIN_PIN>-1
#endif // MIN_HARDWARE_ENDSTOP_X

#if MIN_HARDWARE_ENDSTOP_Y
#if Y_MIN_PIN>-1
    SET_INPUT(Y_MIN_PIN);

#if ENDSTOP_PULLUP_Y_MIN
    PULLUP(Y_MIN_PIN,HIGH);
#endif // ENDSTOP_PULLUP_Y_MIN
#else
#error You have defined hardware y min endstop without pin assignment. Set pin number for Y_MIN_PIN
#endif // Y_MIN_PIN>-1
#endif // MIN_HARDWARE_ENDSTOP_Y

#if MIN_HARDWARE_ENDSTOP_Z
#if Z_MIN_PIN>-1
    SET_INPUT(Z_MIN_PIN);

#if ENDSTOP_PULLUP_Z_MIN
    PULLUP(Z_MIN_PIN,HIGH);
#endif // ENDSTOP_PULLUP_Z_MIN
#else
#error You have defined hardware z min endstop without pin assignment. Set pin number for Z_MIN_PIN
#endif // Z_MIN_PIN>-1
#endif // MIN_HARDWARE_ENDSTOP_Z

#if MAX_HARDWARE_ENDSTOP_X
#if X_MAX_PIN>-1
    SET_INPUT(X_MAX_PIN);

#if ENDSTOP_PULLUP_X_MAX
    PULLUP(X_MAX_PIN,HIGH);
#endif // ENDSTOP_PULLUP_X_MAX
#else
#error You have defined hardware x max endstop without pin assignment. Set pin number for X_MAX_PIN
#endif // X_MAX_PIN>-1
#endif // MAX_HARDWARE_ENDSTOP_X

#if MAX_HARDWARE_ENDSTOP_Y
#if Y_MAX_PIN>-1
    SET_INPUT(Y_MAX_PIN);

#if ENDSTOP_PULLUP_Y_MAX
    PULLUP(Y_MAX_PIN,HIGH);
#endif // ENDSTOP_PULLUP_Y_MAX
#else
#error You have defined hardware y max endstop without pin assignment. Set pin number for Y_MAX_PIN
#endif // Y_MAX_PIN>-1
#endif // MAX_HARDWARE_ENDSTOP_Y

#if MAX_HARDWARE_ENDSTOP_Z
#if Z_MAX_PIN>-1
    SET_INPUT(Z_MAX_PIN);

#if ENDSTOP_PULLUP_Z_MAX
    PULLUP(Z_MAX_PIN,HIGH);
#endif // ENDSTOP_PULLUP_Z_MAX
#else
#error You have defined hardware z max endstop without pin assignment. Set pin number for Z_MAX_PIN
#endif // Z_MAX_PIN>-1
#endif // MAX_HARDWARE_ENDSTOP_Z

#if FAN_PIN>-1 && FEATURE_FAN_CONTROL
    SET_OUTPUT(FAN_PIN);
    WRITE(FAN_PIN,LOW);
#endif // FAN_PIN>-1 && FEATURE_FAN_CONTROL

#if FAN_BOARD_PIN>-1
    SET_OUTPUT(FAN_BOARD_PIN);
    WRITE(FAN_BOARD_PIN,LOW);
#endif // FAN_BOARD_PIN>-1

#if EXT0_HEATER_PIN>-1
    SET_OUTPUT(EXT0_HEATER_PIN);
    WRITE(EXT0_HEATER_PIN,HEATER_PINS_INVERTED);
#endif // EXT0_HEATER_PIN>-1

#if defined(EXT1_HEATER_PIN) && EXT1_HEATER_PIN>-1 && NUM_EXTRUDER>1
    SET_OUTPUT(EXT1_HEATER_PIN);
    WRITE(EXT1_HEATER_PIN,HEATER_PINS_INVERTED);
#endif // defined(EXT1_HEATER_PIN) && EXT1_HEATER_PIN>-1 && NUM_EXTRUDER>1

#if EXT0_EXTRUDER_COOLER_PIN>-1
    SET_OUTPUT(EXT0_EXTRUDER_COOLER_PIN);
    WRITE(EXT0_EXTRUDER_COOLER_PIN,LOW);
#endif // EXT0_EXTRUDER_COOLER_PIN>-1

#if defined(EXT1_EXTRUDER_COOLER_PIN) && EXT1_EXTRUDER_COOLER_PIN>-1 && NUM_EXTRUDER>1
    SET_OUTPUT(EXT1_EXTRUDER_COOLER_PIN);
    WRITE(EXT1_EXTRUDER_COOLER_PIN,LOW);
#endif // defined(EXT1_EXTRUDER_COOLER_PIN) && EXT1_EXTRUDER_COOLER_PIN>-1 && NUM_EXTRUDER>1

    feedrate = 50; ///< Current feedrate in mm/s.
    feedrateMultiply = 100;
    extrudeMultiply = 100;
    queuePositionCommandMM[X_AXIS] = queuePositionCommandMM[Y_AXIS] = queuePositionCommandMM[Z_AXIS] = 0;

#if USE_ADVANCE
#ifdef ENABLE_QUADRATIC_ADVANCE
    advanceExecuted = 0;
#endif // ENABLE_QUADRATIC_ADVANCE
    advanceStepsSet = 0;
#endif // USE_ADVANCE

    queuePositionLastSteps[X_AXIS] = queuePositionLastSteps[Y_AXIS] = queuePositionLastSteps[Z_AXIS] = queuePositionLastSteps[E_AXIS] = 0;

#if FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING
    directPositionTargetSteps[X_AXIS]  = directPositionTargetSteps[Y_AXIS]  = directPositionTargetSteps[Z_AXIS]  = directPositionTargetSteps[E_AXIS]  = 0;
    directPositionCurrentSteps[X_AXIS] = directPositionCurrentSteps[Y_AXIS] = directPositionCurrentSteps[Z_AXIS] = directPositionCurrentSteps[E_AXIS] = 0;
    directPositionLastSteps[X_AXIS]    = directPositionLastSteps[Y_AXIS]    = directPositionLastSteps[Z_AXIS]    = directPositionLastSteps[E_AXIS]    = 0;
    waitMove                           = 0;
#endif // FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING

    maxJerk = MAX_JERK;
    maxZJerk = MAX_ZJERK;
    extruderOffset[X_AXIS] = extruderOffset[Y_AXIS] = extruderOffset[Z_AXIS] = 0;

    ZOffset = 0;
    interval = 5000;
    stepsPerTimerCall = 1;
    msecondsPrinting = 0;
    msecondsMilling = 0;
    filamentPrinted = 0;
    flag0 = PRINTER_FLAG0_STEPPER_DISABLED;

    moveMode[X_AXIS] = DEFAULT_MOVE_MODE_X;
    moveMode[Y_AXIS] = DEFAULT_MOVE_MODE_Y;
    moveMode[Z_AXIS] = DEFAULT_MOVE_MODE_Z;

#if ENABLE_BACKLASH_COMPENSATION
    backlash[X_AXIS] = X_BACKLASH;
    backlash[Y_AXIS] = Y_BACKLASH;
    backlash[Z_AXIS] = Z_BACKLASH;
    backlashDir = 0;
#endif // ENABLE_BACKLASH_COMPENSATION

#if FEATURE_HEAT_BED_Z_COMPENSATION
    doHeatBedZCompensation = 0; //init
#endif // FEATURE_HEAT_BED_Z_COMPENSATION

#if FEATURE_WORK_PART_Z_COMPENSATION
    doWorkPartZCompensation = 0; //init
    staticCompensationZ     = 0; //init
#endif // FEATURE_WORK_PART_Z_COMPENSATION

    queuePositionCurrentSteps[X_AXIS] = 
    queuePositionCurrentSteps[Y_AXIS] = 
    queuePositionCurrentSteps[Z_AXIS] = 0;
    blockAll                          = 0;

    currentZSteps                     = 0;

#if FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING
    directPositionCurrentSteps[X_AXIS] =
    directPositionCurrentSteps[Y_AXIS] =
    directPositionCurrentSteps[Z_AXIS] =
    directPositionCurrentSteps[E_AXIS] =
    directPositionTargetSteps[X_AXIS]  =
    directPositionTargetSteps[Y_AXIS]  =
    directPositionTargetSteps[Z_AXIS]  =
    directPositionTargetSteps[E_AXIS]  = 0;
#endif // FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING

#if FEATURE_MILLING_MODE
    operatingMode = DEFAULT_OPERATING_MODE;
    drillFeedrate = 0.0;
    drillZDepth   = 0.0;
#endif // FEATURE_MILLING_MODE

#if FEATURE_MILLING_MODE
    if( Printer::operatingMode == OPERATING_MODE_PRINT )
    {
#endif // FEATURE_MILLING_MODE
        Printer::homingFeedrate[X_AXIS] = HOMING_FEEDRATE_X_PRINT;
        Printer::homingFeedrate[Y_AXIS] = HOMING_FEEDRATE_Y_PRINT;
        Printer::homingFeedrate[Z_AXIS] = HOMING_FEEDRATE_Z_PRINT;
        lengthMM[X_AXIS] = X_MAX_LENGTH_PRINT;
#if FEATURE_MILLING_MODE
    }
    else
    {
        Printer::homingFeedrate[X_AXIS] = HOMING_FEEDRATE_X_MILL;
        Printer::homingFeedrate[Y_AXIS] = HOMING_FEEDRATE_Y_MILL;
        Printer::homingFeedrate[Z_AXIS] = HOMING_FEEDRATE_Z_MILL;
        lengthMM[X_AXIS] = X_MAX_LENGTH_MILL;
    }
#endif // FEATURE_MILLING_MODE

    lengthMM[Y_AXIS] = Y_MAX_LENGTH;
    lengthMM[Z_AXIS] = Z_MAX_LENGTH;
    minMM[X_AXIS] = X_MIN_POS;
    minMM[Y_AXIS] = Y_MIN_POS;

#if FEATURE_CONFIGURABLE_Z_ENDSTOPS
    ZEndstopType          = DEFAULT_Z_ENDSTOP_TYPE;
    ZEndstopUnknown       = 0;
    lastZDirection        = 0;
    endstopZMinHit        = ENDSTOP_NOT_HIT;
    endstopZMaxHit        = ENDSTOP_NOT_HIT;
#endif // FEATURE_CONFIGURABLE_Z_ENDSTOPS


#if STEPPER_ON_DELAY
    enabledStepper[X_AXIS] = 0;
    enabledStepper[Y_AXIS] = 0;
    enabledStepper[Z_AXIS] = 0;
#endif // STEPPER_ON_DELAY

#if FEATURE_CASE_LIGHT
    enableCaseLight = CASE_LIGHTS_DEFAULT_ON;
#endif // FEATURE_CASE_LIGHT

#if FEATURE_24V_FET_OUTPUTS
    enableFET1 = FET1_DEFAULT_ON;
    enableFET2 = FET2_DEFAULT_ON;
    enableFET3 = FET3_DEFAULT_ON;

    SET_OUTPUT(FET1);
    WRITE(FET1, enableFET1);
    
    SET_OUTPUT(FET2);
    WRITE(FET2, enableFET2);
    
    SET_OUTPUT(FET3);
    WRITE(FET3, enableFET3);
#endif // FEATURE_24V_FET_OUTPUTS

#if FEATURE_CASE_FAN
    fanOffDelay = CASE_FAN_OFF_DELAY;
#endif // FEATURE_CASE_FAN

#if FEATURE_RGB_LIGHT_EFFECTS
    RGBLightMode = RGB_LIGHT_DEFAULT_MODE;
    if( RGBLightMode == RGB_MODE_AUTOMATIC )
    {
        RGBLightStatus = RGB_STATUS_AUTOMATIC;
    }
    else
    {
        RGBLightStatus = RGB_STATUS_NOT_AUTOMATIC;
    }
    RGBLightIdleStart      = 0;
    RGBButtonBackPressed   = 0;
    RGBLightModeForceWhite = 0;
#endif // FEATURE_RGB_LIGHT_EFFECTS

#if USE_ADVANCE
    extruderStepsNeeded = 0;
#endif // USE_ADVANCE

    UI_INITIALIZE;
    
    EEPROM::initBaudrate();
    HAL::serialSetBaudrate(baudrate);

    // sending of this information tells the Repetier-Host that the firmware has restarted - never delete or change this to-be-sent information
    Com::println();
    Com::printFLN(Com::tStart); //http://forum.repetier.com/discussion/comment/16949/#Comment_16949

    HAL::showStartReason();
    Extruder::initExtruder();
    
    // configure all DRV8711
    drv8711Init();
    
    // Read settings from eeprom if wanted [readDataFromEEPROM or destroy corrupted eeprom]
    EEPROM::init(); 


#if FEATURE_230V_OUTPUT
    enable230VOutput = OUTPUT_230V_DEFAULT_ON;
    SET_OUTPUT(OUTPUT_230V_PIN);
    WRITE(OUTPUT_230V_PIN, enable230VOutput);
#endif // FEATURE_230V_OUTPUT

#if FEATURE_CASE_LIGHT
    SET_OUTPUT(CASE_LIGHT_PIN);
    WRITE(CASE_LIGHT_PIN, enableCaseLight);
#endif // FEATURE_CASE_LIGHT

    SET_OUTPUT(CASE_FAN_PIN);

#if CASE_FAN_ALWAYS_ON
    WRITE(CASE_FAN_PIN, 1);
#else
    WRITE(CASE_FAN_PIN, 0);
#endif // CASE_FAN_ALWAYS_ON

    updateDerivedParameter();
    Commands::checkFreeMemory();
    Commands::writeLowestFreeRAM();
    HAL::setupTimer();

    Extruder::selectExtruderById(0);

#if SDSUPPORT
    sd.mount();
#endif // SDSUPPORT

    g_nActiveHeatBed = (char)readWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, EEPROM_OFFSET_ACTIVE_HEAT_BED_Z_MATRIX );
#if FEATURE_MILLING_MODE
    g_nActiveWorkPart = (char)readWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, EEPROM_OFFSET_ACTIVE_WORK_PART_Z_MATRIX );
#endif // FEATURE_MILLING_MODE

    if (Printer::ZMode == Z_VALUE_MODE_SURFACE)
    {
        if( g_ZCompensationMatrix[0][0] != EEPROM_FORMAT )
        {
            // we load the z compensation matrix
            prepareZCompensation();
        }
    }

#if FEATURE_RGB_LIGHT_EFFECTS
    setRGBLEDs( 0, 0, 0 );

    switch( RGBLightMode )
    {
        case RGB_MODE_OFF:
        {
            setRGBTargetColors( 0, 0, 0 );
            break;
        }
        case RGB_MODE_WHITE:
        {
            setRGBTargetColors( 255, 255, 255 );
            break;
        }
        case RGB_MODE_AUTOMATIC:
        {
            setRGBTargetColors( g_uRGBIdleR, g_uRGBIdleG, g_uRGBIdleB );
            break;
        }
        case RGB_MODE_MANUAL:
        {
            setRGBTargetColors( g_uRGBManualR, g_uRGBManualG, g_uRGBManualB );
            break;
        }
    }
#endif // FEATURE_RGB_LIGHT_EFFECTS

#if FEATURE_UNLOCK_MOVEMENT
    g_unlock_movement = 0;
#endif //FEATURE_UNLOCK_MOVEMENT

#if FEATURE_WATCHDOG
    if( Printer::debugInfo() )
    {
        Com::printFLN(Com::tStartWatchdog);
    }
    HAL::startWatchdog();
#endif // FEATURE_WATCHDOG
} // setup()


void Printer::defaultLoopActions()
{
    Commands::checkForPeriodicalActions();  //check heater every n milliseconds

    millis_t curtime = HAL::timeInMilliseconds();

    if( PrintLine::hasLines() || Printer::isPrinting() || isMenuMode(MENU_MODE_PAUSED) )
    {
        previousMillisCmd = curtime;
    }
    else
    {
        curtime -= previousMillisCmd;
        if( maxInactiveTime != 0 && curtime > maxInactiveTime ) Printer::kill(false);
        else Printer::setAllKilled(false); // prevent repeated kills
        if( stepperInactiveTime != 0 && curtime > stepperInactiveTime )
        {
            Printer::kill(true);
        }
    }

#if defined(SDCARDDETECT) && SDCARDDETECT>-1 && defined(SDSUPPORT) && SDSUPPORT
    sd.automount();
#endif // defined(SDCARDDETECT) && SDCARDDETECT>-1 && defined(SDSUPPORT) && SDSUPPORT

    DEBUG_MEMORY;

} // defaultLoopActions


#if FEATURE_MEMORY_POSITION
void Printer::MemoryPosition()
{
    //https://github.com/repetier/Repetier-Firmware/blob/652d86aa5ad778222cab738f4448f6495bb85245/src/ArduinoAVR/Repetier/Printer.cpp#L1309
    Commands::waitUntilEndOfAllMoves(); //MemoryPosition: Repetier hat hier schon geupdated aber alles auf CURRENT nicht auf LAST position? Macht das einen Unterschied?
    updateCurrentPosition(false);
    lastCalculatedPosition(memoryX,memoryY,memoryZ); //fill with queuePositionLastMM[]
    memoryE = queuePositionLastSteps[E_AXIS]*invAxisStepsPerMM[E_AXIS];
    memoryF = feedrate;
} // MemoryPosition

void Printer::GoToMemoryPosition(bool x,bool y,bool z,bool e,float feed)
{
    bool all = !(x || y || z);
    moveToReal((all || x ? memoryX : IGNORE_COORDINATE)
               ,(all || y ? memoryY : IGNORE_COORDINATE)
               ,(all || z ? memoryZ : IGNORE_COORDINATE)
               ,(e ? memoryE:IGNORE_COORDINATE),
               feed);
    feedrate = memoryF;
    updateCurrentPosition(false);
} // GoToMemoryPosition
#endif // FEATURE_MEMORY_POSITION



#if FEATURE_HEAT_BED_Z_COMPENSATION || FEATURE_WORK_PART_Z_COMPENSATION

#if FEATURE_SENSIBLE_PRESSURE
void Printer::enableSenseOffsetnow( void ){
    short oldval = HAL::eprGetInt16(EPR_RF_MOD_SENSEOFFSET_DIGITS);
    if ( oldval > 0 && oldval < EMERGENCY_PAUSE_DIGITS_MAX ){
        g_nSensiblePressureDigits = oldval;
    }
    oldval = HAL::eprGetInt16(EPR_RF_MOD_SENSEOFFSET_OFFSET_MAX);
    if( oldval > 0 && oldval < 300 ){
       g_nSensiblePressureOffsetMax = oldval;
    }
}
#endif // FEATURE_SENSIBLE_PRESSURE

void Printer::enableCMPnow( void ){
#if FEATURE_MILLING_MODE
    if( Printer::operatingMode == OPERATING_MODE_MILL)
    {
#if FEATURE_WORK_PART_Z_COMPENSATION
        if( Printer::doWorkPartZCompensation ) return; // false;
#endif // FEATURE_WORK_PART_Z_COMPENSATION
    }
    else
#endif // FEATURE_MILLING_MODE
    {
#if FEATURE_HEAT_BED_Z_COMPENSATION
        if( Printer::doHeatBedZCompensation ) return; // false;
#endif // FEATURE_HEAT_BED_Z_COMPENSATION
    }

    if( !Printer::areAxisHomed() ){
        return; // false;
    }
                    
    // enable the z compensation
    if( g_ZCompensationMatrix[0][0] != EEPROM_FORMAT )  return; // false;

    // enable the z compensation only in case we have valid compensation values
#if FEATURE_MILLING_MODE
    if( Printer::operatingMode == OPERATING_MODE_MILL )
    {
#if FEATURE_WORK_PART_Z_COMPENSATION
        Printer::doWorkPartZCompensation = 1;
#if FEATURE_FIND_Z_ORIGIN
        if( g_nZOriginPosition[X_AXIS] || g_nZOriginPosition[Y_AXIS] )
        {
            Printer::staticCompensationZ = getZMatrixDepth(g_nZOriginPosition[X_AXIS], g_nZOriginPosition[Y_AXIS]); //determineStaticCompensationZ();
        }
        else
        {
            // we know nothing about a static z-delta in case we do not know the x and y positions at which the z-origin has been determined
            Printer::staticCompensationZ = 0;
        }
#else
        // we know nothing about a static z-delta when we do not have the automatic search of the z-origin available
        Printer::staticCompensationZ = 0;
#endif // FEATURE_FIND_Z_ORIGIN
#endif // FEATURE_WORK_PART_Z_COMPENSATION
    }
    else
#endif // FEATURE_MILLING_MODE
    {
#if FEATURE_HEAT_BED_Z_COMPENSATION
        Printer::doHeatBedZCompensation = 1;
#endif // FEATURE_HEAT_BED_Z_COMPENSATION
    }
}

bool Printer::needsCMPwait( void ){
    if( abs( Printer::compensatedPositionCurrentStepsZ - Printer::compensatedPositionTargetStepsZ ) ){
        if( !Printer::checkCMPblocked() ) return true;
    }
    return false;
}

bool Printer::checkCMPblocked( void ){
    //die zcmp läuft dann, wenn die achse nicht gegenläufig steht und reserviert ist: 
    //true = zcmp ist blockiert -> ist mal möglich aber wenn nichts läuft ein fehler, weil die achse nicht zeitnah von einer anderen funktion freigegeben wird.
    //false = zcmp darf sich aktuell korrigieren, wenn sonst keine prioritären Einschränkungen aktiv sind. -> auf kompensationsende warten müsste erfolgreich verlaufen
    return ((Printer::compensatedPositionCurrentStepsZ < Printer::compensatedPositionTargetStepsZ) && !Printer::getZDirectionIsPos())
        || ((Printer::compensatedPositionCurrentStepsZ > Printer::compensatedPositionTargetStepsZ) &&  Printer::getZDirectionIsPos());
}

void Printer::disableCMPnow( bool wait ) {
#if FEATURE_MILLING_MODE
    if( Printer::operatingMode == OPERATING_MODE_PRINT )
    {
#endif // FEATURE_MILLING_MODE
 #if FEATURE_HEAT_BED_Z_COMPENSATION
        Printer::doHeatBedZCompensation = 0;
 #endif // FEATURE_HEAT_BED_Z_COMPENSATION
#if FEATURE_MILLING_MODE
 #if FEATURE_WORK_PART_Z_COMPENSATION
    }
    else
    {
        Printer::doWorkPartZCompensation = 0;
        Printer::staticCompensationZ     = 0;
    }
 #endif // FEATURE_WORK_PART_Z_COMPENSATION
#endif // FEATURE_MILLING_MODE
    Printer::compensatedPositionTargetStepsZ = 0; //tell CMP to move to 0. TODO: Care for positive matrix beds.
    
    while( wait && compensatedPositionCurrentStepsZ - compensatedPositionTargetStepsZ ) //warte auf queue befehlsende
    {
        HAL::delayMilliseconds( 1 );
        //checkforperiodical .. possible loop inside scans and homing.
    }
}
#endif // FEATURE_HEAT_BED_Z_COMPENSATION || FEATURE_WORK_PART_Z_COMPENSATION

int8_t Printer::anyHomeDir(uint8_t axis){
    int8_t nHomeDir = 0;
    switch(axis){
        case X_AXIS: 
            {
                if( (MIN_HARDWARE_ENDSTOP_X && X_MIN_PIN > -1 && X_HOME_DIR==-1) || (MAX_HARDWARE_ENDSTOP_X && X_MAX_PIN > -1 && X_HOME_DIR==1) )
                {
                    nHomeDir = X_HOME_DIR; //-1 -> RFx000.h
                }
                break;
            }
        case Y_AXIS: 
            {
                if( (MIN_HARDWARE_ENDSTOP_Y && Y_MIN_PIN > -1 && Y_HOME_DIR==-1) || (MAX_HARDWARE_ENDSTOP_Y && Y_MAX_PIN > -1 && Y_HOME_DIR==1) )
                {
                    nHomeDir = Y_HOME_DIR; //-1 -> RFx000.h
                }
                break;
            }
        case Z_AXIS: 
            {
    #if FEATURE_MILLING_MODE
                if( Printer::operatingMode == OPERATING_MODE_PRINT && (MIN_HARDWARE_ENDSTOP_Z && Z_MIN_PIN > -1) )
                {
                    // in operating mode "print" we use the z min endstop
                    nHomeDir = Z_HOME_DIR;
                }
                else if( MAX_HARDWARE_ENDSTOP_Z && Z_MAX_PIN > -1 )
                {
                    // in operating mode "mill" we use the z max endstop
                    nHomeDir = -1*Z_HOME_DIR;
                }
    #else
                if( (MIN_HARDWARE_ENDSTOP_Z && Z_MIN_PIN > -1 && Z_HOME_DIR==-1) || (MAX_HARDWARE_ENDSTOP_Z && Z_MAX_PIN > -1 && Z_HOME_DIR==1) )
                {
                    nHomeDir = Z_HOME_DIR; //-1 -> RFx000.h
                }
                break;
    #endif // FEATURE_MILLING_MODE
    
                /* ALT ohne check der Pins:
                    #if FEATURE_MILLING_MODE
                        //nProcess = 1;
                        if( Printer::operatingMode == OPERATING_MODE_PRINT )
                        {
                            // in operating mode "print" we use the z min endstop
                            nHomeDir = -1;
                        }
                        else
                        {
                            // in operating mode "mill" we use the z max endstop
                            nHomeDir = 1;
                        }
                    #else
                        if ((MIN_HARDWARE_ENDSTOP_Z && Z_MIN_PIN > -1 && Z_HOME_DIR==-1) || (MAX_HARDWARE_ENDSTOP_Z && Z_MAX_PIN > -1 && Z_HOME_DIR==1))
                        {
                            //nProcess = 1;
                            nHomeDir = Z_HOME_DIR;
                        }
                    #endif // FEATURE_MILLING_MODE
                */
            }
    }
    return nHomeDir;
}
    
#if FEATURE_CHECK_HOME
bool Printer::anyEndstop( uint8_t axis ){
    int endstop = 0;
    for(uint8_t i = 1; i <= 3; i++){
        switch(axis){
            case X_AXIS: 
                {
                    endstop += (Printer::isXMinEndstopHit() || Printer::isXMaxEndstopHit());
                    break;
                }
            case Y_AXIS: 
                {
                    endstop += (Printer::isYMinEndstopHit() || Printer::isYMaxEndstopHit());
                    break;
                }
            case Z_AXIS: 
                {
                    endstop += (Printer::isZMinEndstopHit() || Printer::isZMaxEndstopHit());
                    break;
                }
        }
    }
    return (bool)(endstop > 1); //endstop muss sauber gedrückt sein.
}

void Printer::changeAxisDirection( uint8_t axis, int8_t direction ){
    switch(axis){
        case X_AXIS: 
            {
                Printer::setXDirection( (direction > 0 ? true : false ) );
                break;
            }
        case Y_AXIS: 
            {
                Printer::setYDirection( (direction > 0 ? true : false ) );
                break;
            }
        case Z_AXIS: 
            {
                Printer::setZDirection( (direction > 0 ? true : false ) );
                break;
            }
    }
}

void Printer::startAxisStep( uint8_t axis ){
    switch(axis){
        case X_AXIS: 
            {
                Printer::startXStep();
                break;
            }
        case Y_AXIS: 
            {
                Printer::startYStep();
                break;
            }
        case Z_AXIS: 
            {
                Printer::startZStep();
                break;
            }
    }
}
void Printer::endAxisStep(uint8_t axis){
    switch(axis){
        case X_AXIS: 
            {
                Printer::endXStep();
                break;
            }
        case Y_AXIS: 
            {
                Printer::endYStep();
                break;
            }
        case Z_AXIS: 
            {
                Printer::endZStep();
                break;
            }
    }
}

void Printer::stepAxisStep(uint8_t axis, uint8_t slower){
    HAL::delayMicroseconds( XYZ_STEPPER_HIGH_DELAY * slower );
    Printer::startAxisStep(axis);
    HAL::delayMicroseconds( XYZ_STEPPER_LOW_DELAY * slower );
    Printer::endAxisStep(axis);
}

int8_t Printer::checkHome(int8_t axis) //X_AXIS 0, Y_AXIS 1, Z_AXIS 2
{
    //do not allow Z jet because we have to check compensation behaviour
    Com::printFLN( PSTR( "checkHome: start axis" ), axis );
    //if(axis == Z_AXIS) return -1; //noch verboten, weil solange das läuft der emergency-Stop nicht funktioniert. funktion wird evtl. wie HBS oder ZOS geteilt.
    
    if(axis > Z_AXIS) return -1;
    
    //do not allow checkHome without Home
    if( !Printer::isAxisHomed(axis) ) return -1;
    Com::printFLN( PSTR( "ishomed!" ) );
    
    //do not allow checkHome when some endstop is already reached
    if( Printer::anyEndstop(axis) ) return -1; //knopf gedrückt = kein sinnvoller rückfahrweg bzw. nicht vertrauenswürdige coordinaten.

    //get the right homeDirection for moving -> 0 is none.
    char nHomeDir = Printer::anyHomeDir(axis);
    Com::printFLN( PSTR( "nHomeDir=" ), nHomeDir );
    if(!nHomeDir) return -1;
    
    //set stepper direction as homing direction
    Printer::changeAxisDirection( axis, nHomeDir );

    //drive axis towards endstop:
    long Didsteps = 0;
    bool finish = false;
    int mmLoops = Printer::axisStepsPerMM[axis]; //fahre in mm-Blöcken.
    
    while(1){
        /* Pfusch, das hier extra einzufügen? -> ^^ Prototype -> Funktion muss u.U. später gesplittet und mit einer Art M400 geschützt werden. */
        HAL::tellWatchdogOk(); //ultra ausnahme!! Das gibts normal nur in RF.cpp an einer Stelle. TODO!!
        Extruder::manageTemperatures();
        GCode::keepAlive( Processing );
        
        for( int i=0; i < mmLoops; i++ )
        {
            Printer::stepAxisStep(axis);
            if( Printer::anyEndstop(axis) ){
                finish = true;
            }else{
                Didsteps += nHomeDir;
            }
            //verbot unendlich weiterzufahren, wenn endstop kaputt
            if( abs(Didsteps) > abs(Printer::maxSteps[axis] - Printer::minSteps[axis])*110/100 ) finish = true;
            
            if(finish) break;
        }
        if(finish) break;
    }
    
    //steps to log
    Com::printFLN( PSTR( "didsteps=" ), Didsteps );
    Com::printFLN( PSTR( "wassteps=" ), (Printer::queuePositionCurrentSteps[axis] + Printer::directPositionCurrentSteps[axis]) );
    
    Com::printFLN( PSTR( "recheck endstop:" ) );
    //check ob endstop da (oder defekt oder nicht erreicht: return)
    if (!Printer::anyEndstop(axis)) return -1;

    //change stepper direction to drive against homing direction
    Printer::changeAxisDirection( axis, -1*nHomeDir );
    //Ein paar steps zurück, bis schalter wieder losgelassen wurde:
    finish = false; // Neue Abbruchbedingung
    for( int i=0; i < mmLoops; i++ ) {
        Printer::stepAxisStep(axis, 20 /*20x langsamer*/);
        //end if button is not pressed anymore.
        if(!Printer::anyEndstop(axis)){
            finish = true;
        }else{
            Didsteps += -1*nHomeDir;
        }
        if(finish) break;
    }

    //check ob endstop nach 1mm immernoch gedrückt:
    if ( Printer::anyEndstop(axis) ) return -1;
    
    //merke schalter-aus-punkt für hysteresemessung:
    long hysterese = Didsteps;

    //Neu und langsamer in den Schalter fahren:    
    //change stepper direction to drive against homing direction
    Printer::changeAxisDirection( axis, nHomeDir );
    finish = false; // Neue Abbruchbedingung
    for( int i=0; i < mmLoops; i++ ) {
        Printer::stepAxisStep(axis, 20 /*20x langsamer*/);
        //end if button is not pressed anymore.
        if(Printer::anyEndstop(axis)){
            finish = true;
        }else{
            Didsteps += nHomeDir;
        }
        if(finish) break;
    }    
    //check ob endstop nach 1mm immernoch nicht gedrückt:
    if ( !Printer::anyEndstop(axis) ) return -1;
    
    hysterese -= Didsteps;
    Com::printFLN( PSTR( "aus-ein-hysterese=" ), hysterese );
    
    //check ob steps verloren gehen:
    long delta;
    if(axis == Z_AXIS) delta = Printer::currentZSteps; //zählt hoch und runter. alternativ achsen-kosys wie bei x und y, aber wegen zcompensation problematisch, weil verzerrt (??-> noch nicht fertig durchdacht).
    else               delta = Printer::queuePositionCurrentSteps[axis] + Printer::directPositionCurrentSteps[axis] + Didsteps;
    
    Com::printFLN( PSTR( "delta=" ), delta );

    //fahre zurück an Startpunkt:
    Printer::changeAxisDirection( axis, -1*nHomeDir );
    
    Didsteps = abs(Didsteps);
    while(Didsteps > 0){
        /* Pfusch, das hier extra einzufügen? -> ^^ Prototype -> Funktion muss u.U. später gesplittet und mit einer Art M400 geschützt werden. */
        HAL::tellWatchdogOk(); //ultra ausnahme!! Das gibts normal nur in RF.cpp an einer Stelle. TODO!!
        Extruder::manageTemperatures();
        GCode::keepAlive( Processing );
        
        if(Didsteps >= mmLoops){
            Didsteps -= mmLoops;
        }else{
            mmLoops = Didsteps;
            Didsteps = 0;
        }
        
        for( int i=0; i < mmLoops; i++ )
        {
            Printer::stepAxisStep(axis);
        }
    }

    return 1;
} // checkHome
#endif // FEATURE_CHECK_HOME


void Printer::homeXAxis()
{
    char    nHomeDir = Printer::anyHomeDir(X_AXIS);

    if (nHomeDir)
    {
        UI_STATUS_UPD(UI_TEXT_HOME_X);

#if FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING
        InterruptProtectedBlock noInts;
        directPositionTargetSteps[X_AXIS]  = 0;
        directPositionCurrentSteps[X_AXIS] = 0;
        directPositionLastSteps[X_AXIS]    = 0;
        noInts.unprotect();
#endif // FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING

        int32_t offX = 0;
#if NUM_EXTRUDER>1
        // Reposition extruder that way, that all extruders can be selected at home pos.
        for(uint8_t i=0; i<NUM_EXTRUDER; i++) offX = ( nHomeDir < 0 ? RMath::max(offX,extruder[i].xOffset) : RMath::min(offX,extruder[i].xOffset) );
#endif // NUM_EXTRUDER>1

#if FEATURE_MILLING_MODE
        if( Printer::operatingMode == OPERATING_MODE_MILL ) offX = 0; // in operating mode mill, there is no extruder offset
#endif // FEATURE_MILLING_MODE

        queuePositionLastSteps[X_AXIS] = (nHomeDir == -1) ? maxSteps[X_AXIS] + offX : minSteps[X_AXIS] - offX;

        PrintLine::moveRelativeDistanceInSteps( long( 2 * abs(maxSteps[X_AXIS] - minSteps[X_AXIS] + 2*offX) * nHomeDir ), 0, 0, 0, homingFeedrate[X_AXIS], true, true);
        PrintLine::moveRelativeDistanceInSteps( long( axisStepsPerMM[X_AXIS] * ENDSTOP_X_BACK_MOVE * (nHomeDir * -1) ),   0, 0, 0, homingFeedrate[X_AXIS] / ENDSTOP_X_RETEST_REDUCTION_FACTOR, true, false);
        PrintLine::moveRelativeDistanceInSteps( long( 2 * axisStepsPerMM[X_AXIS] * ENDSTOP_X_BACK_MOVE * nHomeDir ),      0, 0, 0, homingFeedrate[X_AXIS] / ENDSTOP_X_RETEST_REDUCTION_FACTOR, true, true);

        queuePositionLastSteps[X_AXIS]    = (nHomeDir == -1) ? minSteps[X_AXIS] - offX : maxSteps[X_AXIS] + offX;
        queuePositionCurrentSteps[X_AXIS] = queuePositionLastSteps[X_AXIS];

#if NUM_EXTRUDER>1
        if( offX )
        {
            PrintLine::moveRelativeDistanceInSteps((Extruder::current->xOffset-offX) * nHomeDir,0,0,0,homingFeedrate[X_AXIS],true,false);
        }
#endif // NUM_EXTRUDER>1

        // show that we are active
        previousMillisCmd = HAL::timeInMilliseconds();
        setHomed( true , -1 , -1);
    }

} // homeXAxis


void Printer::homeYAxis()
{
    char    nHomeDir = Printer::anyHomeDir(Y_AXIS);

    if (nHomeDir)
    {
        UI_STATUS_UPD(UI_TEXT_HOME_Y);

#if FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING
        InterruptProtectedBlock noInts;
        directPositionTargetSteps[Y_AXIS]  = 0;
        directPositionCurrentSteps[Y_AXIS] = 0;
        directPositionLastSteps[Y_AXIS]    = 0;
        noInts.unprotect();
#endif // FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING

        int32_t offY = 0;
#if NUM_EXTRUDER>1
        // Reposition extruder that way, that all extruders can be selected at home pos.
        for(uint8_t i=0; i<NUM_EXTRUDER; i++) offY = ( nHomeDir < 0 ? RMath::max(offY, extruder[i].yOffset) : RMath::min(offY, extruder[i].yOffset) );
#endif // NUM_EXTRUDER>1

#if FEATURE_MILLING_MODE
        if( Printer::operatingMode == OPERATING_MODE_MILL ) offY = 0; // in operating mode mill, there is no extruder offset
#endif // FEATURE_MILLING_MODE

        queuePositionLastSteps[Y_AXIS] = (nHomeDir == -1) ? maxSteps[Y_AXIS] + offY : minSteps[Y_AXIS] - offY;

        PrintLine::moveRelativeDistanceInSteps(0, long( 2 * abs(maxSteps[Y_AXIS] - minSteps[Y_AXIS] + 2*offY) * nHomeDir ), 0, 0, homingFeedrate[Y_AXIS], true, true);
        PrintLine::moveRelativeDistanceInSteps(0, long( axisStepsPerMM[Y_AXIS] * ENDSTOP_Y_BACK_MOVE * (nHomeDir * -1) ),   0, 0, homingFeedrate[Y_AXIS] / ENDSTOP_Y_RETEST_REDUCTION_FACTOR, true, false);
        PrintLine::moveRelativeDistanceInSteps(0, long( 2 * axisStepsPerMM[Y_AXIS] * ENDSTOP_Y_BACK_MOVE * nHomeDir),       0, 0, homingFeedrate[Y_AXIS] / ENDSTOP_Y_RETEST_REDUCTION_FACTOR, true, true);

        queuePositionLastSteps[Y_AXIS] = (nHomeDir == -1) ? minSteps[Y_AXIS] - offY : maxSteps[Y_AXIS] + offY;
        queuePositionCurrentSteps[Y_AXIS] = queuePositionLastSteps[Y_AXIS];

#if NUM_EXTRUDER>1
        if( offY )
        {
            PrintLine::moveRelativeDistanceInSteps(0,(Extruder::current->yOffset-offY) * nHomeDir,0,0,homingFeedrate[Y_AXIS],true,false);
        }
#endif // NUM_EXTRUDER>1

        // show that we are active
        previousMillisCmd = HAL::timeInMilliseconds();
        setHomed( -1 , true , -1);
    }
} // homeYAxis


void Printer::homeZAxis()
{
    char    nHomeDir = Printer::anyHomeDir(Z_AXIS);

    if( nHomeDir )
    {
        UI_STATUS_UPD( UI_TEXT_HOME_Z );
        
        // if we have circuit-type Z endstops and we don't know at which endstop we currently are, first move down a bit
#if FEATURE_CONFIGURABLE_Z_ENDSTOPS
        if( Printer::ZEndstopUnknown ) {
            //RF1000 und Min-oder-Max gedrückt - nicht klar welcher. Man fährt immer nach unten! Der Schalter hält das aus.
            PrintLine::moveRelativeDistanceInSteps(0, 0, axisStepsPerMM[Z_AXIS] * ENDSTOP_Z_BACK_MOVE, 0, homingFeedrate[Z_AXIS] / ENDSTOP_Z_RETEST_REDUCTION_FACTOR, true, false); //drucker muss immer nach 
        }
#endif

        //homing ausschalten und zCMP (...) auch.
        setHomed( -1 , -1 , false);

        InterruptProtectedBlock noInts;
#if FEATURE_FIND_Z_ORIGIN
        //das ist die Z-Origin-Höhe und ihre XY-Scan-Stelle.
        g_nZOriginPosition[X_AXIS] = 0;
        g_nZOriginPosition[Y_AXIS] = 0;
        g_nZOriginPosition[Z_AXIS] = 0;
        Printer::setZOriginSet(false); //removes flag wegen statusnachricht
#endif // FEATURE_FIND_Z_ORIGIN

#if FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING
        //nullen und in jedem fall sofort aufhören per directSteps zu fahren!
        directPositionTargetSteps[Z_AXIS]  = 0;
        directPositionCurrentSteps[Z_AXIS] = 0;
        directPositionLastSteps[Z_AXIS]    = 0;
#endif // FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING

#if FEATURE_HEAT_BED_Z_COMPENSATION || FEATURE_WORK_PART_Z_COMPENSATION
        //das ist diese Scan-Positions-Z-Zusatzachse für MoveZ-Bewegungen.
        g_nZScanZPosition = 0;
#endif // FEATURE_HEAT_BED_Z_COMPENSATION || FEATURE_WORK_PART_Z_COMPENSATION
        
        /*
        moveRelativeDistanceInSteps bedeutet: Printer::queuePositionTargetSteps[Z_AXIS] = Printer::queuePositionLastSteps[Z_AXIS] + deltaZ;
        dann ab in den MOVE_CACHE
        dann warten
        dann updateposition
        */
        //wir tun so als wären wir JETZT am anderen koordinaten-ende. Maximal weit weg vom Schalter.
        queuePositionLastSteps[Z_AXIS] = (nHomeDir == -1) ? maxSteps[Z_AXIS] : minSteps[Z_AXIS];
        noInts.unprotect();
                
        //1. Schnelles Fahren bis zum Schalterkontakt:
            //Ist der Schalter gedrückt, wird sofort geskipped. 
            //Ansonsten bis Schalter fahren. Doppelte mögliche Distanz, sodass Fahrt nicht zu früh aufhören kann.
        PrintLine::moveRelativeDistanceInSteps(0, 0, long( 2 * abs(maxSteps[Z_AXIS] - minSteps[Z_AXIS]) * nHomeDir ), 0, homingFeedrate[Z_AXIS], true, true); 
        
        //2. in jedem Fall Freifahren vom Schalterkontakt:
            //ENDSTOP_Z_BACK_MOVE größer als 32768 ist eigentlich nicht möglich, nicht sinnvoll und würde, da das überfahren bei 32microsteps von der z-matrix >-12,7mm abhängig ist verboten sein.
            //darum ist uint16_t in jedem fall ohne overflow.
        for(uint16_t step = 0; step < uint16_t(axisStepsPerMM[Z_AXIS] * ENDSTOP_Z_BACK_MOVE); step += uint16_t(0.1f * axisStepsPerMM[Z_AXIS]) ){
            //faktor *2 und *5 : doppelt/5x so schnell beim Zurücksetzen als nachher beim langsamst hinfahren. Sonst dauert das ewig.
            if(Printer::isZMinEndstopHit()){
                //schalter noch gedrückt, wir müssen weiter aus dem schalter rausfahren, aber keinesfalls mehr als ENDSTOP_Z_BACK_MOVE
                PrintLine::moveRelativeDistanceInSteps(0, 0, long(axisStepsPerMM[Z_AXIS] * 0.1f * (-1 * nHomeDir) ),                     0, float(homingFeedrate[Z_AXIS] / ENDSTOP_Z_RETEST_REDUCTION_FACTOR * 5.0f), true, false);
            }else{ //wir sind aus dem schalterbereich raus, müssten also nicht weiter zurücksetzen:
                //1) egal ob der schalter zu anfang überfahren war oder nicht: etwas zurücksetzen, nachdem der schalter angefahren wurde.
                //2) hier wird in jedem Fall etwas weiter weggefahren, sodass man wieder neu auf Z anfahren kann.
                PrintLine::moveRelativeDistanceInSteps(0, 0, long(axisStepsPerMM[Z_AXIS] * Z_ENDSTOP_MAX_HYSTERESIS * (-1 * nHomeDir) ), 0, float(homingFeedrate[Z_AXIS] / ENDSTOP_Z_RETEST_REDUCTION_FACTOR * 2.0f), true, false);
                break;
            }
        }
        
        //3. langsames Fahren bis zum Schalterkontakt:
        PrintLine::moveRelativeDistanceInSteps(0, 0, long(axisStepsPerMM[Z_AXIS] * (0.1f + ENDSTOP_Z_BACK_MOVE) * nHomeDir),          0, float(homingFeedrate[Z_AXIS] / ENDSTOP_Z_RETEST_REDUCTION_FACTOR), true, true);

#if FEATURE_MILLING_MODE
        //4. Wenn Millingmode dann nochmal freifahren und Koordinate nullen.
        if( Printer::operatingMode == OPERATING_MODE_MILL )
        {
            // when the milling mode is active and we are in operating mode "mill", we use the z max endstop and we free the z-max endstop after it has been hit
            PrintLine::moveRelativeDistanceInSteps(0, 0, long(axisStepsPerMM[Z_AXIS] * (LEAVE_Z_MAX_ENDSTOP_AFTER_HOME + Z_ENDSTOP_MAX_HYSTERESIS) * (-1 * nHomeDir) ), 0, float(homingFeedrate[Z_AXIS]), true, false);
        }
#endif // FEATURE_MILLING_MODE

        //5. Setzen der aktuellen End-Position auf die Koordinate, zu der das Homing gehört.
        noInts.protect();
        queuePositionLastSteps[Z_AXIS]    = (nHomeDir == -1) ? minSteps[Z_AXIS] : maxSteps[Z_AXIS];
        queuePositionCurrentSteps[Z_AXIS] = queuePositionLastSteps[Z_AXIS];
        currentZSteps                     = queuePositionLastSteps[Z_AXIS]; //das ist die Z-Koordinate, die die Z-Steps per Dir-Pin abzählt.
        noInts.unprotect();

        //6. Korrektur der Flags
        setHomed( -1 , -1 , true);

        // show that we are active
        previousMillisCmd = HAL::timeInMilliseconds();
#if FEATURE_CONFIGURABLE_Z_ENDSTOPS
        ZEndstopUnknown = false;
#endif // FEATURE_CONFIGURABLE_Z_ENDSTOPS
    }

} // homeZAxis

void Printer::homeDigits(){
#if FEATURE_ZERO_DIGITS
    short   nTempPressure = 0;
    if(Printer::g_pressure_offset_active){ //only adjust pressure if you do a full homing.
        Printer::g_pressure_offset = 0; //prevent to messure already adjusted offset -> without = 0 this would only iterate some bad values.
        if( !readAveragePressure( &nTempPressure ) ){
            if(-5000 < nTempPressure && nTempPressure < 5000){
                Com::printFLN( PSTR( "DigitOffset = " ), nTempPressure );
                Printer::g_pressure_offset = nTempPressure;
            }else{
                //those high values shouldnt happen! fix your machine... DONT ZEROSCALE DIGITS
                Com::printFLN( PSTR( "DigitOffset failed " ), nTempPressure );
            }
        } else{
            Com::printFLN( PSTR( "DigitOffset failed reading " ) );
            g_abortZScan = 0;
        }
    }
#endif // FEATURE_ZERO_DIGITS
}

void Printer::homeAxis(bool xaxis,bool yaxis,bool zaxis) // home non-delta printer
{
    g_uStartOfIdle = 0; //start of homing xyz

    //Bei beliebiger user interaktion oder Homing soll G1 etc. erlaubt werden. Dann ist der Drucker nicht abgestürzt, sondern bedient worden.
#if FEATURE_UNLOCK_MOVEMENT
    g_unlock_movement = 1;
#endif //FEATURE_UNLOCK_MOVEMENT

    float   startX,startY,startZ;
    lastCalculatedPosition(startX,startY,startZ); //fill with queuePositionLastMM[]

    char    homingOrder;
#if FEATURE_MILLING_MODE
    if( operatingMode == OPERATING_MODE_PRINT )
    {
#endif // FEATURE_MILLING_MODE
        homingOrder = HOMING_ORDER_PRINT;
#if FEATURE_MILLING_MODE
    }
    else
    {
        homingOrder = HOMING_ORDER_MILL;
    }
#endif // FEATURE_MILLING_MODE

#if FEATURE_CONFIGURABLE_Z_ENDSTOPS
    //ich weiß nicht ob das überhaupt ein gutes verhalten ist. Es ist nicht gut, wenn die z-schraube zu weit reingeschraubt ist und daher z-homing über dem bett verboten sein sollte.
    //Printer::ZEndstopUnknown == 1 gibts nur, wenn der RF1000 mit Circuitschaltung mit einem der z-endstops gedrückt aufwacht, oder man in dieser position auf den endstop umstellt.
    //Ein Z-Homing mit EndstopUnknown == 1 fährt immer erst nach unten, also ist unsere Druckerdüse safe, wenn wir erst Z homen bevor X und Y gehomed werden darf.
    if( Printer::ZEndstopUnknown && homingOrder < 5 /*not z first -> do z first*/ )
    {
        if(homingOrder == HOME_ORDER_XYZ
        || homingOrder == HOME_ORDER_XZY)
        {
            homingOrder = HOME_ORDER_ZXY;
        }
        else if(homingOrder == HOME_ORDER_YXZ
             || homingOrder == HOME_ORDER_YZX)
        {
                homingOrder = HOME_ORDER_ZYX;
        }
    }
#endif //FEATURE_CONFIGURABLE_Z_ENDSTOPS
    
#if FEATURE_MILLING_MODE
    if(operatingMode == OPERATING_MODE_PRINT){ //wollte nicht milling mode, weil ich die mechanik da nicht kenne, dieses if ist unter umständen nutzlos.
#endif // FEATURE_MILLING_MODE
      if( (!yaxis && zaxis) || ( /* z vor y */ homingOrder == HOME_ORDER_XZY || homingOrder == HOME_ORDER_ZXY || homingOrder == HOME_ORDER_ZYX ) )
      {
       // do not allow homing Z-Only within menu, when the Extruder is configured < 0 and over bed.
       if( !Printer::isZHomeSafe() ) 
       {
          homeYAxis(); //bevor die düse gegen das bett knallen könnte, weil positive z-matix oder tipdown-extruder sollte erst y genullt werden: kann das im printermode schädlich sein?
          //wenn Z genullt wird, sollte auch Y genullt werden dürfen.
       }
      }
#if FEATURE_MILLING_MODE
    }
#endif // FEATURE_MILLING_MODE

    switch( homingOrder )
    {
        case HOME_ORDER_XYZ:
        {
            if(xaxis) homeXAxis();
            if(yaxis) homeYAxis();
            if(zaxis) homeZAxis();
            break;
        }
        case HOME_ORDER_XZY:
        {
            if(xaxis) homeXAxis();
            if(zaxis) homeZAxis();
            if(yaxis) homeYAxis();
            break;
        }
        case HOME_ORDER_YXZ:
        {
            if(yaxis) homeYAxis();
            if(xaxis) homeXAxis();
            if(zaxis) homeZAxis();
            break;
        }
        case HOME_ORDER_YZX:
        {
            if(yaxis) homeYAxis();
            if(zaxis) homeZAxis();
            if(xaxis) homeXAxis();
            break;
        }
        case HOME_ORDER_ZXY:
        {
            if(zaxis) homeZAxis();
            if(xaxis) homeXAxis();
            if(yaxis) homeYAxis();
            break;
        }
        case HOME_ORDER_ZYX:
        {
            if(zaxis) homeZAxis();
            if(yaxis) homeYAxis();
            if(xaxis) homeXAxis();
            break;
        }
    }

    //warum das nicht in die x_y_z_homingfunctions verlegen? gemeinsame fahrt? was, wenn das beim einzelhoming fehlt -> schlimm?
    //könnte ich die definition und das lesen von startx hier runterlegen? oder wird home?Axis einen einfluss drauf haben?
    // ---> Weil homeZAxis etc. private ist kein problem.
    //###############################
    if(xaxis)
    {
        int8_t xhomedir = Printer::anyHomeDir(X_AXIS);
        if(xhomedir < 0)      startX = Printer::minMM[X_AXIS];
        else if(xhomedir > 0) startX = Printer::minMM[X_AXIS]+Printer::lengthMM[X_AXIS];
    }
    if(yaxis)
    {
        int8_t yhomedir = Printer::anyHomeDir(Y_AXIS);
        if(yhomedir < 0)      startY = Printer::minMM[Y_AXIS];
        else if(yhomedir > 0) startY = Printer::minMM[Y_AXIS]+Printer::lengthMM[Y_AXIS];
    }
    if(zaxis)
    {
        int8_t zhomedir = Printer::anyHomeDir(Z_AXIS);
        if(zhomedir < 0)      startZ = 0;                           //switch is zero. no min Z available in this printer.
        else if(zhomedir > 0) startZ = Printer::lengthMM[Z_AXIS];
    }
    updateCurrentPosition(true);
    moveToReal(startX,startY,startZ,IGNORE_COORDINATE,(zaxis ? homingFeedrate[Z_AXIS] : RMath::min(homingFeedrate[X_AXIS],homingFeedrate[Y_AXIS]) ));
    //###############################
    
    if(xaxis && yaxis && zaxis) Printer::homeDigits();

    g_uStartOfIdle = HAL::timeInMilliseconds(); //homing xyz just ended
    Commands::printCurrentPosition();
} // homeAxis


bool Printer::allowQueueMove( void )
{
#if FEATURE_PAUSE_PRINTING
    if( g_pauseStatus == PAUSE_STATUS_PAUSED ) return false;
#endif // FEATURE_PAUSE_PRINTING
    if( !( (PAUSE_STATUS_GOTO_PAUSE1 <= g_pauseStatus && g_pauseStatus <= PAUSE_STATUS_HEATING) || g_pauseStatus == PAUSE_STATUS_NONE) 
        && !PrintLine::cur )
    {
        // do not allow to process new moves from the queue while the printing is paused
        return false;
    }

    if( !PrintLine::hasLines() )
    {
        // do not allow to process moves from the queue in case there is no queue
        return false;
    }

    // we are not paused and there are moves in our queue - we should process them
    return true;

} // allowQueueMove


#if FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING
bool Printer::allowDirectMove( void )
{
    if( PrintLine::direct.stepsRemaining )
    {
        // the currently known direct movements must be processed as move
        return true;
    }

    // the currently known direct movements must be processed as single steps
    return false;

} // allowDirectMove


bool Printer::allowDirectSteps( void )
{
    if( PrintLine::direct.stepsRemaining )
    {
        // the currently known direct movements must be processed as move
        return false;
    }

    if( endZCompensationStep )
    {
        // while zcompensation is working it has higher priority
        return false;
    }

    // the currently known direct movements must be processed as single steps
    return true;

} // allowDirectSteps


bool Printer::processAsDirectSteps( void )
{
    if( PrintLine::linesCount || waitMove )
    {
        // we are printing at the moment, thus all direct movements must be processed as single steps
        return true;
    }

    // we are not printing at the moment, thus all direct movements must be processed as move
    return false;

} // processAsDirectSteps

#endif // FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING


#if FEATURE_HEAT_BED_Z_COMPENSATION || FEATURE_WORK_PART_Z_COMPENSATION
void Printer::performZCompensation( void )
{
    if( blockAll )
    {
        // do not perform any compensation in case the moving is blocked
        return;
    }

    if( endZCompensationStep ) //zuerst beenden, bevor irgendwas neues an Z manipuliert werden darf. 20.07.2017 Nibbels
    {
#if STEPPER_HIGH_DELAY>0
        HAL::delayMicroseconds( STEPPER_HIGH_DELAY );
#endif // STEPPER_HIGH_DELAY>0

        Printer::endZStep();
        compensatedPositionCurrentStepsZ += endZCompensationStep;
        endZCompensationStep = 0;

        //Insert little wait if next Z step might follow now.
        if( isDirectOrQueueOrCompZMove() )
        {
#if STEPPER_HIGH_DELAY+DOUBLE_STEP_DELAY>0
            HAL::delayMicroseconds(STEPPER_HIGH_DELAY+DOUBLE_STEP_DELAY);
#endif // STEPPER_HIGH_DELAY+DOUBLE_STEP_DELAY>0
            return;
        }
        else if( compensatedPositionCurrentStepsZ == compensatedPositionTargetStepsZ /* && not isDirectOrQueueOrCompZMove() */ ) stepperDirection[Z_AXIS] = 0; //-> Ich glaube, das brauchen wir hier nicht, wenn der DirectMove sauber abschließt. Hier wird nur reingeschummelt wenn ein DirectMove läuft.

        return;
    }

    if( PrintLine::cur )
    {
        if( PrintLine::cur->isZMove() )
        {
            // do not peform any compensation while there is a queue move into z-direction
            if( PrintLine::cur->stepsRemaining ) return;
            else PrintLine::cur->setZMoveFinished();
        }
    }

    if( Printer::directPositionCurrentSteps[Z_AXIS] != Printer::directPositionTargetSteps[Z_AXIS] )
    {
        // do not perform any compensation while there is a direct move into z-direction
        return;
    }

    if( compensatedPositionCurrentStepsZ < compensatedPositionTargetStepsZ )
    {
        // here we shall move the z-axis only in case performQueueMove() is not moving into the other direction at the moment
        if( !stepperDirection[Z_AXIS] )
        {
            // set the direction only in case it is not set already
            Printer::setZDirection(true);
        }
        // we must move the heat bed do the bottom
        if( Printer::getZDirectionIsPos() )
        {
            Printer::startZStep();
            endZCompensationStep = 1;
        }
        return;
    }

    if( compensatedPositionCurrentStepsZ > compensatedPositionTargetStepsZ )
    {
        // here we shall move the z-axis only in case performQueueMove() is not moving into the other direction at the moment
        if( !stepperDirection[Z_AXIS] )
        {
            // set the direction only in case it is not set already
            Printer::setZDirection(false);
        }
        // we must move the heat bed to the top
        if( !Printer::getZDirectionIsPos() )
        {
            Printer::startZStep();
            endZCompensationStep = -1;
        }
        return;
    }

} // performZCompensation

#endif // FEATURE_HEAT_BED_Z_COMPENSATION || FEATURE_WORK_PART_Z_COMPENSATION

void Printer::stopPrint() //function for aborting USB and SD-Prints
{
    if( !Printer::isPrinting() ) return;
    g_uStartOfIdle = 0; //jetzt nicht mehr in showidle() gehen;

#if SDSUPPORT
    if( sd.sdmode ) //prüfung auf !sdmode sollte hier eigenlicht nicht mehr nötig sein, aber ..
    {
         //block sdcard from reading more.
        Com::printFLN( PSTR("SD print stopped.") );
        sd.sdmode = 0;
    }
    else
#endif //SDSUPPORT
    {
         //block some of the usb sources from sending more data
        Com::printFLN( PSTR( "RequestStop:" ) ); //tell repetierserver to stop.
        Com::printFLN( PSTR( "// action:cancel" ) ); //tell octoprint to cancel print. > 1.3.7  https://github.com/foosel/OctoPrint/issues/2367#issuecomment-357554341
        Com::printFLN( PSTR("USB print stopped.") );
    }

    InterruptProtectedBlock noInts;
    g_uBlockCommands = 1; //keine gcodes mehr ausführen bis beenden beendet.
    //now mark the print to get cleaned up after some time:
    g_uStopTime = HAL::timeInMilliseconds(); //starts output object in combination with g_uBlockCommands
    
#if FEATURE_PAUSE_PRINTING
    if( g_pauseStatus != PAUSE_STATUS_NONE )
    {
        // the printing is paused at the moment
        g_nContinueSteps[X_AXIS] = 0;
        g_nContinueSteps[Y_AXIS] = 0;
        g_nContinueSteps[Z_AXIS] = 0;
        g_nContinueSteps[E_AXIS] = 0;

        g_uPauseTime  = 0;
        g_pauseStatus = PAUSE_STATUS_NONE;
        g_pauseMode   = PAUSE_MODE_NONE;
    }
    Printer::setMenuMode(MENU_MODE_PAUSED,false); //egal ob nicht gesetzt.
#endif // FEATURE_PAUSE_PRINTING

    //erase the coordinates and kill the current taskplaner:
    PrintLine::resetPathPlanner();
    PrintLine::cur = NULL;
    // we have to tell the firmware about its real current position
    Printer::queuePositionLastSteps[X_AXIS] = Printer::queuePositionTargetSteps[X_AXIS] = Printer::queuePositionCurrentSteps[X_AXIS];
    Printer::queuePositionLastSteps[Y_AXIS] = Printer::queuePositionTargetSteps[Y_AXIS] = Printer::queuePositionCurrentSteps[Y_AXIS];
    Printer::queuePositionLastSteps[Z_AXIS] = Printer::queuePositionTargetSteps[Z_AXIS] = Printer::queuePositionCurrentSteps[Z_AXIS];
    //G92 E0:
    Printer::queuePositionTargetSteps[E_AXIS] = Printer::queuePositionLastSteps[E_AXIS] = 0;
    Printer::updateCurrentPosition(true);
    noInts.unprotect();

    Commands::printCurrentPosition();
    UI_STATUS_UPD( UI_TEXT_STOP_PRINT );
} // stopPrint

extern void ui_check_keys(int &action);
bool Printer::checkAbortKeys( void ){
    int16_t activeKeys = 0;
    ui_check_keys(activeKeys);
    if(activeKeys == UI_ACTION_OK || activeKeys == UI_ACTION_BACK){
        return true;
    }
    return false;
}

bool Printer::checkPlayKey( void ){
    if(g_pauseMode == PAUSE_MODE_NONE){
        int16_t activeKeys = 0;
        ui_check_keys(activeKeys);
        if(activeKeys == UI_ACTION_RF_CONTINUE){
            return true;
        }
    }
    return false;
}
