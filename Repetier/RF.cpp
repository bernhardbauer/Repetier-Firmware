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


#include <Arduino.h> // capital A so it is error prone on case-sensitive filesystems
#include "Repetier.h"
#include <Wire.h>


FSTRINGVALUE( ui_text_error, UI_TEXT_ERROR )
FSTRINGVALUE( ui_text_warning, UI_TEXT_WARNING )
FSTRINGVALUE( ui_text_information, UI_TEXT_INFORMATION )
FSTRINGVALUE( ui_text_set_origin, UI_TEXT_SET_ORIGIN )
FSTRINGVALUE( ui_text_heat_bed_scan, UI_TEXT_HEAT_BED_SCAN )
FSTRINGVALUE( ui_text_work_part_scan, UI_TEXT_WORK_PART_SCAN )
FSTRINGVALUE( ui_text_find_z_origin, UI_TEXT_FIND_Z_ORIGIN )
FSTRINGVALUE( ui_text_output_object, UI_TEXT_OUTPUT_OBJECT )
FSTRINGVALUE( ui_text_park_heat_bed, UI_TEXT_PARK_HEAT_BED )
FSTRINGVALUE( ui_text_pause, UI_TEXT_PAUSE )
FSTRINGVALUE( ui_text_home, UI_TEXT_HOME )
FSTRINGVALUE( ui_text_delete_file, UI_TEXT_DELETE_FILE )
FSTRINGVALUE( ui_text_z_compensation, UI_TEXT_Z_COMPENSATION )
FSTRINGVALUE( ui_text_change_mode, UI_TEXT_CHANGE_MODE )
FSTRINGVALUE( ui_text_change_z_type, UI_TEXT_CHANGE_Z_TYPE )
FSTRINGVALUE( ui_text_change_miller_type, UI_TEXT_CHANGE_MILLER_TYPE )
FSTRINGVALUE( ui_text_x_axis, UI_TEXT_X_AXIS )
FSTRINGVALUE( ui_text_y_axis, UI_TEXT_Y_AXIS )
FSTRINGVALUE( ui_text_z_axis, UI_TEXT_Z_AXIS )
FSTRINGVALUE( ui_text_extruder, UI_TEXT_EXTRUDER )
FSTRINGVALUE( ui_text_autodetect_pid, UI_TEXT_AUTODETECT_PID )
FSTRINGVALUE( ui_text_temperature_manager, UI_TEXT_TEMPERATURE_MANAGER )
FSTRINGVALUE( ui_text_home_unknown, UI_TEXT_HOME_UNKNOWN )
FSTRINGVALUE( ui_text_saving_needless, UI_TEXT_SAVING_NEEDLESS )
FSTRINGVALUE( ui_text_operation_denied, UI_TEXT_OPERATION_DENIED )
FSTRINGVALUE( ui_text_emergency_pause, UI_TEXT_EMERGENCY_PAUSE )
FSTRINGVALUE( ui_text_emergency_stop, UI_TEXT_EMERGENCY_STOP )
FSTRINGVALUE( ui_text_invalid_matrix, UI_TEXT_INVALID_MATRIX )
FSTRINGVALUE( ui_text_min_reached, UI_TEXT_MIN_REACHED )
FSTRINGVALUE( ui_text_min_reached_unhomed, UI_TEXT_MIN_REACHED_UNHOMED )
FSTRINGVALUE( ui_text_max_reached, UI_TEXT_MAX_REACHED )
FSTRINGVALUE( ui_text_temperature_wrong, UI_TEXT_TEMPERATURE_WRONG )
FSTRINGVALUE( ui_text_timeout, UI_TEXT_TIMEOUT )
FSTRINGVALUE( ui_text_sensor_error, UI_TEXT_SENSOR_ERROR )
FSTRINGVALUE( ui_text_heat_bed_zoffset_search_status, UI_TEXT_HEAT_BED_ZOFFSET_SEARCH_STATUS )
FSTRINGVALUE( ui_text_heat_bed_zoffset_fix_z1, UI_TEXT_HEAT_BED_ZOFFSET_FIX_Z1 )
FSTRINGVALUE( ui_text_question, UI_TEXT_UNKNOWN )
FSTRINGVALUE( ui_text_heat_bed_zoffset_fix_z2, UI_TEXT_HEAT_BED_ZOFFSET_FIX_Z2 )
FSTRINGVALUE( ui_text_statusmsg, UI_TEXT_STATUSMSG )

FSTRINGVALUE( ui_text_saving_success, UI_TEXT_SAVING_SUCCESS )

unsigned long   g_uStartOfIdle             = 0;

#if FEATURE_HEAT_BED_Z_COMPENSATION
long            g_offsetZCompensationSteps = 0;
long            g_minZCompensationSteps    = HEAT_BED_Z_COMPENSATION_MIN_STEPS;
long            g_maxZCompensationSteps    = HEAT_BED_Z_COMPENSATION_MAX_STEPS;
long            g_diffZCompensationSteps   = HEAT_BED_Z_COMPENSATION_MAX_STEPS - HEAT_BED_Z_COMPENSATION_MIN_STEPS;
volatile unsigned char  g_nHeatBedScanStatus       = 0;
char            g_nActiveHeatBed           = 1;

//ZOS
//Nibbels: Das ist wie die g_nHeatBedScanStatus, die Schwestervariable, für den ZOS-Scan -> Vorsicht, wenn man sowas einführt müssen die überall vermerkt werden, weil sonst z.B. der G-Code weiter vorgeführt wird.
// g_ZMatrixChangedInRam soll 1 werden, wenn ZOS, Offsetänderung der Matrix etc. Sonst wäre Sichern der Matrix unnötig.
volatile unsigned char  g_ZMatrixChangedInRam = 0;
volatile unsigned char  g_ZOSScanStatus    = 0;     
//Nibbels:
unsigned char   g_ZOSTestPoint[2]     = { SEARCH_HEAT_BED_OFFSET_SCAN_POSITION_INDEX_X, SEARCH_HEAT_BED_OFFSET_SCAN_POSITION_INDEX_Y };
float           g_ZOSlearningRate = 1.0;
float           g_ZOSlearningGradient = 0.0;
long            g_min_nZScanZPosition = 0;
unsigned char   g_ZOS_Auto_Matrix_Leveling_State = 0; //if 1 do multiple scans to correct matrix. while correcting this is rising for switch cases see state 50+
#endif // FEATURE_HEAT_BED_Z_COMPENSATION

#if FEATURE_WORK_PART_Z_COMPENSATION
char            g_nWorkPartScanStatus = 0;
char            g_nWorkPartScanMode   = 0;
char            g_nActiveWorkPart     = 1;
#endif // FEATURE_WORK_PART_Z_COMPENSATION

#if FEATURE_HEAT_BED_Z_COMPENSATION || FEATURE_WORK_PART_Z_COMPENSATION
unsigned long   g_lastScanTime        = 0;
unsigned long   g_scanStartTime       = 0;
char            g_scanRetries         = 0;
char            g_retryZScan          = 0;
unsigned char   g_retryStatus         = 0;

#if FEATURE_PRECISE_HEAT_BED_SCAN
char            g_nHeatBedScanMode    = 0;
#endif // FEATURE_PRECISE_HEAT_BED_SCAN

char            g_abortZScan          = 0;
short           g_ZCompensationMatrix[COMPENSATION_MATRIX_MAX_X][COMPENSATION_MATRIX_MAX_Y];
unsigned char   g_uZMatrixMax[2]      = { 0, 0 };
long            g_nZScanZPosition     = 0;
long            g_nLastZScanZPosition = 0;

short           g_nMaxPressureContact;
short           g_nMaxPressureRetry;
short           g_nMaxPressureIdle;
short           g_nMinPressureContact;
short           g_nMinPressureRetry;
short           g_nMinPressureIdle;
short           g_nFirstIdlePressure;
short           g_nCurrentIdlePressure;
char            g_nTempDirectionZ            = 0;   // this is the current z-direction during operations like the bed scan or finding of the z-origin

// configurable scan parameters - the proper default values are set by restoreDefaultScanParameters()
long            g_nScanXStartSteps           = 0;
long            g_nScanXStepSizeMm           = 0;
long            g_nScanXStepSizeSteps        = 0;
long            g_nScanXEndSteps             = 0;
long            g_nScanXMaxPositionSteps     = 0;
long            g_nScanYStartSteps           = 0;
long            g_nScanYStepSizeMm           = 0;
long            g_nScanYStepSizeSteps        = 0;
long            g_nScanYEndSteps             = 0;
long            g_nScanYMaxPositionSteps     = 0;
short           g_nScanHeatBedUpFastSteps    = 0;
short           g_nScanHeatBedUpSlowSteps    = 0;
short           g_nScanHeatBedDownFastSteps  = 0;
short           g_nScanHeatBedDownSlowSteps  = 0;
long            g_nScanZMaxCompensationSteps = 0;
unsigned short  g_nScanFastStepDelay         = 0;
unsigned short  g_nScanSlowStepDelay         = 0;
unsigned short  g_nScanIdleDelay             = 0;
unsigned short  g_nScanContactPressureDelta  = 0;
unsigned short  g_nScanRetryPressureDelta    = 0;
unsigned short  g_nScanIdlePressureDelta     = 0;
short           g_nScanIdlePressureMin       = 0;
short           g_nScanIdlePressureMax       = 0;
char            g_nScanPressureReads         = 0;
unsigned short  g_nScanPressureReadDelay     = 0;
short           g_nScanPressureTolerance     = 0;
#endif // FEATURE_HEAT_BED_Z_COMPENSATION || FEATURE_WORK_PART_Z_COMPENSATION

#if DEBUG_REMEMBER_SCAN_PRESSURE
short           g_ScanPressure[COMPENSATION_MATRIX_MAX_X][COMPENSATION_MATRIX_MAX_Y];
#endif // DEBUG_REMEMBER_SCAN_PRESSURE

long            g_staticZSteps              = 0;
char            g_debugLevel                = 0;
char            g_debugLog                  = 0;
//long          g_debugCounter[20]          = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
//short         g_debugCounter[12]          = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
//short         g_debugCounter[6]           = { 0, 0, 0, 0, 0, 0 };
unsigned long   g_uStopTime                 = 0;
unsigned long   g_uBlockSDCommands          = 0;
//short         g_debugInt16                = 0;
//unsigned short    g_debugUInt16               = 0;
//long          g_debugInt32                = 0;

#if FEATURE_EXTENDED_BUTTONS
// other configurable parameters
unsigned long   g_nManualSteps[4]           = { (unsigned long)DEFAULT_MANUAL_STEPS_X, (unsigned long)DEFAULT_MANUAL_STEPS_Y, (unsigned long)DEFAULT_MANUAL_STEPS_Z, (unsigned long)DEFAULT_MANUAL_STEPS_E };
#endif // FEATURE_EXTENDED_BUTTONS

#if FEATURE_PAUSE_PRINTING
volatile long   g_nPauseSteps[4]            = { (long)DEFAULT_PAUSE_STEPS_X, (long)DEFAULT_PAUSE_STEPS_Y, (long)DEFAULT_PAUSE_STEPS_Z, (long)DEFAULT_PAUSE_STEPS_EXTRUDER };
volatile long   g_nContinueSteps[4]         = { 0, 0, 0, 0 };
volatile char   g_pauseStatus               = PAUSE_STATUS_NONE;
volatile char   g_pauseMode                 = PAUSE_MODE_NONE;
volatile unsigned long  g_uPauseTime                = 0;
volatile char           g_pauseBeepDone             = 0;
#endif // FEATURE_PAUSE_PRINTING

#if FEATURE_PARK
long            g_nParkPosition[3]          = { PARK_POSITION_X, PARK_POSITION_Y, PARK_POSITION_Z };
#endif // FEATURE_PARK

#if FEATURE_EMERGENCY_PAUSE
long            g_nEmergencyPauseDigitsMin  = EMERGENCY_PAUSE_DIGITS_MIN; //short reicht eigentlich
long            g_nEmergencyPauseDigitsMax  = EMERGENCY_PAUSE_DIGITS_MAX; //short reicht eigentlich
#endif // FEATURE_EMERGENCY_PAUSE

#if FEATURE_EMERGENCY_STOP_ALL
short            g_nZEmergencyStopAllMin  = EMERGENCY_STOP_DIGITS_MIN;
short            g_nZEmergencyStopAllMax  = EMERGENCY_STOP_DIGITS_MAX;
#endif //FEATURE_EMERGENCY_STOP_ALL

#if FEATURE_SENSIBLE_PRESSURE
/* brief: This is for correcting too close Z at first layer, see FEATURE_SENSIBLE_PRESSURE // Idee Wessix, coded by Nibbels  */
short           g_nSensiblePressureDigits   = 0; //init auf zahl bringt stand 1.37r2 nur was wenn zcompensation an. darum lesen aus eeprom oder setzen auf wert nur durch Mcode
short           g_nSensiblePressureOffsetMax = SENSIBLE_PRESSURE_MAX_OFFSET;
short           g_nSensiblePressureOffset   = 0;
char            g_nSensiblePressure1stMarke = 0; //sagt, ob regelung aktiv oder inaktiv, wegen Z-Limits
#endif // FEATURE_SENSIBLE_PRESSURE

short           g_nLastDigits = 0;
#if FEATURE_DIGIT_Z_COMPENSATION
float           g_nDigitZCompensationDigits = 0.0f;
bool            g_nDigitZCompensationDigits_active = true;
 #if FEATURE_DIGIT_FLOW_COMPENSATION
 int8_t           g_nDigitFlowCompensation_intense = 0; // +- %
 short            g_nDigitFlowCompensation_Fmin = 5000;  //mögliche Standardwerte
 short            g_nDigitFlowCompensation_Fmax = 12000; //mögliche Standardwerte
 float            g_nDigitFlowCompensation_flowmulti = 1.0f;
 #endif // FEATURE_DIGIT_FLOW_COMPENSATION
#endif // FEATURE_DIGIT_Z_COMPENSATION

#if FEATURE_EMERGENCY_STOP_ALL
unsigned long   uLastZPressureTime_IgnoreUntil = 0;
#endif // FEATURE_EMERGENCY_STOP_ALL

#if FEATURE_FIND_Z_ORIGIN
volatile char   g_nFindZOriginStatus        = 0;
long            g_nZOriginPosition[3]       = { 0, 0, 0 };
int             g_nZOriginSet               = 0;
char            g_abortSearch               = 0;
#endif // FEATURE_FIND_Z_ORIGIN

#if FEATURE_RGB_LIGHT_EFFECTS
unsigned char   g_uRGBHeatingR              = RGB_HEATING_R;
unsigned char   g_uRGBHeatingG              = RGB_HEATING_G;
unsigned char   g_uRGBHeatingB              = RGB_HEATING_B;
unsigned char   g_uRGBPrintingR             = RGB_PRINTING_R;
unsigned char   g_uRGBPrintingG             = RGB_PRINTING_G;
unsigned char   g_uRGBPrintingB             = RGB_PRINTING_B;
unsigned char   g_uRGBCoolingR              = RGB_COOLING_R;
unsigned char   g_uRGBCoolingG              = RGB_COOLING_G;
unsigned char   g_uRGBCoolingB              = RGB_COOLING_B;
unsigned char   g_uRGBIdleR                 = RGB_IDLE_R;
unsigned char   g_uRGBIdleG                 = RGB_IDLE_G;
unsigned char   g_uRGBIdleB                 = RGB_IDLE_B;
unsigned char   g_uRGBManualR               = RGB_MANUAL_R;
unsigned char   g_uRGBManualG               = RGB_MANUAL_G;
unsigned char   g_uRGBManualB               = RGB_MANUAL_B;
volatile unsigned char  g_uRGBCurrentR              = 0;
volatile unsigned char  g_uRGBCurrentG              = 0;
volatile unsigned char  g_uRGBCurrentB              = 0;
unsigned char   g_uRGBTargetR               = 0;
unsigned char   g_uRGBTargetG               = 0;
unsigned char   g_uRGBTargetB               = 0;
#endif // FEATURE_RGB_LIGHT_EFFECTS

#if DEBUG_HEAT_BED_Z_COMPENSATION || DEBUG_WORK_PART_Z_COMPENSATION
volatile long   g_nLastZCompensationPositionSteps[3] = { 0, 0, 0 };
volatile long   g_nLastZCompensationTargetStepsZ     = 0;
volatile long   g_nZCompensationUpdates              = 0;
long            g_nDelta[2]                          = { 0, 0 };
long            g_nStepSize[2]                       = { 0, 0 };
long            g_nTempXFront                        = 0;
long            g_nTempXBack                         = 0;
long            g_nNeededZ                           = 0;
unsigned char   g_uIndex[4]                          = { 0, 0, 0, 0 };
short           g_nMatrix[4]                         = { 0, 0, 0, 0 };
long            g_nZDeltaMin                         = 100000;
long            g_nZDeltaMax                         = -100000;
long            g_nZCompensationUpdateTime           = 0;
volatile long   g_nZCompensationDelayMax             = 0;
long            g_nTooFast                           = 0;
#endif // DEBUG_HEAT_BED_Z_COMPENSATION || DEBUG_WORK_PART_Z_COMPENSATION

#if FEATURE_SERVICE_INTERVAL
unsigned long   g_nlastServiceTime  = 0;
int             g_nEnteredService   = 0;
#endif // FEATURE_SERVICE_INTERVAL


void initRF( void )
{
    // initialize the strain gauge
    initStrainGauge();

#if FEATURE_MILLING_MODE
    switchOperatingMode( Printer::operatingMode );
#else
    setupForPrinting();
#endif // FEATURE_MILLING_MODE

#if FEATURE_CONFIGURABLE_Z_ENDSTOPS
    if( Printer::ZEndstopType != ENDSTOP_TYPE_SINGLE )
    {
        if( Printer::isZMinEndstopHit() || Printer::isZMaxEndstopHit() )
        {
            // a z-endstop is active at the moment of the startup of the firmware, but both z-endstops are within one circuit so we do not know which one is the pressed one
            // in this situation we do not allow any moving into z-direction before a z-homing has been performed
            Printer::ZEndstopUnknown = 1;
        }
    }
#endif // FEATURE_CONFIGURABLE_Z_ENDSTOPS

#if FEATURE_SERVICE_INTERVAL
    float   fDistanceService     = Printer::filamentPrinted*0.001+HAL::eprGetFloat(EPR_PRINTING_DISTANCE_SERVICE);
    int32_t uSecondsServicePrint = ((HAL::timeInMilliseconds()-Printer::msecondsPrinting)/1000)+HAL::eprGetInt32(EPR_PRINTING_TIME_SERVICE);
    int32_t uHoursServicePrint   = uSecondsServicePrint/3600;
    int32_t uSecondsServiceMill  = ((HAL::timeInMilliseconds()-Printer::msecondsMilling)/1000)+HAL::eprGetInt32(EPR_MILLING_TIME_SERVICE);
    int32_t uHoursServiceMill    = uSecondsServiceMill/3600;
    
    if( fDistanceService >= FILAMENT_PRINTED_UNTIL_SERVICE || uHoursServicePrint >= HOURS_PRINTED_UNTIL_SERVICE || uHoursServiceMill >= HOURS_MILLED_UNTIL_SERVICE )
    {
        UI_STATUS( UI_TEXT_SERVICE );
        BEEP_SERVICE_INTERVALL
        g_nlastServiceTime = HAL::timeInMilliseconds();
    }

#endif // FEATURE_SERVICE_INTERVAL

    return;

} // initRF


void initStrainGauge( void )
{
    // configure DMS #1 (0x8C = single mode, 16 bits, gain = *1)
    Wire.beginTransmission( I2C_ADDRESS_STRAIN_GAUGE );
    Wire.write( 0x8C );
    Wire.endTransmission();
    return;

} // initStrainGauge


short readStrainGauge( unsigned char uAddress ) //readStrainGauge dauert etwas unter einer Millisekunde!
{
    unsigned char   Register;
    short           Result;

    Wire.beginTransmission( uAddress );
    Wire.requestFrom( (uint8_t)uAddress, (uint8_t)3 );
        
    Result =  Wire.read();
    Result =  Result << 8;
    Result += Wire.read();
        
    Register = Wire.read();
    (void)Register; //Nibbels: Tut so als würde die variable benutzt werden. Macht aber nix.
    Wire.endTransmission();

#if FEATURE_ZERO_DIGITS
    if(Printer::g_pressure_offset_active && -27768 < Result && Result < 27767){
        Result -= Printer::g_pressure_offset; //no overflow possible: pressure_offset ist 5000 max.
    }
#endif // FEATURE_ZERO_DIGITS


/* brief: This is for correcting sinking hotends at high digit values because of DMS-Sensor by Nibbels  */
#if FEATURE_DIGIT_Z_COMPENSATION
    static long nSensibleCompensationSum    = 0;
    static char nSensibleCompensationChecks = 0;
    if(Printer::doHeatBedZCompensation){ 
        //wenn ein retract stattfindet und das nicht echtzeit-schnell funktioniert, könnte es leichte probleme geben, aber prinzipiell wäre dann der Einfluss nicht so schädlich, wie ständig die digitabsenkung zu ignorieren.
        //das folgende sammelt immer 4 messwerte, schreibt den mittelwert raus und minimiert sammelwert und zähler auf 75%, dann den vierten wert neu in den topf..
        nSensibleCompensationSum     += Result;
        nSensibleCompensationChecks  += 1;
        if( nSensibleCompensationChecks == 4 ){
            InterruptProtectedBlock noInts;
            g_nDigitZCompensationDigits = (float)(nSensibleCompensationSum / 4); //*0.25, nachkommsstellen sind egal.
            noInts.unprotect();
            nSensibleCompensationSum = (long)g_nDigitZCompensationDigits * 3; //(nSensibleCompensationSum >> 1) + (nSensibleCompensationSum >> 2);--> sign-extension?? //nSensibleCompensationSum*0.75 
            nSensibleCompensationChecks -= 1; //*=0.75 bei 4 ist 3
#if FEATURE_DIGIT_FLOW_COMPENSATION
            if(g_nDigitFlowCompensation_intense != 0){
                short active_summed_digits = abs(static_cast<short>(g_nDigitZCompensationDigits));
                /*
                unter unterem digits limit: flow = 1.000
                zwischen beiden limits    : flow = 1.000 + anteil an maximaler auslenkung
                über oberem   digits limit: flow = 1.000 + maximale auslenkung plus oder minus
                */
                if(active_summed_digits <= g_nDigitFlowCompensation_Fmin){
                    g_nDigitFlowCompensation_flowmulti = 1.0f;
                }else if(active_summed_digits >= g_nDigitFlowCompensation_Fmax){
                    if( Printer::queuePositionCurrentSteps[Z_AXIS] > g_minZCompensationSteps - Extruder::current->zOffset ) 
                        g_nDigitFlowCompensation_flowmulti = 1.0f + 0.01f * g_nDigitFlowCompensation_intense;
                    else g_nDigitFlowCompensation_flowmulti = 1.0f;
                }else{
                    if( Printer::queuePositionCurrentSteps[Z_AXIS] > g_minZCompensationSteps - Extruder::current->zOffset ) 
                        g_nDigitFlowCompensation_flowmulti = 1.0f + 0.01f * g_nDigitFlowCompensation_intense
                                                                             *(active_summed_digits - g_nDigitFlowCompensation_Fmin)
                                                                             /(g_nDigitFlowCompensation_Fmax - g_nDigitFlowCompensation_Fmin);
                    else g_nDigitFlowCompensation_flowmulti = 1.0f;
                }
            }else{
                g_nDigitFlowCompensation_flowmulti = 1.0f;
            }
 #endif // FEATURE_DIGIT_FLOW_COMPENSATION
 
        }else{
#if FEATURE_DIGIT_FLOW_COMPENSATION
            g_nDigitFlowCompensation_flowmulti = 1.0f;
 #endif // FEATURE_DIGIT_FLOW_COMPENSATION
            InterruptProtectedBlock noInts;
            g_nDigitZCompensationDigits = (float)Result; //startwert / failwert
            noInts.unprotect();
        }
    }
#endif // FEATURE_DIGIT_Z_COMPENSATION
    g_nLastDigits = Result;
    return Result;
} // readStrainGauge


#if FEATURE_HEAT_BED_Z_COMPENSATION
void startHeatBedScan( void )
{
    if( g_nHeatBedScanStatus )
    {
        // abort the heat bed scan
        if( Printer::debugInfo() )
        {
            Com::printFLN( PSTR( "startHeatBedScan(): the scan has been cancelled" ) );
        }
        g_abortZScan = 1;
    }
    else
    {
        if( PrintLine::linesCount )
        {
            // there is some printing in progress at the moment - do not start the heat bed scan in this case
            if( Printer::debugErrors() )
            {
                Com::printFLN( PSTR( "startHeatBedScan(): the scan can not be started while the printing is in progress" ) );
            }

            showError( (void*)ui_text_heat_bed_scan, (void*)ui_text_operation_denied );
        }
        else
        {
            // start the heat bed scan
            g_nHeatBedScanStatus = 1;
            BEEP_START_HEAT_BED_SCAN

            // when the heat bed is scanned, the z-compensation must be disabled
            if( Printer::doHeatBedZCompensation )
            {
                if( Printer::debugInfo() )
                {
                    Com::printFLN( PSTR( "startHeatBedScan(): the z compensation has been disabled" ) );
                }
                resetZCompensation();
            }
        }
    }

    return;

} // startHeatBedScan


void scanHeatBed( void )
{
    if(g_ZOSScanStatus) return;

    static unsigned char    nIndexX;
    static unsigned char    nIndexY;
    static char             nIndexYDirection;
    static long             nX;
    static long             nY;
    static long             nZ;
    static long             nYDirection;
#if DEBUG_HEAT_BED_SCAN
    static short            nContactPressure;
#endif // DEBUG_HEAT_BED_SCAN
    //unsigned char         nLastHeatBedScanStatus = g_nHeatBedScanStatus;
    short                   nTempPressure;
    long                    nTempPosition;

    // directions:
    // +x = to the right
    // -x = to the left
    // +y = heat bed moves to the front
    // -y = heat bed moves to the back
    // +z = heat bed moves down
    // -z = heat bed moves up

    if( g_abortZScan )
    {
        // the scan has been aborted
        g_abortZScan = 0;

        // avoid to crash the extruder against the heat bed during the following homing
        g_nZScanZPosition += moveZ( int(Printer::axisStepsPerMM[Z_AXIS] *5) );

        // start at the home position
        Printer::homeAxis( true, true, true );

        // turn off the engines
        Printer::disableXStepper();
        Printer::disableYStepper();
        Printer::disableZStepper();

        // disable all heaters
        Extruder::setHeatedBedTemperature( 0, false );
        Extruder::setTemperatureForExtruder( 0, 0, false );
#if NUM_EXTRUDER == 2
        Extruder::setTemperatureForExtruder( 0, 1, false );
#endif // NUM_EXTRUDER == 2

        if( Printer::debugInfo() )
        {
            Com::printF( Com::tscanHeatBed );
            Com::printFLN( PSTR( "the scan has been aborted" ) );
        }

        UI_STATUS_UPD( UI_TEXT_HEAT_BED_SCAN_ABORTED );
        BEEP_ABORT_HEAT_BED_SCAN

        // restore the compensation values from the EEPROM
        if( loadCompensationMatrix( (unsigned int)(EEPROM_SECTOR_SIZE * g_nActiveHeatBed) ) )
        {
            // there is no valid compensation matrix available
            initCompensationMatrix();
        }

        g_nHeatBedScanStatus  = 0;
        g_nZScanZPosition     = 0;
        g_nLastZScanZPosition = 0;
        g_retryZScan          = 0;
        g_retryStatus         = 0;
        return;
    }

    // show that we are active
    previousMillisCmd = HAL::timeInMilliseconds();

    if( g_nHeatBedScanStatus )
    {
        if( g_nHeatBedScanStatus != 15 &&
            g_nHeatBedScanStatus != 20 &&
            g_nHeatBedScanStatus != 22 &&
            g_nHeatBedScanStatus != 123 &&
            g_nHeatBedScanStatus != 125 &&
            g_nHeatBedScanStatus != 136 &&
            g_nHeatBedScanStatus != 137 )
        {
            // there are a few cases where we do not want to change the current status text
            UI_STATUS( UI_TEXT_HEAT_BED_SCAN );
        }

        if( g_retryZScan )
        {
            // we have to retry to scan the current position
            g_nHeatBedScanStatus = g_retryStatus;
            g_retryZScan         = 0;

#if DEBUG_HEAT_BED_SCAN == 2
            if( Printer::debugInfo() )
            {
                Com::printF( Com::tscanHeatBed );
                Com::printFLN( PSTR( "retry -> " ), g_retryStatus );
            }
#endif // DEBUG_HEAT_BED_SCAN == 2
        }

        switch( g_nHeatBedScanStatus )
        {
            case 1:
            {
                g_scanStartTime    = HAL::timeInMilliseconds();
                g_abortZScan       = 0;
#if DEBUG_HEAT_BED_SCAN
                nContactPressure   = 0;
#endif // DEBUG_HEAT_BED_SCAN
                g_nTempDirectionZ  = 0;
                g_retryStatus      = 0;

                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "the scan has been started" ) );
                }

                // clear all fields of the heat bed compensation matrix
                initCompensationMatrix();

                g_uZMatrixMax[X_AXIS] =
                g_uZMatrixMax[Y_AXIS] = 0;

                // output the currently used scan parameters
                outputScanParameters();

                g_nHeatBedScanStatus = 10;

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "1 -> 10" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 10:
            {
                // the scan is performed with the left extruder
                Extruder::selectExtruderById( 0 );

#if FEATURE_PRECISE_HEAT_BED_SCAN
                if( g_nHeatBedScanMode == HEAT_BED_SCAN_MODE_PLA )
                {
                    Extruder::setHeatedBedTemperature( PRECISE_HEAT_BED_SCAN_BED_TEMP_PLA, false);
                    Extruder::setTemperatureForExtruder( PRECISE_HEAT_BED_SCAN_EXTRUDER_TEMP_SCAN, 0, false);

#if NUM_EXTRUDER == 2
                    Extruder::setTemperatureForExtruder( PRECISE_HEAT_BED_SCAN_EXTRUDER_TEMP_SCAN, 1, false);
#endif // NUM_EXTRUDER == 2
                }
                else if( g_nHeatBedScanMode == HEAT_BED_SCAN_MODE_ABS )
                {
                    Extruder::setHeatedBedTemperature( PRECISE_HEAT_BED_SCAN_BED_TEMP_ABS, false);
                    Extruder::setTemperatureForExtruder( PRECISE_HEAT_BED_SCAN_EXTRUDER_TEMP_SCAN, 0, false);

#if NUM_EXTRUDER == 2
                    Extruder::setTemperatureForExtruder( PRECISE_HEAT_BED_SCAN_EXTRUDER_TEMP_SCAN, 1, false);
#endif // NUM_EXTRUDER == 2
                }
#endif // FEATURE_PRECISE_HEAT_BED_SCAN

                g_nHeatBedScanStatus = 15;
                g_lastScanTime       = HAL::timeInMilliseconds();

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printFLN( PSTR( "scanHeatBed(): 10 -> 15" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 15:
            {
                if( (HAL::timeInMilliseconds() - g_lastScanTime) < HEAT_BED_SCAN_DELAY )
                {
                    // do not check too often
                    break;
                }

                if( testHeatBedTemperature() )
                {
                    // we did not reach the proper temperature
                    g_lastScanTime = HAL::timeInMilliseconds();
                    break;
                }
                g_nHeatBedScanStatus = 20;
                g_lastScanTime       = HAL::timeInMilliseconds();

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "15 -> 20" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 20:
            {
                if( (HAL::timeInMilliseconds() - g_lastScanTime) < HEAT_BED_SCAN_DELAY )
                {
                    // do not check too often
                    break;
                }

                if( testExtruderTemperature() )
                {
                    // we did not reach the proper temperature
                    g_lastScanTime = HAL::timeInMilliseconds();
                    break;
                }
                g_nHeatBedScanStatus = 22;
                g_lastScanTime       = HAL::timeInMilliseconds();

#if FEATURE_PRECISE_HEAT_BED_SCAN
                if ( g_nHeatBedScanMode )
                {
                    if( Printer::debugInfo() )
                    {
                        Com::printF( Com::tscanHeatBed );
                        Com::printFLN( PSTR( "warmup delay [s] = " ), PRECISE_HEAT_BED_SCAN_WARMUP_DELAY );
                    }
                }
#endif // FEATURE_PRECISE_HEAT_BED_SCAN

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "20 -> 22" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 22:
            {
#if FEATURE_PRECISE_HEAT_BED_SCAN
                if ( g_nHeatBedScanMode )
                {
                    // wait some time so that the desired target temperature is reached in all parts of our components
                    if( (HAL::timeInMilliseconds() - g_lastScanTime) < (PRECISE_HEAT_BED_SCAN_WARMUP_DELAY * 1000) )
                    {
                        UI_STATUS_UPD( UI_TEXT_HEATING ); 
                        break;
                    }
                }
#endif // FEATURE_PRECISE_HEAT_BED_SCAN

                // start at the home position
                Printer::homeAxis( true, true, true );
                Commands::waitUntilEndOfAllMoves();

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "move = " ), HEAT_BED_SCAN_Z_START_STEPS );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2

                // move a bit away from the heat bed in order to achieve better measurements in case of hardware configurations where the extruder is very close to the heat bed after the z-homing
                g_nZScanZPosition += moveZ( HEAT_BED_SCAN_Z_START_STEPS );

                g_nHeatBedScanStatus = 25;
                g_lastScanTime       = HAL::timeInMilliseconds();

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "22 -> 25" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 25:
            {
                // move to the first position
                PrintLine::moveRelativeDistanceInSteps( g_nScanXStartSteps, 0, 0, 0, MAX_FEEDRATE_X, true, true );
                PrintLine::moveRelativeDistanceInSteps( 0, g_nScanYStartSteps, 0, 0, MAX_FEEDRATE_Y, true, true );

                g_nHeatBedScanStatus = 30;
                g_lastScanTime       = HAL::timeInMilliseconds();

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "25 -> 30" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 30:
            {
                if( (HAL::timeInMilliseconds() - g_lastScanTime) < HEAT_BED_SCAN_DELAY )
                {
                    // do not check too early
                    break;
                }

                if( readIdlePressure( &g_nFirstIdlePressure ) )
                {
                    // we were unable to determine the idle pressure
                    break;
                }

                g_nHeatBedScanStatus = 35;
                g_lastScanTime       = HAL::timeInMilliseconds();

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "30 -> 35" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 35:
            {
                nX               = g_nScanXStartSteps;
                nY               = g_nScanYStartSteps;
                nZ               = HEAT_BED_SCAN_Z_START_STEPS;
                nYDirection      = g_nScanYStepSizeSteps;   // we start to move the heat bed from the back to the front
                nIndexYDirection = 1;
                nIndexX          = 2;
                nIndexY          = 2;

                g_nMinPressureContact = g_nFirstIdlePressure - g_nScanContactPressureDelta;
                g_nMaxPressureContact = g_nFirstIdlePressure + g_nScanContactPressureDelta;
                g_nMinPressureRetry   = g_nFirstIdlePressure - g_nScanRetryPressureDelta;
                g_nMaxPressureRetry   = g_nFirstIdlePressure + g_nScanRetryPressureDelta;
                g_nMinPressureIdle    = g_nFirstIdlePressure - g_nScanIdlePressureDelta;
                g_nMaxPressureIdle    = g_nFirstIdlePressure + g_nScanIdlePressureDelta;

                // store also the version of this heat bed compensation matrix
#if DEBUG_REMEMBER_SCAN_PRESSURE
                g_ScanPressure[0][0]        = EEPROM_FORMAT;
#endif // DEBUG_REMEMBER_SCAN_PRESSURE

                g_ZCompensationMatrix[0][0] = EEPROM_FORMAT;

                g_nHeatBedScanStatus = 40;

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "35 -> 40" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 39:
            {
                nTempPosition = nX + g_nScanXStepSizeSteps;
                if( nTempPosition > g_nScanXMaxPositionSteps )
                {
                    // we end up here when the scan is complete
                    g_nHeatBedScanStatus = 60;

#if DEBUG_HEAT_BED_SCAN == 2
                    if( Printer::debugInfo() )
                    {
                        Com::printF( Com::tscanHeatBed );
                        Com::printFLN( PSTR( "39 -> 60" ) );
                    }
#endif // DEBUG_HEAT_BED_SCAN == 2
                    break;
                }

                // move to the next x-position
                PrintLine::moveRelativeDistanceInSteps( g_nScanXStepSizeSteps, 0, 0, 0, MAX_FEEDRATE_X, true, true );
                nX += g_nScanXStepSizeSteps;
                nIndexX ++;

                if( nIndexX > COMPENSATION_MATRIX_MAX_X )
                {
#if DEBUG_HEAT_BED_SCAN == 2
                    if( Printer::debugErrors() )
                    {
                        Com::printF( Com::tscanHeatBed );
                        Com::printFLN( PSTR( "x-dimension of the z matrix became too big: " ), nIndexX );
                    }
#endif // DEBUG_HEAT_BED_SCAN == 2
                    g_abortZScan = 1;
                    break;
                }

                if( nYDirection > 0 )
                {
                    // we were moving from the front to the back during this column, so we have to move from the back to the front during the next column
                    nYDirection      = -g_nScanYStepSizeSteps;  // we start to move the heat bed from the back to the front
                    nIndexYDirection = -1;
                }
                else
                {
                    // we were moving from the back to the front during this column, so we have to move from the front to the back during the next column
                    nYDirection      = g_nScanYStepSizeSteps;   // we start to move the heat bed from the back to the front
                    nIndexYDirection = 1;
                }

                g_nHeatBedScanStatus = 40;

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "39 -> 40" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 40:
            {
                // safety checks
                if( nX <= g_nScanXMaxPositionSteps )
                {
                    // remember also the exact x-position of this row/column
#if DEBUG_REMEMBER_SCAN_PRESSURE
                    g_ScanPressure[nIndexX][0] = nX;
#endif // DEBUG_REMEMBER_SCAN_PRESSURE

                    g_ZCompensationMatrix[nIndexX][0] = (short)((float)nX / Printer::axisStepsPerMM[X_AXIS] + 0.5); // convert to mm

                    g_nHeatBedScanStatus = 49;
                    g_lastScanTime       = HAL::timeInMilliseconds();

#if DEBUG_HEAT_BED_SCAN == 2
                    if( Printer::debugInfo() )
                    {
                        Com::printF( Com::tscanHeatBed );
                        Com::printFLN( PSTR( "40 -> 49" ) );
                    }
#endif // DEBUG_HEAT_BED_SCAN == 2
                    break;
                }

                // we end up here when the scan is complete
                g_nHeatBedScanStatus = 60;

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "40 -> 60" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 45: //case kommt beim HBS nicht vor??? doch, als retryStatus von 49 aus.
            {
                while( Printer::isZMinEndstopHit() || Printer::isZMaxEndstopHit() ) //sollte inzwischen egal (??) sein, wir fahren immer automatisch per homing aus dem endstop heraus.
                {
                    // ensure that there is no z endstop hit before we perform the z-axis homing
                    moveZ( int(Printer::axisStepsPerMM[Z_AXIS] *2) );
                }

                // home the z-axis in order to find the starting point again
                Printer::homeAxis( false, false, true ); //Nibbels: wäre schädlich für zu hohe druckbetten.
                //TODO: Home Y+Z, goto Z=HEAT_BED_SCAN_Z_START_STEPS, goto Y, continue. für Case 45 105 139 + 132
                g_scanRetries        --;
                g_nZScanZPosition    = 0;
                nZ                   = 0;
                g_nHeatBedScanStatus = 50;
                g_lastScanTime       = HAL::timeInMilliseconds();

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "45 -> 50" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 49:
            {
                g_scanRetries        = HEAT_BED_SCAN_RETRIES;
                g_retryStatus        = 45;
                g_nHeatBedScanStatus = 50;

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "49 -> 50" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 50:
            {
                if( (HAL::timeInMilliseconds() - g_lastScanTime) < g_nScanIdleDelay )
                {
                    // do not check too early
                    break;
                }

                // scan this point
                if( testIdlePressure() )
                {
                    // the current idle pressure is not plausible
                    g_abortZScan = 1;
                    break;
                }

                // we should consider that the idle presse can change slightly
                g_nMinPressureContact = g_nCurrentIdlePressure - g_nScanContactPressureDelta;
                g_nMaxPressureContact = g_nCurrentIdlePressure + g_nScanContactPressureDelta;
                g_nMinPressureRetry   = g_nCurrentIdlePressure - g_nScanRetryPressureDelta;
                g_nMaxPressureRetry   = g_nCurrentIdlePressure + g_nScanRetryPressureDelta;
                g_nMinPressureIdle    = g_nCurrentIdlePressure - g_nScanIdlePressureDelta;
                g_nMaxPressureIdle    = g_nCurrentIdlePressure + g_nScanIdlePressureDelta;

                g_nHeatBedScanStatus = 51;

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "50 -> 51" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 51:
            {
                // move fast to the surface
                nZ += moveZUpFast();
                g_nZScanZPosition = nZ;

                g_nHeatBedScanStatus = 52;

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "51 -> 52" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 52:
            {
                // move a little bit away from the surface
                nZ += moveZDownSlow();
                g_nZScanZPosition = nZ;

                g_nHeatBedScanStatus = 53;

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "52 -> 53" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 53:
            {
                // move slowly to the surface
                nZ += moveZUpSlow( &nTempPressure );
                g_nZScanZPosition = nZ;
#if DEBUG_HEAT_BED_SCAN
                nContactPressure  = nTempPressure;
#endif // DEBUG_HEAT_BED_SCAN

                g_nHeatBedScanStatus = 54;

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "53 -> 54" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 54:
            {
#if DEBUG_HEAT_BED_SCAN
                if( Printer::debugInfo() )
                {
                    Com::printF( PSTR( "nX;" ), nX );
                    Com::printF( Com::tSemiColon, (float)nX / Printer::axisStepsPerMM[X_AXIS] );
                    Com::printF( PSTR( ";nY;" ), nY );
                    Com::printF( Com::tSemiColon, (float)nY / Printer::axisStepsPerMM[Y_AXIS] );
                    Com::printF( PSTR( ";nZ;" ), nZ );
                    Com::printF( Com::tSemiColon, (float)nZ / Printer::axisStepsPerMM[Z_AXIS] );
                    Com::printF( PSTR( ";Pressure;" ), nContactPressure );

                    Com::printF( PSTR( ";nIndexX;" ), (int)nIndexX );
                    Com::printF( PSTR( ";nIndexY;" ), (int)nIndexY );

/*                  // output the non compensated position values
                    Com::printF( PSTR( ";;" ), Printer::queuePositionCurrentSteps[X_AXIS] );
                    Com::printF( Com::tSemiColon, Printer::queuePositionCurrentSteps[Y_AXIS] );
                    Com::printF( Com::tSemiColon, Printer::queuePositionCurrentSteps[Z_AXIS] );
                    Com::printF( Com::tSemiColon, Printer::compensatedPositionCurrentStepsZ );
*/
                    Com::printFLN( PSTR( " " ) );
                }
#endif // DEBUG_HEAT_BED_SCAN

                // remember the z-position and the exact y-position of this row/column
                g_ZCompensationMatrix[nIndexX][nIndexY] = (short)nZ;
                g_ZCompensationMatrix[0][nIndexY]       = (short)((float)nY / Printer::axisStepsPerMM[Y_AXIS] + 0.5);   // convert to mm

#if DEBUG_REMEMBER_SCAN_PRESSURE
                // remember the pressure and the exact y-position of this row/column
                g_ScanPressure[nIndexX][nIndexY] = nContactPressure;
                g_ScanPressure[0][nIndexY]       = nY;
#endif // DEBUG_REMEMBER_SCAN_PRESSURE

                if( nIndexX > g_uZMatrixMax[X_AXIS] )
                {
                    g_uZMatrixMax[X_AXIS] = nIndexX;
                }

                if( nIndexY > g_uZMatrixMax[Y_AXIS] )
                {
                    g_uZMatrixMax[Y_AXIS] = nIndexY;
                }
        
                g_nHeatBedScanStatus = 55;

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "54 -> 55" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 55:
            {
                // move away from the surface
                nZ += moveZDownFast();
                g_nZScanZPosition = nZ;

                if( nYDirection > 0 )
                {
                    nTempPosition = nY+nYDirection;

                    if( nTempPosition > g_nScanYMaxPositionSteps )
                    {
                        // we have reached the end of this column
                        g_nHeatBedScanStatus = 39;

#if DEBUG_HEAT_BED_SCAN == 2
                        if( Printer::debugInfo() )
                        {
                            Com::printF( Com::tscanHeatBed );
                            Com::printFLN( PSTR( "55 -> 39" ) );
                        }
#endif // DEBUG_HEAT_BED_SCAN == 2
                        break;
                    }
                }
                else
                {
                    nTempPosition = nY+nYDirection;

                    if( nTempPosition < g_nScanYStartSteps )
                    {
                        // we have reached the end of this column
                        g_nHeatBedScanStatus = 39;

#if DEBUG_HEAT_BED_SCAN == 2
                        if( Printer::debugInfo() )
                        {
                            Com::printF( Com::tscanHeatBed );
                            Com::printFLN( PSTR( "55 -> 39" ) );
                        }
#endif // DEBUG_HEAT_BED_SCAN == 2
                        break;
                    }
                }

                // move to the next y-position
                PrintLine::moveRelativeDistanceInSteps( 0, nYDirection, 0, 0, MAX_FEEDRATE_Y, true, true );
                nY      += nYDirection;
                nIndexY += nIndexYDirection;

                if( nIndexY > COMPENSATION_MATRIX_MAX_Y )
                {
#if DEBUG_HEAT_BED_SCAN == 2
                    if( Printer::debugErrors() )
                    {
                        Com::printF( Com::tscanHeatBed );
                        Com::printFLN( PSTR( "y-dimension of the z matrix became too big: " ), nIndexY );
                    }
#endif // DEBUG_HEAT_BED_SCAN == 2
                    g_abortZScan = 1;
                    break;
                }

                g_nHeatBedScanStatus = 49;
                g_lastScanTime       = HAL::timeInMilliseconds();

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "55 -> 49" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 60:
            {
                // avoid to crash the extruder against the heat bed during the following homing
                g_nZScanZPosition += moveZ( int(Printer::axisStepsPerMM[Z_AXIS] *5) );

                // move back to the home position
                Printer::homeAxis( true, true, true);
                g_nZScanZPosition =
                nZ                = 0;

#if FEATURE_PRECISE_HEAT_BED_SCAN
                if ( !g_nHeatBedScanMode )
                {
                    // disable all heaters
                    Extruder::setHeatedBedTemperature( 0, false );
                    Extruder::setTemperatureForExtruder( 0, 0, false );
#if NUM_EXTRUDER > 1
                    Extruder::setTemperatureForExtruder( 0, 1, false );
#endif // NUM_EXTRUDER > 1
                }
#else
                // disable all heaters
                Extruder::setHeatedBedTemperature( 0, false );
                Extruder::setTemperatureForExtruder( 0, 0, false );

#if NUM_EXTRUDER > 1
                Extruder::setTemperatureForExtruder( 0, 1, false );
#endif // NUM_EXTRUDER > 1
#endif // FEATURE_PRECISE_HEAT_BED_SCAN

                g_nHeatBedScanStatus = 65;

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "60 -> 65" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 65:
            {
                if( Printer::debugInfo() )
                {
                    // output the determined compensation
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "raw heat bed z matrix: " ) );
                    determineCompensationOffsetZ();
                    outputCompensationMatrix();
                }

                g_nHeatBedScanStatus = 70;

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "65 -> 70" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 70:
            {
                // output the determined pressure
                outputPressureMatrix();

                g_nHeatBedScanStatus = 75;

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "70 -> 75" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 75:
            {
                if( Printer::debugInfo() )
                {
                    // output the pure scan time
                    Com::printF( Com::tscanHeatBed );
                    Com::printF( PSTR( "total scan time: " ), long((HAL::timeInMilliseconds() - g_scanStartTime) / 1000) );
                    Com::printFLN( PSTR( " [s]" ) );
                }

                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "g_uZMatrixMax[Y_AXIS].1 = " ), (int)g_uZMatrixMax[Y_AXIS] );
                }

                // prepare the heat bed compensation matrix for fast usage during the actual printing
                prepareCompensationMatrix();

                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "g_uZMatrixMax[Y_AXIS].2 = " ), (int)g_uZMatrixMax[Y_AXIS] );

                    // output the converted heat bed compensation matrix
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "converted heat bed z matrix: " ) );
                    outputCompensationMatrix();
                }

                g_nHeatBedScanStatus = 80;
                g_lastScanTime       = HAL::timeInMilliseconds();

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "75 -> 80" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2              
                break;
            }
            case 80:
            {
                if( (HAL::timeInMilliseconds() - g_lastScanTime) < HEAT_BED_SCAN_DELAY )
                {
                    // do not check too early
                    break;
                }

                // compare the idle pressure at the beginning and at the end
                readAveragePressure( &nTempPressure );

                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "idle pressure at start: " ), g_nFirstIdlePressure );
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "idle pressure at stop: " ), nTempPressure );
                }

#if NUM_EXTRUDER == 2
                // we have to align the two extruders
                g_nHeatBedScanStatus = 100;

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "80 -> 100" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2              
#else
#if FEATURE_PRECISE_HEAT_BED_SCAN
                if ( g_nHeatBedScanMode )
                {
                    // we have to determine the z-offset which is caused by different extruder temperatures
                    g_nHeatBedScanStatus = 130;

#if DEBUG_HEAT_BED_SCAN == 2
                    if( Printer::debugInfo() )
                    {
                        Com::printF( Com::tscanHeatBed );
                        Com::printFLN( PSTR( "80 -> 130" ) );
                    }
#endif // DEBUG_HEAT_BED_SCAN == 2
                }
                else
#endif //FEATURE_PRECISE_HEAT_BED_SCAN
                {
                    // we are done
                    g_nHeatBedScanStatus = 150;

#if DEBUG_HEAT_BED_SCAN == 2
                    if( Printer::debugInfo() )
                    {
                        Com::printF( Com::tscanHeatBed );
                        Com::printFLN( PSTR( "80 -> 150" ) );
                    }
#endif // DEBUG_HEAT_BED_SCAN == 2
                }
#endif // NUM_EXTRUDER == 2
                break;
            }
            case 100:
            {
                // we are homed at the moment - move to the position where both extruders shall be aligned to the same z position
                PrintLine::moveRelativeDistanceInSteps( HEAT_BED_SCAN_X_CALIBRATION_POINT_STEPS, HEAT_BED_SCAN_Y_CALIBRATION_POINT_STEPS, 0, 0, MAX_FEEDRATE_X, true, true );

                g_lastScanTime       = HAL::timeInMilliseconds();
                g_scanRetries        = HEAT_BED_SCAN_RETRIES;
                g_retryStatus        = 105;
                g_nHeatBedScanStatus = 110;

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "100 -> 110" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 105:  //retryStatus von 100 aus.
            {
                while( Printer::isZMinEndstopHit() || Printer::isZMaxEndstopHit() ) //sollte inzwischen egal (??) sein, wir fahren immer automatisch per homing aus dem endstop heraus.
                {
                    // ensure that there is no z endstop hit before we perform the z-axis homing
                    moveZ( int(Printer::axisStepsPerMM[Z_AXIS] *2) );
                }

                // home the z-axis in order to find the starting point again
                Printer::homeAxis( false, false, true );  //Nibbels: wäre schädlich für zu hohe druckbetten.
                //TODO: Home Y+Z, goto Z=HEAT_BED_SCAN_Z_START_STEPS, goto Y, continue. für Case 45 105 139 + 132
                g_scanRetries        --;
                g_nZScanZPosition    = 0;
                nZ                   = 0;
                g_nHeatBedScanStatus = 110;
                g_lastScanTime       = HAL::timeInMilliseconds();

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "105 -> 110" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 110:
            {
                if( (HAL::timeInMilliseconds() - g_lastScanTime) < g_nScanIdleDelay )
                {
                    // do not check too early
                    break;
                }

                // scan this point
                if( testIdlePressure() )
                {
                    // the current idle pressure is not plausible
                    g_abortZScan = 1;
                    break;
                }

                // we should consider that the idle presse can change slightly
                g_nMinPressureContact = g_nCurrentIdlePressure - g_nScanContactPressureDelta;
                g_nMaxPressureContact = g_nCurrentIdlePressure + g_nScanContactPressureDelta;
                g_nMinPressureRetry   = g_nCurrentIdlePressure - g_nScanRetryPressureDelta;
                g_nMaxPressureRetry   = g_nCurrentIdlePressure + g_nScanRetryPressureDelta;
                g_nMinPressureIdle    = g_nCurrentIdlePressure - g_nScanIdlePressureDelta;
                g_nMaxPressureIdle    = g_nCurrentIdlePressure + g_nScanIdlePressureDelta;

                g_nHeatBedScanStatus = 120;

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "110 -> 120" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 120:
            {
                // move to the surface
                nZ += moveZUpFast();
                g_nZScanZPosition = nZ;

                g_nHeatBedScanStatus = 121;

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "120 -> 121" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 121:
            {
                // ensure that we do not remember any previous z-position at this moment
                g_nLastZScanZPosition = 0;

                // move a little bit away from the surface
                nZ += moveZDownSlow();
                g_nZScanZPosition = nZ;

                g_nHeatBedScanStatus = 122;

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "121 -> 122" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 122:
            {
                // move slowly to the surface
                nZ += moveZUpSlow( &nTempPressure );
                g_nZScanZPosition = nZ;

                g_nHeatBedScanStatus = 123;

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "122 -> 123" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 123:
            {
                // the left extruder is at the surface now - show that the user must move also the right extruder to the surface in order to get them to the same z-height
                UI_STATUS_UPD( UI_TEXT_ALIGN_EXTRUDERS );
                BEEP_ALIGN_EXTRUDERS

                g_nContinueButtonPressed = 0;
                g_nHeatBedScanStatus     = 125;

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "123 -> 125" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 125:
            {
                // wait until the continue button has been pressed
                if( !g_nContinueButtonPressed )
                {
                    break;
                }

#if FEATURE_PRECISE_HEAT_BED_SCAN
                if ( g_nHeatBedScanMode )
                {
                    // we have to determine the z-offset which is caused by different extruder temperatures
                    g_nHeatBedScanStatus = 132;

#if DEBUG_HEAT_BED_SCAN == 2
                    if( Printer::debugInfo() )
                    {
                        Com::printF( Com::tscanHeatBed );
                        Com::printFLN( PSTR( "125 -> 132" ) );
                    }
#endif // DEBUG_HEAT_BED_SCAN == 2
                }
                else
#endif //FEATURE_PRECISE_HEAT_BED_SCAN
                {
                    // we are done
                    g_nHeatBedScanStatus = 145;

#if DEBUG_HEAT_BED_SCAN == 2
                    if( Printer::debugInfo() )
                    {
                        Com::printF( Com::tscanHeatBed );
                        Com::printFLN( PSTR( "125 -> 145" ) );
                    }
#endif // DEBUG_HEAT_BED_SCAN == 2
                }
                break;
            }
            case 130:
            {
                // we are homed at the moment - move to the position where we shall determine the length offset which is caused by the heated up extruder 
                PrintLine::moveRelativeDistanceInSteps( HEAT_BED_SCAN_X_CALIBRATION_POINT_STEPS, HEAT_BED_SCAN_Y_CALIBRATION_POINT_STEPS, 0, 0, MAX_FEEDRATE_X, true, true );

                g_lastScanTime       = HAL::timeInMilliseconds();
                g_nHeatBedScanStatus = 135;

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "130 -> 135" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 132:
            {
                // determine the z-home position
                Printer::homeAxis( false, false, true); //Nibbels: wäre schädlich für zu hohe druckbetten.
                g_nZScanZPosition =
                nZ                = 0;

                g_lastScanTime       = HAL::timeInMilliseconds();
                g_nHeatBedScanStatus = 133;

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "132 -> 133" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 133:
            {
                // move the heat bed 5mm down
                g_nZScanZPosition += moveZ( int(Printer::axisStepsPerMM[Z_AXIS] *5) );
                g_lastScanTime       = HAL::timeInMilliseconds();
                g_nHeatBedScanStatus = 134;

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "133 -> 134" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 134:
            {
                // move the heat bed 5mm down
                g_nZScanZPosition += moveZ( int(Printer::axisStepsPerMM[Z_AXIS] *5) );
                g_lastScanTime       = HAL::timeInMilliseconds();
                g_nHeatBedScanStatus = 135;

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "134 -> 135" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 135:
            {
                // at this point we are homed and we are above the x/y position at which we shall perform the measurement of the z-offset with the hot extruder(s)
#if FEATURE_PRECISE_HEAT_BED_SCAN
                if ( g_nHeatBedScanMode == HEAT_BED_SCAN_MODE_PLA )
                {
                    Extruder::setTemperatureForExtruder( PRECISE_HEAT_BED_SCAN_EXTRUDER_TEMP_PLA, 0, false);
#if NUM_EXTRUDER == 2
                    Extruder::setTemperatureForExtruder( PRECISE_HEAT_BED_SCAN_EXTRUDER_TEMP_PLA, 1, false);
#endif // NUM_EXTRUDER == 2
                }
                else if ( g_nHeatBedScanMode == HEAT_BED_SCAN_MODE_ABS )
                {
                    Extruder::setTemperatureForExtruder( PRECISE_HEAT_BED_SCAN_EXTRUDER_TEMP_ABS, 0, false);
#if NUM_EXTRUDER == 2
                    Extruder::setTemperatureForExtruder( PRECISE_HEAT_BED_SCAN_EXTRUDER_TEMP_ABS, 1, false);
#endif // NUM_EXTRUDER == 2
                }
#endif // FEATURE_PRECISE_HEAT_BED_SCAN

                g_nHeatBedScanStatus = 136;

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "135 -> 136" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 136:
            {
                if( (HAL::timeInMilliseconds() - g_lastScanTime) < HEAT_BED_SCAN_DELAY )
                {
                    // do not check too often
                    break;
                }

                if( testExtruderTemperature() )
                {
                    // we did not reach the proper temperature
                    g_lastScanTime = HAL::timeInMilliseconds();
                    break;
                }

                g_nHeatBedScanStatus = 137;
                g_lastScanTime       = HAL::timeInMilliseconds();

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "136 -> 137" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2

#if FEATURE_PRECISE_HEAT_BED_SCAN
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "calibration delay [s] =  " ), (uint32_t)PRECISE_HEAT_BED_SCAN_CALIBRATION_DELAY );
                }
#endif // FEATURE_PRECISE_HEAT_BED_SCAN
                break;
            }
            case 137:
            {
#if FEATURE_PRECISE_HEAT_BED_SCAN
                if ( g_nHeatBedScanMode )
                {
                    // wait some time so that the desired target temperature is reached in all parts of our components
                    if( (HAL::timeInMilliseconds() - g_lastScanTime) < (PRECISE_HEAT_BED_SCAN_CALIBRATION_DELAY * 1000) )
                    {
                        UI_STATUS_UPD( UI_TEXT_HEATING );
                        break;
                    }
                }
#endif // FEATURE_PRECISE_HEAT_BED_SCAN

                g_scanRetries        = HEAT_BED_SCAN_RETRIES;
                g_retryStatus        = 139;
                g_nHeatBedScanStatus = 140;
                g_lastScanTime       = HAL::timeInMilliseconds();

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "137 -> 140" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 139:   //retryStatus von 137 aus.
            {
                while( Printer::isZMinEndstopHit() || Printer::isZMaxEndstopHit() ) //sollte inzwischen egal (??) sein, wir fahren immer automatisch per homing aus dem endstop heraus.
                {
                    // ensure that there is no z endstop hit before we perform the z-axis homing
                    moveZ( int(Printer::axisStepsPerMM[Z_AXIS] *2) );
                }

                // home the z-axis in order to find the starting point again
                Printer::homeAxis( false, false, true ); //Nibbels: wäre schädlich für zu hohe druckbetten.
                //TODO: Home Y+Z, goto Z=HEAT_BED_SCAN_Z_START_STEPS, goto Y, continue. für Case 45 105 139 + 132
                g_scanRetries        --;
                g_nZScanZPosition    = 0;
                nZ                   = 0;
                g_nHeatBedScanStatus = 140;
                g_lastScanTime       = HAL::timeInMilliseconds();

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "139 -> 140" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 140:
            {
                if( (HAL::timeInMilliseconds() - g_lastScanTime) < g_nScanIdleDelay )
                {
                    // do not check too early
                    break;
                }

                // scan this point
                if( testIdlePressure() )
                {
                    // the current idle pressure is not plausible
                    g_abortZScan = 1;
                    break;
                }

                // we should consider that the idle presse can change slightly
                g_nMinPressureContact = g_nCurrentIdlePressure - g_nScanContactPressureDelta;
                g_nMaxPressureContact = g_nCurrentIdlePressure + g_nScanContactPressureDelta;
                g_nMinPressureRetry   = g_nCurrentIdlePressure - g_nScanRetryPressureDelta;
                g_nMaxPressureRetry   = g_nCurrentIdlePressure + g_nScanRetryPressureDelta;
                g_nMinPressureIdle    = g_nCurrentIdlePressure - g_nScanIdlePressureDelta;
                g_nMaxPressureIdle    = g_nCurrentIdlePressure + g_nScanIdlePressureDelta;

                g_nHeatBedScanStatus = 141;

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "140 -> 141" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 141:
            {
                // move to the surface
                nZ += moveZUpFast();
                g_nZScanZPosition = nZ;

                g_nHeatBedScanStatus = 142;

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "141 -> 142" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 142:
            {
                // ensure that we do not remember any previous z-position at this moment
                g_nLastZScanZPosition = 0;

                // move a little bit away from the surface
                nZ += moveZDownSlow();
                g_nZScanZPosition = nZ;

                g_nHeatBedScanStatus = 143;

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "142 -> 143" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 143:
            {
                // move slowly to the surface
                nZ += moveZUpSlow( &nTempPressure );
                g_nZScanZPosition = nZ;

                g_nHeatBedScanStatus = 144;

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "143 -> 144" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 144:
            {
                // adjust the current z-position to the compensation matrix in order to consider the different length of the extruder at higher temperatures
                adjustCompensationMatrix( (short)nZ );

                if( Printer::debugInfo() )
                {
                    // output the converted heat bed compensation matrix
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "adjusted heat bed z matrix: " ) );
                    outputCompensationMatrix();
                }

                g_nHeatBedScanStatus = 145;

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "144 -> 145" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 145:
            {
                // avoid to crash the extruder against the heat bed during the following homing
                g_nZScanZPosition += moveZ( int(Printer::axisStepsPerMM[Z_AXIS] *5) );

                // move back to the home position
                Printer::homeAxis( true, true, true);
                g_nZScanZPosition =
                nZ                = 0;

                g_nHeatBedScanStatus = 150;

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "145 -> 150" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 150:
            {
                // turn off the engines
                Printer::disableXStepper();
                Printer::disableYStepper();
                Printer::disableZStepper();

                // disable all heaters
                Extruder::setHeatedBedTemperature( 0, false );
                Extruder::setTemperatureForExtruder( 0, 0, false );
#if NUM_EXTRUDER == 2
                Extruder::setTemperatureForExtruder( 0, 1, false );
#endif // NUM_EXTRUDER == 2

                g_nHeatBedScanStatus = 160;

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "150 -> 160" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
            case 160:
            {
                // save the determined values to the EEPROM
                if( saveCompensationMatrix( (unsigned int)(EEPROM_SECTOR_SIZE * g_nActiveHeatBed) ) )
                {
                    //Ähm.... diesen Fall gibts garnicht laut saveCompensationMatrix()
                }
                else
                {
                    if( Printer::debugInfo() )
                    {
                        Com::printF( Com::tscanHeatBed );
                        Com::printFLN( PSTR( "the heat bed z matrix has been saved" ) );
                    }
                }

                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "the scan has been completed" ) );
                }
                UI_STATUS_UPD( UI_TEXT_HEAT_BED_SCAN_DONE );
                BEEP_STOP_HEAT_BED_SCAN

                g_nHeatBedScanStatus = 0;

#if DEBUG_HEAT_BED_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanHeatBed );
                    Com::printFLN( PSTR( "160 -> 0" ) );
                }
#endif // DEBUG_HEAT_BED_SCAN == 2
                break;
            }
        }
    }

    return;

} // scanHeatBed

/**************************************************************************************************************************************/

void startZOScan( bool automatrixleveling )
{
    if(g_nHeatBedScanStatus 
#if FEATURE_WORK_PART_Z_COMPENSATION
        || g_nWorkPartScanStatus
#endif // FEATURE_WORK_PART_Z_COMPENSATION
        ) return;
    if( g_ZOSScanStatus )
    {
        // abort the heat bed scan
        if( Printer::debugInfo() )
        {
            Com::printFLN( PSTR( "ZOS(): the scan has been cancelled" ) );
        }
        abortSearchHeatBedZOffset(false);
    }
    else
    {
        if( PrintLine::linesCount )
        {
            // there is some printing in progress at the moment - do not start the heat bed scan in this case
            if( Printer::debugErrors() )
            {
                Com::printFLN( PSTR( "ZOS(): exit - printing in progress" ) );
            }
            showError( (void*)ui_text_heat_bed_scan, (void*)ui_text_operation_denied );
        }
        else
        {
            Com::printFLN( PSTR( "ZOS(): started" ) );
            BEEP_START_HEAT_BED_SCAN
            g_ZOSScanStatus = 1;
            // when the heat bed is scanned, the z-compensation must be disabled
            if( Printer::doHeatBedZCompensation )
            {
                if( Printer::debugInfo() )
                {
                    Com::printFLN( PSTR( "ZOS(): the z compensation has been disabled" ) );
                }
                resetZCompensation();
            }
            // start the heat bed scan
            g_abortZScan = 0;
            g_retryZScan = 0;
            if(automatrixleveling) g_ZOS_Auto_Matrix_Leveling_State = 1; //aktiviert besonderer modus, bei dem der ZOffsetScan mehrfach in schleife scant und ein schiefes Bett geraderückt, aber die Welligkeit des ursprünglichen HBS behält.
        }
    }

    return;

} // startZOScan

void searchZOScan( void )
{
    if(g_ZOSScanStatus == 0) return;
    if(g_nHeatBedScanStatus 
#if FEATURE_WORK_PART_Z_COMPENSATION
        || g_nWorkPartScanStatus
#endif // FEATURE_WORK_PART_Z_COMPENSATION
        ){
      g_ZOSScanStatus = 0;
      g_ZOS_Auto_Matrix_Leveling_State = 0;
      return;
    }

    switch( g_ZOSScanStatus )
        {
            case 1:
            {
                Com::printFLN( PSTR( "ZOS(): init" ) ); 
                // when the heat bed Z offset is searched, the z-compensation must be disabled
                g_ZOSScanStatus = 2;
                g_min_nZScanZPosition = HEAT_BED_SCAN_Z_START_STEPS; //nur nutzen wenn kleiner.
                g_scanRetries = 0; // never retry   TODO allow retries?
                g_abortZScan = 0;  // will be set in case of error inside moveZUpFast/Slow
                break;
            }
            case 2:
            {
#if DEBUG_HEAT_BED_SCAN == 2
                Com::printFLN( PSTR( "ZOS(): STEP 1 : Home" ) );
#endif // DEBUG_HEAT_BED_SCAN == 2

                //bissel übertrieben, sollte aber jede eventualität abfangen: Wir brauchen die maximale Matrix-Dimension auch schon hier (ganz grob) und wollen nicht so lange warten.
                unsigned short uDimensionX, uDimensionY;
                if(g_ZCompensationMatrix[0][0] == EEPROM_FORMAT){
                    uDimensionX = g_uZMatrixMax[X_AXIS] - 1;
                    uDimensionY = g_uZMatrixMax[Y_AXIS] - 1;
                }else{
                    uDimensionX = (unsigned char)readWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, EEPROM_SECTOR_SIZE * g_nActiveHeatBed + EEPROM_OFFSET_DIMENSION_X ) - 1;
                    uDimensionY = (unsigned char)readWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, EEPROM_SECTOR_SIZE * g_nActiveHeatBed + EEPROM_OFFSET_DIMENSION_Y ) - 1;
                }
                if(uDimensionX > COMPENSATION_MATRIX_MAX_X - 1) uDimensionX = (unsigned char)(COMPENSATION_MATRIX_MAX_X - 1);
                if(uDimensionY > COMPENSATION_MATRIX_MAX_Y - 1) uDimensionY = (unsigned char)(COMPENSATION_MATRIX_MAX_Y - 1);

                //nun zu den settings:
                if(g_ZOS_Auto_Matrix_Leveling_State <= 1){
                    previousMillisCmd = HAL::timeInMilliseconds();
                    Printer::enableZStepper();
                    Printer::unsetAllSteppersDisabled();
                }
                //HERE THE FUNCTION MIGHT JUMP IN TO REDO SCANS FOR AUTO_MATRIX_LEVELING
                switch(g_ZOS_Auto_Matrix_Leveling_State){
                    case 1:
                        {
                        //Normal start, but overwrite first scan coordinate:
                        g_ZOSTestPoint[X_AXIS] = (unsigned char)(uDimensionX/2);
                        g_ZOSTestPoint[Y_AXIS] = (unsigned char)(uDimensionY/2);
                        g_ZOSlearningRate = 1.0f;
                        g_ZOSlearningGradient = 0.0f;
                        g_min_nZScanZPosition = HEAT_BED_SCAN_Z_START_STEPS; //nur nutzen wenn kleiner.
                        Com::printF( Com::tAutoMatrixLeveling, g_ZOSTestPoint[X_AXIS] );
                        Com::printFLN( PSTR( " Y:" ), g_ZOSTestPoint[Y_AXIS] );
                        break;
                        }
                    case 2:
                        {
                        g_ZOSTestPoint[X_AXIS] = 1; //will be constrained
                        g_ZOSTestPoint[Y_AXIS] = 1; //will be constrained
                        g_ZOSlearningRate = 0.8f;
                        g_ZOSlearningGradient = 1.0f;
                        g_min_nZScanZPosition = HEAT_BED_SCAN_Z_START_STEPS; //nur nutzen wenn kleiner.
                        Com::printF( Com::tAutoMatrixLeveling, g_ZOSTestPoint[X_AXIS] );
                        Com::printFLN( PSTR( " Y:" ), g_ZOSTestPoint[Y_AXIS] );
                        break;
                        }
                    case 3:
                        {
                        g_ZOSTestPoint[X_AXIS] = uDimensionX; //will be constrained
                        g_ZOSTestPoint[Y_AXIS] = uDimensionY; //will be constrained
                        g_ZOSlearningRate = 0.8f;
                        g_ZOSlearningGradient = 1.0f;
                        g_min_nZScanZPosition = HEAT_BED_SCAN_Z_START_STEPS; //nur nutzen wenn kleiner.
                        Com::printF( Com::tAutoMatrixLeveling, g_ZOSTestPoint[X_AXIS] );
                        Com::printFLN( PSTR( " Y:" ), g_ZOSTestPoint[Y_AXIS] );
                        break;
                        }
                    case 4:
                        {
                        g_ZOSTestPoint[X_AXIS] = (unsigned char)(uDimensionX/2);
                        g_ZOSTestPoint[Y_AXIS] = (unsigned char)(uDimensionY/2-1);
                        g_ZOSlearningRate = 0.8f;
                        g_ZOSlearningGradient = 1.0f;
                        g_min_nZScanZPosition = HEAT_BED_SCAN_Z_START_STEPS; //nur nutzen wenn kleiner.
                        Com::printF( Com::tAutoMatrixLeveling, g_ZOSTestPoint[X_AXIS] );
                        Com::printFLN( PSTR( " Y:" ), g_ZOSTestPoint[Y_AXIS] );
                        break;
                        }
                    case 5:
                        {
                        g_ZOSTestPoint[X_AXIS] = 1; //will be constrained
                        g_ZOSTestPoint[Y_AXIS] = uDimensionY; //will be constrained
                        g_ZOSlearningRate = 0.8f;
                        g_ZOSlearningGradient = 1.0f;
                        g_min_nZScanZPosition = HEAT_BED_SCAN_Z_START_STEPS; //nur nutzen wenn kleiner.
                        Com::printF( Com::tAutoMatrixLeveling, g_ZOSTestPoint[X_AXIS] );
                        Com::printFLN( PSTR( " Y:" ), g_ZOSTestPoint[Y_AXIS] );
                        break;
                        }
                    case 6:
                        {
                        g_ZOSTestPoint[X_AXIS] = uDimensionX; //will be constrained
                        g_ZOSTestPoint[Y_AXIS] = 1; //will be constrained
                        g_ZOSlearningRate = 0.8f;
                        g_ZOSlearningGradient = 1.0f;
                        g_min_nZScanZPosition = HEAT_BED_SCAN_Z_START_STEPS; //nur nutzen wenn kleiner.
                        Com::printF( Com::tAutoMatrixLeveling, g_ZOSTestPoint[X_AXIS] );
                        Com::printFLN( PSTR( " Y:" ), g_ZOSTestPoint[Y_AXIS] );
                        break;
                        }
                    case 7:
                        {
                        g_ZOSTestPoint[X_AXIS] = (unsigned char)(uDimensionX/2);
                        g_ZOSTestPoint[Y_AXIS] = (unsigned char)(uDimensionY/2+1);
                        g_ZOSlearningRate = 0.8f;
                        g_ZOSlearningGradient = 1.0f;
                        g_min_nZScanZPosition = HEAT_BED_SCAN_Z_START_STEPS; //nur nutzen wenn kleiner.
                        Com::printF( Com::tAutoMatrixLeveling, g_ZOSTestPoint[X_AXIS] );
                        Com::printFLN( PSTR( " Y:" ), g_ZOSTestPoint[Y_AXIS] );
                        g_ZOS_Auto_Matrix_Leveling_State = 0; //Auto-Matrix-Leveling ends after last scan.
                        break;
                        }
                    default:
                        {

                        } // No AUTO_MATRIX_LEVELING
                        //Normal start: End after this scan.
                }

                // start at the home position
                Printer::homeAxis( true, true, true );
                Commands::waitUntilEndOfAllMoves();
                if(g_nZScanZPosition){
                    Com::printF( PSTR( "ZOS(): Z-Error = " ), g_nZScanZPosition );
                    g_nZScanZPosition = 0;
                }
                g_ZOSScanStatus = 3;  
                break;
            }
            case 3:
            {
#if DEBUG_HEAT_BED_SCAN == 2
                Com::printF( PSTR( "ZOS(): STEP 2 : Spacing Z" ),HEAT_BED_SCAN_Z_START_STEPS );
                Com::printFLN( PSTR( " [Steps]" ) );
#endif // DEBUG_HEAT_BED_SCAN == 2

                g_nZScanZPosition += moveZ( HEAT_BED_SCAN_Z_START_STEPS ); //Wenn man hier mit moveRelativeDistanceInSteps über die queue Z verfährt, zeigt das display 10mm statt 5mm an. Weil currentZPositionSteps addiert und man die hier braucht. Also während dem Scan immer nur die moveZ verwenden.

                // move a bit away from the heat bed in order to achieve better measurements in case of hardware configurations where the extruder is very close to the heat bed after the z-homing     
                UI_STATUS_UPD( UI_TEXT_ZCALIB );
                g_uStartOfIdle = 0; //zeige nicht gleich wieder Printer Ready an.
                GCode::keepAlive( Processing );
                g_ZOSScanStatus = 4;   
                break;
            }
            case 4:
            {
#if DEBUG_HEAT_BED_SCAN == 2
                Com::printFLN( PSTR( "ZOS(): STEP 3 : Load Matrix" ) );
#endif // DEBUG_HEAT_BED_SCAN == 2
                // load the unaltered compensation matrix from the EEPROM
                if(g_ZCompensationMatrix[0][0] != EEPROM_FORMAT || g_ZOSlearningRate == 1.0){
                    Com::printFLN( PSTR( "ZOS(): Loading zMatrix from EEPROM" ) );
                    loadCompensationMatrix( (unsigned int)(EEPROM_SECTOR_SIZE * g_nActiveHeatBed) );
                }else{
                    Com::printFLN( PSTR( "ZOS(): Reusing existing zMatrix" ) );
                }

                // safety check on the current matrix
                if(g_ZCompensationMatrix[0][0] != EEPROM_FORMAT) {
                  Com::printFLN( PSTR( "ZOS(): ERROR::prev. matrix invalid!" ) );
#if DEBUG_HEAT_BED_SCAN == 2
                  Com::printFLN( PSTR( "ZOS(): TIPP::Neuen HBS machen! Do a fresh HBS!" ) );
#endif // DEBUG_HEAT_BED_SCAN == 2
                  abortSearchHeatBedZOffset(false);
                  break;
                }
                g_ZOSScanStatus = 5;
                break;
            }
            case 5:
            {
                //Constrain X-Y for Scanposition to valid locations at noninterpolated matrix-scanpoints.
                if( g_ZOSTestPoint[X_AXIS] < 1 + ((HEAT_BED_SCAN_X_START_MM == 0) ? 0 : 1) ) g_ZOSTestPoint[X_AXIS] = 1 + ((HEAT_BED_SCAN_X_START_MM == 0) ? 0 : 1);
                if( g_ZOSTestPoint[X_AXIS] > g_uZMatrixMax[X_AXIS] - 1 ) g_ZOSTestPoint[X_AXIS] = g_uZMatrixMax[X_AXIS] - 1; //2..n-1
                if( g_ZOSTestPoint[Y_AXIS] < 1 + ((HEAT_BED_SCAN_Y_START_MM == 0) ? 0 : 1) ) g_ZOSTestPoint[Y_AXIS] = 1 + ((HEAT_BED_SCAN_Y_START_MM == 0) ? 0 : 1);
                if( g_ZOSTestPoint[Y_AXIS] > g_uZMatrixMax[Y_AXIS] - 1 ) g_ZOSTestPoint[Y_AXIS] = g_uZMatrixMax[Y_AXIS] - 1; //2..n-1
                // move to the first scan position of the heat bed scan matrix
                long xScanPosition = (long)((float)g_ZCompensationMatrix[g_ZOSTestPoint[X_AXIS]][0] * Printer::axisStepsPerMM[X_AXIS]); // + g_nScanXStartSteps; <-- NEIN! Man muss nur die jeweils erste und letzte Matrix-Zeile meiden, ausser HEAT_BED_SCAN_X_START_MM ist 0 oder HEAT_BED_SCAN_Y_START_MM ist 0
                long yScanPosition = (long)((float)g_ZCompensationMatrix[0][g_ZOSTestPoint[Y_AXIS]] * Printer::axisStepsPerMM[Y_AXIS]); // + g_nScanYStartSteps; <-- NEIN!
#if DEBUG_HEAT_BED_SCAN == 2
                Com::printF( PSTR( "ZOS(): STEP 4 : Scan Position X+Y" ) );
                Com::printF( PSTR( "= (" ), xScanPosition );
                Com::printF( PSTR( ", " ), yScanPosition );
                Com::printFLN( PSTR( ") [(x,y) Steps]" ) );
#endif // DEBUG_HEAT_BED_SCAN == 2
                PrintLine::moveRelativeDistanceInSteps( xScanPosition, yScanPosition, 0, 0, RMath::min(MAX_FEEDRATE_X,MAX_FEEDRATE_Y), true, true );
                GCode::keepAlive( Processing );
                g_ZOSScanStatus = 6;    
                break;
            }
            case 6:
            {
                g_scanRetries = 20; //für 9, 10, 20
                GCode::keepAlive( Processing );
                g_ZOSScanStatus = 7;    
                break;
            }

            case 7: //ab hier bei Fehler:
            {
                HAL::delayMilliseconds( HEAT_BED_SCAN_DELAY );
                GCode::keepAlive( Processing );
                g_ZOSScanStatus = 9;    
                break;
            }
            case 9:
            {
#if DEBUG_HEAT_BED_SCAN == 2
                Com::printFLN( PSTR( "ZOS(): STEP 5 : Idle Pressure" ) );
#endif // DEBUG_HEAT_BED_SCAN == 2
                if( readIdlePressure( &g_nCurrentIdlePressure ) ) {
                  //Problem mit Digits, die wackeln:
                  if(g_scanRetries-- > 0){
                    g_ZOSScanStatus = 7;
                    break;
                  }else{
                    Com::printFLN( PSTR( "ZOS(): ERROR::the idle pressure could not be determined" ) );
                    abortSearchHeatBedZOffset(false);
                  }
                  break;
                } //egal was readIdlePressure zurückgibt, g_abortZScan könnte 1 sein und muss genullt werden.
                g_abortZScan = 0;  // will be set in case of error inside moveZUpFast/Slow -> != 0 AFTER RETURN would temper with normal HBS-Scan function @ABORT

                g_nMinPressureContact = g_nCurrentIdlePressure - SEARCH_HEAT_BED_OFFSET_CONTACT_PRESSURE_DELTA;
                g_nMaxPressureContact = g_nCurrentIdlePressure + SEARCH_HEAT_BED_OFFSET_CONTACT_PRESSURE_DELTA;
                g_nMinPressureRetry   = g_nCurrentIdlePressure - SEARCH_HEAT_BED_OFFSET_RETRY_PRESSURE_DELTA;
                g_nMaxPressureRetry   = g_nCurrentIdlePressure + SEARCH_HEAT_BED_OFFSET_RETRY_PRESSURE_DELTA;
                g_nMinPressureIdle    = g_nCurrentIdlePressure - SEARCH_HEAT_BED_OFFSET_IDLE_PRESSURE_DELTA;
                g_nMaxPressureIdle    = g_nCurrentIdlePressure + SEARCH_HEAT_BED_OFFSET_IDLE_PRESSURE_DELTA;

#if DEBUG_HEAT_BED_SCAN == 2
                    Com::printFLN( PSTR( "Idle Values:" ) );
                    Com::printFLN( PSTR( " g_nMinPressureContact = " ), g_nMinPressureContact );
                    Com::printFLN( PSTR( " g_nMaxPressureContact = " ), g_nMaxPressureContact );
                    Com::printFLN( PSTR( " g_nMinPressureRetry = " ), g_nMinPressureRetry );
                    Com::printFLN( PSTR( " g_nMaxPressureRetry = " ), g_nMaxPressureRetry );
                    Com::printFLN( PSTR( " g_nMinPressureIdle = " ), g_nMinPressureIdle );
                    Com::printFLN( PSTR( " g_nMaxPressureIdle = " ), g_nMaxPressureIdle );
#endif // DEBUG_HEAT_BED_SCAN == 2
                GCode::keepAlive( Processing );
                g_ZOSScanStatus = 10;   
                break;
            }
            case 10:
            {
#if DEBUG_HEAT_BED_SCAN == 2
                Com::printFLN( PSTR( "ZOS(): STEP 6 : Approaching HeatBed" ) );
#endif // DEBUG_HEAT_BED_SCAN == 2          
                // move to the surface
                moveZUpFast(false); // without runStandardTasks() inside to prevent an endless loop
                HAL::delayMilliseconds( g_nScanSlowStepDelay );

                //Wenn Filament langsam nachgibt, wandert evtl. die Kraft langsam. Hier prüfen, ob idle digits gültig.
                short   nTempPressure;
                if( readAveragePressure( &nTempPressure ) ){
                    if(g_scanRetries > 0) g_abortZScan = 0; //funktion soll wenn retrys übrig sind nie abbrechen, das g_abortZScan kommt aus readAveragePressure() -> hat einfluss auf HBS-Abort!!
                    g_retryZScan = 1;
                } 

                g_nZScanZPosition += moveZ( -g_nScanHeatBedUpFastSteps*4 );
                HAL::delayMilliseconds( g_nScanSlowStepDelay );

                short   nTempPressureUp;
                if( readAveragePressure( &nTempPressureUp ) ){
                    if(g_scanRetries > 0) g_abortZScan = 0; //funktion soll wenn retrys übrig sind nie abbrechen, das g_abortZScan kommt aus readAveragePressure() -> hat einfluss auf HBS-Abort!!
                    g_retryZScan = 1;
                } 

                if(abs(nTempPressureUp - nTempPressure) < SEARCH_HEAT_BED_OFFSET_CONTACT_PRESSURE_DELTA){
                    if(g_scanRetries > 0) g_abortZScan = 0; //funktion soll wenn retrys übrig sind nie abbrechen, das g_abortZScan kommt aus readAveragePressure() -> hat einfluss auf HBS-Abort!!
                    g_retryZScan = 1;
                }

                if(g_scanRetries > 0 && g_retryZScan){
                    g_retryZScan = 0;
                    g_scanRetries--;
                    Com::printFLN( PSTR( "Bettsuchproblem 10 -> 7 :" ), g_scanRetries );
                    GCode::keepAlive( Processing );
                    g_ZOSScanStatus = 7;   
                    break;
                }
               
                // check for error
                if(g_abortZScan) {
                  g_abortZScan = 0;  // will be set in case of error inside moveZUpFast/Slow -> != 0 AFTER RETURN would temper with normal HBS-Scan function @ABORT
                  Com::printFLN( PSTR( "ZOS(): ERROR::cannot find surface in fast scan" ) );
                  abortSearchHeatBedZOffset(false);
                  break;
                }

                GCode::keepAlive( Processing );
                g_ZOSScanStatus = 20;   
                break;
            }
            case 20:
            {   
#if DEBUG_HEAT_BED_SCAN == 2
                Com::printFLN( PSTR( "ZOS(): STEP 7 : Testing Surface " ));
#endif // DEBUG_HEAT_BED_SCAN
                bool prebreak = false;
                // we have roughly found the surface, now we perform the precise slow scan SEARCH_HEAT_BED_OFFSET_SCAN_ITERATIONS times  
                for(int i=0; i<SEARCH_HEAT_BED_OFFSET_SCAN_ITERATIONS; ++i) {
#if DEBUG_HEAT_BED_SCAN == 2
                      Com::printF( PSTR( " " ), (i+1) );
                      Com::printFLN( PSTR( "x" ) );
#endif // DEBUG_HEAT_BED_SCAN

                      // move two of the fast steps from moveZUpFast() down again
                      g_nZScanZPosition += moveZ( -g_nScanHeatBedUpFastSteps*2 );
                      HAL::delayMilliseconds( g_nScanSlowStepDelay );         

                      // move slowly to the surface
                      short nTempPressure;
                      moveZUpSlow( &nTempPressure, false ); // without runStandardTasks() inside to prevent an endless loop

                      if(g_scanRetries > 0 && g_retryZScan){
                        g_retryZScan = 0;
                        g_scanRetries--;
                        Com::printFLN( PSTR( "Suchproblem 20 -> 7:" ), g_scanRetries );
                        GCode::keepAlive( Processing );
                        g_ZOSScanStatus = 7;   
                        prebreak = true; break;
                      }
                      // check for error
                      if(g_abortZScan) {
                        g_abortZScan = 0;  // will be set in case of error inside moveZUpFast/Slow -> != 0 AFTER RETURN would temper with normal HBS-Scan function @ABORT
                        Com::printFLN( PSTR( "ZOS(): ERROR::cannot find surface in slow scan" ) );
                        abortSearchHeatBedZOffset(false);
                        prebreak = true; break;
                      }
                      
                      // keep the minimum as the final result
                      if(g_nZScanZPosition < g_min_nZScanZPosition) g_min_nZScanZPosition = g_nZScanZPosition;

#if DEBUG_HEAT_BED_SCAN == 2
                      Com::printF( PSTR( "ZOS(): Z = " ), g_nZScanZPosition );
                      Com::printFLN( PSTR( " Minimum-Z = " ), g_min_nZScanZPosition );
#endif // DEBUG_HEAT_BED_SCAN
                      GCode::keepAlive( Processing );
                }
                if(prebreak) break;
                g_ZOSScanStatus = 50;   
                break;
                
            }
            case 50:
            {    
                // compute number of steps we need to shift the entire matrix by
                long nZ = g_min_nZScanZPosition - g_ZCompensationMatrix[g_ZOSTestPoint[X_AXIS]][g_ZOSTestPoint[Y_AXIS]];
                            
#if DEBUG_HEAT_BED_SCAN == 2
                Com::printFLN( PSTR( "ZOS(): STEP 8 : " ) );
                Com::printFLN( PSTR( "ZOS(): Matrix-Wert Z = " ), g_ZCompensationMatrix[g_ZOSTestPoint[X_AXIS]][g_ZOSTestPoint[Y_AXIS]] );
#endif // DEBUG_HEAT_BED_SCAN
                Com::printF( PSTR( "ZOS(): Minimum Z = " ), g_min_nZScanZPosition );
                Com::printFLN( PSTR( " dZ = " ), nZ );
                            
                // update the matrix: shift by nZ and check for integer overflow
                bool overflow = false;
                bool overH = false;
                bool overnull = false;
                
                //Nibbels: scaling nZ according to learning Rate for additional corrective scans
                nZ = (long)((float)nZ * g_ZOSlearningRate);
#if DEBUG_HEAT_BED_SCAN == 2
                Com::printFLN( PSTR( "ZOS(): nZ*g_ZOSlearningRate = " ), nZ );     
#endif // DEBUG_HEAT_BED_SCAN 
                
                //Nibbels: weight change because of distance. lerne bettwinkelausgleich.
                float x_bed_len_quadrat = (float)((g_uZMatrixMax[X_AXIS]-2)*(g_uZMatrixMax[X_AXIS]-2)); //index zwischenabstamd x_n - x_0
                float y_bed_len_quadrat = (float)((g_uZMatrixMax[Y_AXIS]-2)*(g_uZMatrixMax[Y_AXIS]-2));
                float x_dist = 0;
                float y_dist = 0;
                float xy_weight = 0;
                long weighted_nZ = 0;
                long newValue = 0;
#if DEBUG_HEAT_BED_SCAN == 2
                Com::printFLN( PSTR( "ZOS(): INFO: weighted_nZ = g_ZOSlearningGradient*xy_weight*nZ + (1.0-g_ZOSlearningGradient)*nZ" ) );
#endif // DEBUG_HEAT_BED_SCAN
                
                for(short x=1; x<=g_uZMatrixMax[X_AXIS]; x++) {
                  for(short y=1; y<=g_uZMatrixMax[Y_AXIS]; y++) {
                    x_dist = (g_ZOSTestPoint[X_AXIS]-x)*(g_ZOSTestPoint[X_AXIS]-x)/x_bed_len_quadrat; //normierter indexabstand
                    y_dist = (g_ZOSTestPoint[Y_AXIS]-y)*(g_ZOSTestPoint[Y_AXIS]-y)/y_bed_len_quadrat; //normierter indexabstand
                    x_dist = sqrt(x_dist); //Linearisieren. für Auto-Matrix-Leveling
                    y_dist = sqrt(y_dist); //Linearisieren. für Auto-Matrix-Leveling
                    //das ist nur ein kreisabstand, wenn die messpunkte quadratisch angeordnet sind, ist aber nicht so?
                      // evtl. todo: achse faktor skalieren, sodass kreis x/y=(10/13)
                    xy_weight = 1 - sqrt(x_dist*x_dist+y_dist*y_dist); //linear gewichtet
                    if(xy_weight < 0.0) xy_weight = 0;
                    if(xy_weight > 1.0) xy_weight = 1.0; //kann aber nicht wirklich vorkommen.
                    weighted_nZ = (long)(g_ZOSlearningGradient*xy_weight*(float)nZ + (1.0-g_ZOSlearningGradient)*(float)nZ);
                    newValue = (long)g_ZCompensationMatrix[x][y] + weighted_nZ;
                    if(newValue > 32767 || newValue < -32768) overflow = true;
                    if(newValue > 0 ) overnull = true; //darf nicht positiv werden. //darf nicht über limit sein.
                    if(newValue > (HEAT_BED_SCAN_Z_START_STEPS + g_nScanHeatBedUpFastSteps) ) overH = true; //darf nicht positiv werden. //darf nicht über limit sein.
                    g_ZCompensationMatrix[x][y] = (short)newValue;
                  }
                }
                // fail if overflow occurred
                if(overflow) {
                  // load the unaltered compensation matrix from the EEPROM since the current in-memory matrix is invalid
                  Com::printFLN( PSTR( "ZOS(): Matrix Overflow!" ) );
#if DEBUG_HEAT_BED_SCAN == 2
                  Com::printFLN( PSTR( "ZOS(): ERROR::The measured correction is too large to be stored in the matrix (integer overflow)!" ) );
                  Com::printFLN( PSTR( "ZOS(): ReLoading zMatrix from EEPROM to RAM" ) );
#endif // DEBUG_HEAT_BED_SCAN
                  abortSearchHeatBedZOffset(true);
                  break;
                }
                // fail if z>(Starthöhe - ein bisschen) occurred
                if(overH) {
                  // load the unaltered compensation matrix from the EEPROM since the current in-memory matrix is bigger than z=zero
                  Com::printFLN( PSTR( "ZOS(): ERROR::Z-Matrix höher Start-Z!" ) );
#if DEBUG_HEAT_BED_SCAN == 2
                  Com::printFLN( PSTR( "ZOS(): HELP::http://www.rf1000.de/viewtopic.php?f=74&t=1674&start=10#p17016" ) );
                  Com::printFLN( PSTR( "ZOS(): FIX::Clean Hotend-Nozzle" ) );
                  Com::printFLN( PSTR( "ZOS(): FIX::Fix Z-Schraube" ) );
                  Com::printFLN( PSTR( "ZOS(): ReLoading zMatrix from EEPROM to RAM" ) );
#endif // DEBUG_HEAT_BED_SCAN
                  abortSearchHeatBedZOffset(true);
                  break;
                }
                // fail if z>0 occurred
                if(overnull) {
                  // load the unaltered compensation matrix from the EEPROM since the current in-memory matrix is bigger than z=zero
                  Com::printFLN( PSTR( "ZOS(): WARNING::At least one measured correction is z>0!" ) );
                  Com::printFLN( PSTR( "ZOS(): FIX::Clean Hotend-Nozzle" ) );
                  Com::printFLN( PSTR( "ZOS(): FIX::Fix Z-Schraube" ) );
#if DEBUG_HEAT_BED_SCAN == 2
                  Com::printFLN( PSTR( "ZOS(): HELP::http://www.rf1000.de/viewtopic.php?f=74&t=1674&start=10#p17016" ) );
#endif // DEBUG_HEAT_BED_SCAN
                }
                // determine the minimal distance between extruder and heat bed
                determineCompensationOffsetZ();     
                g_ZMatrixChangedInRam = 1; //man kan die matrix mit diesem marker nun sichern.
                
                g_ZOSScanStatus = 51;   
                break;
            }
            case 51:
            {
                //HERE THE ZOS MIGHT REDO SCANS FOR AUTO_MATRIX_LEVELING
                switch(g_ZOS_Auto_Matrix_Leveling_State){
                    case 0:
                        {
                        g_ZOSScanStatus = 100; // No AUTO_MATRIX_LEVELING
                        }
                    break;
                    default: 
                        {
                        g_ZOSScanStatus = 2; // Goto next AUTO_MATRIX_LEVELING Setting
                        g_ZOS_Auto_Matrix_Leveling_State++;
                        g_nZScanZPosition += moveZ( Printer::axisStepsPerMM[Z_AXIS] );
                        long xScanPosition = (long)(g_ZCompensationMatrix[g_ZOSTestPoint[X_AXIS]][0] * Printer::axisStepsPerMM[X_AXIS]);
                        long yScanPosition = (long)(g_ZCompensationMatrix[0][g_ZOSTestPoint[Y_AXIS]] * Printer::axisStepsPerMM[Y_AXIS]); // + g_nScanYStartSteps; <-- NEIN!
                        PrintLine::moveRelativeDistanceInSteps( -xScanPosition, -yScanPosition, 0, 0, MAX_FEEDRATE_Y, true, true );
                        g_nZScanZPosition += moveZ( -g_nZScanZPosition );    // g_nZScanZPosition counts z-steps. we need to move the heatbed down to be at z=0 again
                        outputCompensationMatrix( 1 );
                        }
                }
                break;
            }
            case 100:
            {   

#if DEBUG_HEAT_BED_SCAN == 2
                Com::printFLN( PSTR( "ZOS(): finished" ) );
#endif // DEBUG_HEAT_BED_SCAN

                g_ZOSScanStatus = 0;    
                
#if DEBUG_HEAT_BED_SCAN == 2
                Com::printFLN( PSTR( "ZOS(): finished" ) );
#endif // DEBUG_HEAT_BED_SCAN
                g_uStartOfIdle = 0;
                if( calculateZScrewCorrection() ){
                   UI_STATUS_UPD( UI_TEXT_HEAT_BED_SCAN_OFFSET_MIN );
                   showMyPage( (void*)ui_text_heat_bed_zoffset_search_status, (void*)ui_text_heat_bed_zoffset_fix_z1, (void*)ui_text_heat_bed_zoffset_fix_z2, (void*)ui_text_statusmsg );
                   g_nAutoReturnMessage = true;
                   g_nAutoReturnTime=HAL::timeInMilliseconds()+20000;
                }else{
                   UI_STATUS_UPD( UI_TEXT_HEAT_BED_SCAN_ABORTED );
                }

#if DEBUG_HEAT_BED_SCAN == 2
                Com::printFLN( PSTR( "ZOS(): STEP 9 : GOTO z=0" ) );
#endif // DEBUG_HEAT_BED_SCAN
                g_nZScanZPosition += moveZ( Printer::axisStepsPerMM[Z_AXIS] );
                long yScanPosition = (long)(g_ZCompensationMatrix[0][g_ZOSTestPoint[Y_AXIS]]* Printer::axisStepsPerMM[Y_AXIS]); // + g_nScanYStartSteps; <-- NEIN!
                PrintLine::moveRelativeDistanceInSteps( 0, -yScanPosition, 0, 0, MAX_FEEDRATE_Y, true, true );
                g_nZScanZPosition += moveZ( -g_nZScanZPosition );    // g_nZScanZPosition counts z-steps. we need to move the heatbed down to be at z=0 again
                //g_nZScanZPosition is 0 now.
                BEEP_SHORT
                break;
            }
        }
    return;
} // searchZOScan

void abortSearchHeatBedZOffset( bool reloadMatrix )
{
    if(reloadMatrix) loadCompensationMatrix( (unsigned int)(EEPROM_SECTOR_SIZE * g_nActiveHeatBed) ); 
    
    g_ZOSScanStatus = 0;
    g_retryZScan = 0;
    g_abortZScan = 0;
    g_nZScanZPosition = 0;

    // the search has been aborted
    UI_STATUS_UPD( UI_TEXT_HEAT_BED_SCAN_ABORTED );
    Com::printFLN( PSTR( "ZOS(): the scan has been aborted" ) );

    // move the heatbed 5mm down to avoid collisions, then home all axes
    PrintLine::moveRelativeDistanceInSteps( 0, 0, 5*Printer::axisStepsPerMM[Z_AXIS], 0, MAX_FEEDRATE_Z, true, true );
    Printer::homeAxis( true, true, true );

    // turn off all steppers and extruders
    Printer::setAllSteppersDisabled();
    Printer::disableXStepper();
    Printer::disableYStepper();
    Printer::disableZStepper();
    Extruder::disableAllExtruders();
    g_ZOS_Auto_Matrix_Leveling_State = 0;
} /* searchHeatBedZOffset */

float g_ZSchraubenSollDrehungenWarm_U = 0;
float g_ZSchraubenSollKorrekturWarm_mm = 0;
char g_ZSchraubeOk = 0;

bool calculateZScrewCorrection( void )
{
    bool returnwert = false;
    Com::printFLN( PSTR( " " ) );
    Com::printFLN( PSTR( "Z-Schrauben-Helper: " ) );
    if(g_ZCompensationMatrix[0][0] == EEPROM_FORMAT){
        /*IDEAL ist es, wenn mand den Drucker "kalt" und frisch aufgeheizt einstellt.*/
    
        //Gerade eben muss ein Z-Scan die Matrix korrigiert haben.
        //Der Extruder darf sich seither nicht verändert (T0 -> T1) und nicht abgekühlt haben
        //Dann wird pro °C des aktiven Extruders ~0.001mm Längung angenommen.
        //Dann wird pro °C des Heizbettes ~0.0015mm Längung angenommen.
        
        //Wegen der unbekannten Nachlängung werden 0.15mm Puffer auf Z=0 angestrebt.
        //Einfach so: wird ein Puffer von 0.05 angestrebt. Die ideale Matrix-Verschiebung wird auf 260°C Hotend / 120°C Bett, was ein Extremwert darstellen soll auf -0.05mm angepeilt.
        
        //Config
        float maxExtruderTemperature = (float)EXTRUDER_MAX_TEMP;
        float maxBedTemperature = 120.0f; //120°C ist ok... mit 180 zu rechnen wäre übertrieben.
        float BedThermalExplansionInMikrons = 1.5f;
        float ExtruderThermalExpansionInMikrons = 0.95f;
        float maxNachdehnungInMikrons = 150.0f;
        float RestAbstandInMikrons = 50.0f;
        
        //Ist-Abstand.
        float MatrixMaximumInMikrons = (float)g_offsetZCompensationSteps * Printer::invAxisStepsPerMM[Z_AXIS] * 1000.0f;
            
        float ExtruderTemperature = Extruder::current->tempControl.currentTemperatureC;
        if(ExtruderTemperature < 20.0f) ExtruderTemperature = 20.0f; //Wenn zu kalt oder undefiniert dann Standardbedingungen annehmen.
        
        float BedTemperature = Extruder::getHeatedBedTemperature();    
        if(BedTemperature == -1) maxBedTemperature = BedTemperature = 20.0f; //Wenn kein Heated-Bed dann Standardbedingungen annehmen.
            
        //Umrechnung des aktuellen Zustandes auf die heißesten Werte:
        float MinDistanceInMikronsKalt = MatrixMaximumInMikrons 
                + (maxExtruderTemperature - ExtruderTemperature) * ExtruderThermalExpansionInMikrons 
                + (maxBedTemperature - BedTemperature) * BedThermalExplansionInMikrons;
        float MinDistanceInMikronsWarm = MinDistanceInMikronsKalt + maxNachdehnungInMikrons;
        
        Com::printFLN( PSTR( "- Alle Werte in Mikrometern / Einheit [um] -" ) );
        Com::printFLN( PSTR( "Matrix-Minimum: " ) , MatrixMaximumInMikrons );
        Com::printFLN( PSTR( "Weitere Extruderausdehnung maximal: " ) , (maxExtruderTemperature - ExtruderTemperature) * ExtruderThermalExpansionInMikrons  );
        Com::printFLN( PSTR( "Weitere Heizbettausdehnung maximal: " ) , (maxBedTemperature - BedTemperature) * BedThermalExplansionInMikrons );
        Com::printFLN( PSTR( "Maximalwert-zMatrix bei Maximaltemperaturen (kalter Drucker): " ) , MinDistanceInMikronsKalt );
        Com::printFLN( PSTR( "Maximalwert-zMatrix bei Maximaltemperaturen (durchgewaermter Drucker): " ) , MinDistanceInMikronsWarm  ); 
        
        //z.B. -200 <-- um diesen Wert dürfte man korrigieren, wenn der Drucker zum Messzeitpunkt voll durchgewärmt wäre. Weiß er aber nicht!
        //Ein vorgewärmter Drucker justiert das Heizbett eher auf -0.2, ein kalter Drucker justiert es eher auf -0.05 bei Spitzentemperaturen. Beides ist ok.
        //float SollkorrekturKalt = (MinDistanceInMikronsKalt + RestAbstandInMikrons); 
        //z.B.  -50 <-- diesen Wert darf man in jedem Fall korrigieren.
        float SollkorrekturWarm = (MinDistanceInMikronsWarm + RestAbstandInMikrons); 
        
        /*
        Wenn ich den Test mit einem bereits warmen Drucker mache, plane ich unnötig eine Sicherheit ein, die ich nur einrechne, weil der Drucker aktuell kalt sein könnte. 
        Also warmer Drucker: Bett weiter hoch justieren, also Schraube weiter rein, also Drehsinn Minus.
        */        
        // |+0-|..Puffer..|.......Nachdehnung........|Kalt-Soll-Einstellung|
        // |+0-|..Puffer..|Warm-Soll-Einstellung|
        
        /* TIPP: -> Schraube bis maxNachdehnungInMikrons ~ 150um weiter runter(=Bett weiter hoch =Drehsinn Minus) empfehlen, wenn der Drucker aktuell "mehr druchgewärmt" wäre. */    
                
        Com::printFLN( PSTR( " " ) );
        Com::printFLN( PSTR( "#############" ) );
        Com::printF( PSTR( "Sollkorrektur: " ) , SollkorrekturWarm , 0 ); 
        Com::printF( PSTR( " [um] = " ), SollkorrekturWarm*0.001f,3  ); 
        Com::printFLN( PSTR( " [mm]" ) );

        //meldung:
        g_ZSchraubenSollKorrekturWarm_mm = SollkorrekturWarm*0.001f;

        float ZSchraubenDrehungenWarm = SollkorrekturWarm * 0.002f; //[Sollkorrektur in mm] geteilt durch [Gewinde: 0.5 mm/Umdrehung] -> Sollkorrektur / 1000 / 0.5
        Com::printF( PSTR( "Sollumdrehungen: " ) , ZSchraubenDrehungenWarm , 1 ); 
        Com::printF( PSTR( " [U] = " ) , ZSchraubenDrehungenWarm*360 , 0 ); 
        Com::printF( PSTR( " [Grad]" )  );

        //meldung:
        g_ZSchraubenSollDrehungenWarm_U = ZSchraubenDrehungenWarm;
        
        if(ZSchraubenDrehungenWarm > 0) Com::printFLN( PSTR( " (+ heisst rausdrehen/linksrum/gegen die Uhr)" ) ); //Bett wird nach unten justiert
        else                             Com::printFLN( PSTR( " (- heisst reindrehen/rechtsrum/im Uhrzeigersinn)" ) ); //Bett wird nach oben justiert
        
        Com::printFLN( PSTR( "Je kaelter der Gesamtdrucker aktuell ist (nach langer Pause frisch angeschaltet), desto besser der Korrekturwert." ) );
        g_ZSchraubeOk = -1; //neg -> Matrix negativ -> ok. ausser, wenn:      
#if MOTHERBOARD == DEVICE_TYPE_RF2000
        if( -0.5f <= ZSchraubenDrehungenWarm && SollkorrekturWarm < 40.0f /* [um] */){ // < 0.25mm = 0.5Umdrehungen ist mit dem RF2000 nicht machbar.
            Com::printFLN( PSTR( " (Die Z-Schraube ist ok!)" ) ); //das ist die Änderung in M3-Regelgewinde-Z-Schrauben-Umdrehungen
            //meldung:
            g_ZSchraubeOk = -1; //neg
        }else if(SollkorrekturWarm >= 40.0f /* [um] */){ //dann bin ich rechnerisch um Z = 0 (50um mit 10um toleranz, die ich fordere.)
            //eine korrektur von mehr als +40um heißt, ich bin gerade vermutlich im Z>0
            Com::printFLN( PSTR( " (Die Z-Schraube weiter raus! Das Bett scheint zu hoch zu liegen.)" ) ); //das ist die Änderung in M3-Regelgewinde-Z-Schrauben-Umdrehungen
            //meldung:
            g_ZSchraubeOk = 1; //pos
        } 
        Com::printFLN( PSTR( " (RF2000: Minimal eine halbe Schraubendrehung (dZ=0.25mm-Schritte) einstellbar.)" ) ); //das ist die Änderung in M3-Regelgewinde-Z-Schrauben-Umdrehungen
#else //if MOTHERBOARD == DEVICE_TYPE_RF1000        
        if(SollkorrekturWarm >= 40.0f /* [um] */){ //dann bin ich rechnerisch um Z = 0 (50um mit 10um toleranz, die ich fordere.)
            //eine korrektur von mehr als +40um heißt, ich bin gerade vermutlich im Z>0
            Com::printFLN( PSTR( " (Die Z-Schraube weiter raus! Das Bett scheint zu hoch zu liegen.)" ) ); //das ist die Änderung in M3-Regelgewinde-Z-Schrauben-Umdrehungen
            //meldung:
            g_ZSchraubeOk = 1; //pos
        } 
#endif    
        Com::printFLN( PSTR( "#############" ) );
        returnwert = true;
    }else{
        Com::printFLN( Com::tError, g_ZCompensationMatrix[0][0] );
        Com::printFLN( Com::tError, EEPROM_FORMAT ); 
        returnwert = false;
    }
    return returnwert;
} // calculateZScrewCorrection

/**************************************************************************************************************************************/



/**************************************************************************************************************************************/

void fixKeramikLochInMatrix( void )
{   
    //Com::printFLN( PSTR( "fixKeramikLochInMatrix(): STEP 1 Init" ) );
    
    if( g_ZCompensationMatrix[0][0] != EEPROM_FORMAT )
    {
        // we load the z compensation matrix before its first usage because this can take some time
        prepareZCompensation();
    }
    if( g_ZCompensationMatrix[0][0] == EEPROM_FORMAT )
    {
        // search for deepest hole in bed-z-matrix and fix it according to surrounding values
        
        long peak_hole = 0;
        long peak_x = 0;
        long peak_y = 0;
        
        long deepness = 0;
        long heights = 0;
        char div = 0;

        for(short x=1; x<=g_uZMatrixMax[X_AXIS]; x++) { //in der matrix ist alles von 1 an (?) -> also vermutlich anzahl = indexmax.
          for(short y=1; y<=g_uZMatrixMax[Y_AXIS]; y++) {

            heights = 0;
            div = 0;

            //circle around matrixposition
            for(short xx=x-1; xx<=x+1; xx++) {
              for(short yy=y-1; yy<=y+1; yy++) { //iterate all points
                if(xx != x || yy != y){ //nicht den punkt in der mitte
                    if(xx <= g_uZMatrixMax[X_AXIS] && xx >= 1 && yy <= g_uZMatrixMax[Y_AXIS] && yy >= 1){ //nicht punkte ausserhalb der matrix
                        heights += (long)g_ZCompensationMatrix[xx][yy];
                        div += 1;
                    }
                }
              }
            }
            deepness = (long)((float)heights / div) - g_ZCompensationMatrix[x][y]; //nur täler, negative werte.
            if(deepness > peak_hole && div > 3){ //nicht an ecken, sonst immer das tiefste loch suchen.
                peak_hole = deepness;
                peak_x = x;
                peak_y = y;
            }
        
          }
        }

        //Com::printFLN( PSTR( "fixKeramikLochInMatrix(): STEP 3 Extremwert" ) );
        //Com::printF( PSTR( "peak_x = " ), peak_x );
        //Com::printF( PSTR( "; peak_y = " ), peak_y );
        //Com::printF( PSTR( "; peak_hole = " ), peak_hole );
        //Com::printFLN( PSTR( "g_ZCompensationMatrix[peak_x,peak_y] =" ), g_ZCompensationMatrix[peak_x][peak_y] );

        if(peak_hole > 100 && peak_x > 0 && peak_y > 0){
            //loch groß genug
            g_ZCompensationMatrix[peak_x][peak_y] += peak_hole;
            Com::printF( PSTR( "Fixed: [peak_x,peak_y]=" ), g_ZCompensationMatrix[peak_x][peak_y] );
            g_ZMatrixChangedInRam = 1;
        }else{
            Com::printF( PSTR( "Fix not needed dh<100 dh=" ), peak_hole );            
        }
        
    }else{
      Com::printFLN( Com::tError );
    }
    
    determineCompensationOffsetZ(); //zur sicherheit und zukünftiger kompatibilität (Popel rausrechnen, würde den minimalen Z-Abstand verändern.)
    
    //Com::printFLN( PSTR( "fixKeramikLochInMatrix(): finished" ) );
    return;
} // fixKeramikLochInMatrix

/**************************************************************************************************************************************/

/**************************************************************************************************************************************/

void setMatrixNull( void )
{
    //Damit kann die Matrix eingeebnet werden. Anschließend mit M3902 Zx.x verschoben, dann mit dem M3900/M3901 mit Distance-Schalter ein Auto-Bed-Leveling auf die Alte Art gemacht werden.
    //Man hätte dann eine total ebene Matrix. Die Z-Kompensation ist dann faktisch aus, kann aber weiterlaufen.
    
    if( g_ZCompensationMatrix[0][0] != EEPROM_FORMAT )
    {
        // we load the z compensation matrix before its first usage because this can take some time
        prepareZCompensation();
    }
    if( g_ZCompensationMatrix[0][0] == EEPROM_FORMAT )
    {
        for(short x=1; x<=g_uZMatrixMax[X_AXIS]; x++) { //in der matrix ist alles von 1 an (?) -> also vermutlich anzahl = indexmax.
          for(short y=1; y<=g_uZMatrixMax[Y_AXIS]; y++) {
            g_ZCompensationMatrix[x][y] = 0;
          }
        }
        g_ZMatrixChangedInRam = 1; //man kan die matrix mit diesem marker nun sichern.
        Com::printFLN( PSTR( "Matrix set to 0!" ) );
    }else{
      Com::printFLN( Com::tError );
    }
    determineCompensationOffsetZ(); //zur sicherheit und zukünftiger kompatibilität (Popel rausrechnen, würde den minimalen Z-Abstand verändern.)
    return;
} // setMatrixNull

/**************************************************************************************************************************************/


/**************************************************************************************************************************************/
#if FEATURE_VISCOSITY_TEST
void startViscosityTest( int maxdigits = 10000, float maxfeedrate = 5.0f, float incrementfeedrate = 0.05f, short StartTemp = 0, short EndTemp = 0, int refill_digit_limit = 800 )
{
    Com::printFLN( PSTR( "startViscosityTest(): started" ) );

    if(refill_digit_limit > (int)(g_nZEmergencyStopAllMax*0.8) ) refill_digit_limit = (int)(g_nZEmergencyStopAllMax*0.2);
    if(refill_digit_limit < 50) refill_digit_limit = 50;
    Com::printFLN( PSTR( "Refill NozzleDigitsDelta = " ) , refill_digit_limit );

    if(maxdigits > (int)(g_nZEmergencyStopAllMax*0.8) ) maxdigits = (int)(g_nZEmergencyStopAllMax*0.8);
    if(maxdigits < 1000) maxdigits = 1000;
    Com::printFLN( PSTR( "Test DigitsMax = " ) , maxdigits );   

    if(maxfeedrate > Extruder::current->maxStartFeedrate) maxfeedrate = Extruder::current->maxStartFeedrate;
    if(maxfeedrate < 0.05) maxfeedrate = 0.05;
    Com::printFLN( PSTR( "Test FeedrateMax = " ) , maxfeedrate , 1);

    if(StartTemp > UI_SET_MAX_EXTRUDER_TEMP) StartTemp = UI_SET_MAX_EXTRUDER_TEMP;
    if(StartTemp < UI_SET_MIN_EXTRUDER_TEMP && StartTemp != 0) StartTemp = UI_SET_MIN_EXTRUDER_TEMP;
    if(EndTemp > UI_SET_MAX_EXTRUDER_TEMP) EndTemp = UI_SET_MAX_EXTRUDER_TEMP;
    if(EndTemp < UI_SET_MIN_EXTRUDER_TEMP && EndTemp != 0) EndTemp = UI_SET_MIN_EXTRUDER_TEMP;
    if(EndTemp < StartTemp || StartTemp == 0) EndTemp = StartTemp; //even if starttemp has some number and endtemp is 0, then one cycle is driven.

    if(EndTemp > StartTemp){
        Com::printFLN( PSTR( "Multitest StartTemp = " ) , StartTemp );
        Com::printFLN( PSTR( "Multitest EndTemp = " ) , EndTemp );
    }else if(EndTemp == StartTemp){
        Com::printFLN( PSTR( "SingleTest Temperature = " ) , StartTemp );
    }else if(StartTemp == 0 || EndTemp == 0){
        Com::printFLN( PSTR( "SingleTest Temperature = No Adjustment!" ) );
    }

    if(incrementfeedrate > 0.4) incrementfeedrate = 0.4;
    if(incrementfeedrate < 0.02) incrementfeedrate = 0.02;
    Com::printFLN( PSTR( "FeedrateIncrement = " ) , incrementfeedrate , 2 );
    
    previousMillisCmd = HAL::timeInMilliseconds();

    if( !Printer::areAxisHomed() )
    {
        Printer::homeAxis(true,true,true);
    }

    //if [S,P] "go to temp" then do so...
    if(StartTemp > 0){
        Extruder::setTemperatureForExtruder((float)StartTemp,Extruder::current->id,true);
        bool allReached = false;
        while(!allReached)
        {
            allReached = true;
            Commands::printTemperatures();
            Commands::checkForPeriodicalActions();
            GCode::keepAlive( WaitHeater );

            for( uint8_t h=0; h<NUM_TEMPERATURE_LOOPS; h++ )
            {
                TemperatureController *act = tempController[h];
                if( act->targetTemperatureC > 30 && fabs( act->targetTemperatureC - act->currentTemperatureC ) > TEMP_TOLERANCE )
                {
                    allReached = false;
                }
            }
        }
    }

    if( Extruder::current->tempControl.currentTemperatureC < (float)UI_SET_MIN_EXTRUDER_TEMP){      
        Com::printF( PSTR( "ERROR::Temperature:OFF or lower " ), (float)UI_SET_MIN_EXTRUDER_TEMP );
        Com::printFLN( PSTR( ".0°C = " ), Extruder::current->tempControl.currentTemperatureC );
        return;
    }

    //drive up the Bed 100mm -> if to low then filament will pile up to fast on the z-plattform
    Printer::moveToReal( IGNORE_COORDINATE, IGNORE_COORDINATE, 100 , IGNORE_COORDINATE, Printer::homingFeedrate[Z_AXIS]);
    Printer::moveToReal( 0, 0, IGNORE_COORDINATE , IGNORE_COORDINATE, Printer::homingFeedrate[Z_AXIS]);
    Commands::waitUntilEndOfAllMoves();

    //wait and test idle pressure
    HAL::delayMilliseconds( HEAT_BED_SCAN_DELAY );
    int err = readIdlePressure( &g_nCurrentIdlePressure );
    if( err != 0 ) {
        HAL::delayMilliseconds( HEAT_BED_SCAN_DELAY );
        err = readIdlePressure( &g_nCurrentIdlePressure );
        if( err != 0 ) {
            HAL::delayMilliseconds( HEAT_BED_SCAN_DELAY );
            err = readIdlePressure( &g_nCurrentIdlePressure );
            if( err != 0 ) {
                Com::printFLN( PSTR( "error idle pressure" ) );
                return;
            }
        }
    }

    long extrudedigits = 0; 
    //REFILL EXTRUDER
    Com::printFLN( PSTR( "Replenish Hotend..." ), extrudedigits );
    for(float e=0.1; e<=maxfeedrate; e+=0.05) { //iterate to fill extruder with filament.
        PrintLine::moveRelativeDistanceInSteps( 0, 0, 0 , (long)( 1.0f * Printer::axisStepsPerMM[E_AXIS] )* e , e, true, true ); //extrude slow until reaction.
        extrudedigits = (int)readStrainGauge( ACTIVE_STRAIN_GAUGE );
        PrintLine::moveRelativeDistanceInSteps( 0, 0, 0 , (long)( 1.0f * Printer::axisStepsPerMM[E_AXIS] )* e , e, true, true ); //extrude slow until reaction.
        extrudedigits = (int)readStrainGauge( ACTIVE_STRAIN_GAUGE );
        extrudedigits *= 0.5;

        Com::printFLN( PSTR( "force = " ), extrudedigits );
        Commands::printTemperatures();
        
        //refill_digit_limit = n guter Wert fürs Füllen des Hotends nach nem Retract. Zu wenig = noch Luft in Nozzle, zu viel = materialverschwendung bei sehr viskosen materialien.
        if(extrudedigits < g_nCurrentIdlePressure - refill_digit_limit || extrudedigits > g_nCurrentIdlePressure + refill_digit_limit) {  
            break;
        }
    }

    if(sd.sdactive){
        char filename[] = "Visco000.csv"; //000 wird überschrieben mit zahl
        for(uint8_t z = 0; z < 255; z++){
            char *str = &filename[8];
            uint8_t n = z;            
            do
            {
                uint8_t m = n;
                n /= 10;
                *--str = '0'+(m - 10 * n);
            }while(n);
            if(!sd.fat.exists(filename)){
                break;
            }
        }
        sd.startWrite(filename);
    }

    Com::printFLN( PSTR( "CSV-Logfile:START" ) );

    Com::printFLN( PSTR( ";Testing Filament..." ) );
    Com::printFLN( PSTR( ";Temperature [°C];e [mm/s];digits [1]" ) );
    if(sd.savetosd) sd.file.writeln_P( PSTR( ";Temperature [°C];e [mm/s];digits [1]" ) );

    for(float T = (float)StartTemp; T <= EndTemp; ){
        //@Init the Temp is reached by preheat!
        for(float e=0.05; e<=maxfeedrate; e+=incrementfeedrate) { //iterate all points  
            //test extrusion speed and get average digits:
            PrintLine::moveRelativeDistanceInSteps( 0, 0, 0 , (long)( 1.0f * Printer::axisStepsPerMM[E_AXIS] )* e , e, true, true ); //extrude only. time should be constant!
            extrudedigits = (int)readStrainGauge( ACTIVE_STRAIN_GAUGE );
            PrintLine::moveRelativeDistanceInSteps( 0, 0, 0 , (long)( 1.0f * Printer::axisStepsPerMM[E_AXIS] )* e , e, true, true ); //extrude only. time should be constant!
            extrudedigits += (int)readStrainGauge( ACTIVE_STRAIN_GAUGE );
            PrintLine::moveRelativeDistanceInSteps( 0, 0, 0 , (long)( 1.0f * Printer::axisStepsPerMM[E_AXIS] )* e , e, true, true ); //extrude only. time should be constant!
            extrudedigits += (int)readStrainGauge( ACTIVE_STRAIN_GAUGE );
            PrintLine::moveRelativeDistanceInSteps( 0, 0, 0 , (long)( 1.0f * Printer::axisStepsPerMM[E_AXIS] )* e , e, true, true ); //extrude only. time should be constant!
            extrudedigits += (int)readStrainGauge( ACTIVE_STRAIN_GAUGE );
            extrudedigits *= 0.25;

            Com::printF( Com::tSemiColon, Extruder::current->tempControl.currentTemperatureC, 1 ,true ); //true = dezimalkomma, nicht punkt. Wegen Excel.
            Com::printF( Com::tSemiColon, e, 3 , true ); //true = dezimalkomma, nicht punkt. Wegen Excel.
            Com::printFLN( Com::tSemiColon, extrudedigits );

            if(sd.savetosd){
                sd.file.write((uint8_t)';');
                sd.file.writeFloat(Extruder::current->tempControl.currentTemperatureC, 2, true);
                sd.file.write((uint8_t)';');
                sd.file.writeFloat(e, 3, true);
                sd.file.write((uint8_t)';');
                sd.file.writeFloat(extrudedigits, 0, true);
                sd.file.write_P(Com::tNewline);
            }

            previousMillisCmd = HAL::timeInMilliseconds();

            if(extrudedigits < g_nCurrentIdlePressure - maxdigits || extrudedigits > g_nCurrentIdlePressure + maxdigits || extrudedigits < -maxdigits || extrudedigits > maxdigits) {
                PrintLine::moveRelativeDistanceInSteps( 0, 0, 0 , (long)( -0.5 * Printer::axisStepsPerMM[E_AXIS] ), 10, true, true ); //loose some force on dms
                break;
            }
        }

        //Now we did reach Max-Digits, go one Tempstep higher and retest.
        T += 5;
        if(T <= EndTemp){ //nur erhöhen wenn sinnvoll, nicht wenn abbruch.
            Extruder::setTemperatureForExtruder( T, Extruder::current->id, true );
            //Wait until all the Temperatures are reached and stable.
            bool allReached = false;
            while(!allReached)
            {
                allReached = true;
                Commands::printTemperatures();
                Commands::checkForPeriodicalActions();
                GCode::keepAlive( WaitHeater );

                for( uint8_t h=0; h<NUM_TEMPERATURE_LOOPS; h++ )
                {
                    TemperatureController *act = tempController[h];
                    if( act->targetTemperatureC > 30 && fabs( act->targetTemperatureC - act->currentTemperatureC ) > TEMP_TOLERANCE )
                    {
                        allReached = false;
                    }
                }
            }
        }
    }

    Com::printFLN( PSTR( "CSV-Logfile:ENDE" ) );
    Com::printFLN( PSTR( "Copy and save this Log to *.csv-File for Excel." ) );

    sd.finishWrite();

    if(StartTemp > 0) Extruder::setTemperatureForExtruder( 0, Extruder::current->id, true ); //wir schalten aus, aber auch wieder an.
    UI_STATUS_UPD( UI_TEXT_TEST_STRAIN_GAUGE_DONE ); //gives "Test Completed"
    return;
} // startViscosityTest()
#endif //FEATURE_VISCOSITY_TEST
/**************************************************************************************************************************************/



/**************************************************************************************************************************************/
/* Diese Funktion soll die Step_Size eines Z-Steps einstellbar machen. */
/* Noch ist das statisch über eine Tabelle, aber man könnte das später auch berechnen */

void configureMANUAL_STEPS_Z( int8_t increment )
{
    //könnte evtl. auch unsigned short sein, aber das wird evtl. in g_nManualSteps geschrieben...
    const unsigned long stepsize_table[NUM_ACCEPTABLE_STEP_SIZE_TABLE] PROGMEM = ACCEPTABLE_STEP_SIZE_TABLE;
        
    int loop = 0;
    //suche die aktuelle Einstellungsposition (oder aufgerundet) in der Tabelle:
    for(loop = 0; loop < NUM_ACCEPTABLE_STEP_SIZE_TABLE; loop++) if(stepsize_table[loop] >= g_nManualSteps[Z_AXIS]) break;
    
    //ändere die Position in die Wunschposition:
    if(increment >= 0) loop += 1;
    else loop -= 1;
    
    //begrenze ringmenge
    if(loop > NUM_ACCEPTABLE_STEP_SIZE_TABLE -1) loop = 0;
    else if(loop < 0) loop = NUM_ACCEPTABLE_STEP_SIZE_TABLE -1;
    
    //nutze neuen Wert:
    g_nManualSteps[Z_AXIS] = stepsize_table[loop];
#if FEATURE_AUTOMATIC_EEPROM_UPDATE
    unsigned long oldval = HAL::eprGetInt32(EPR_RF_MOD_Z_STEP_SIZE);
    if(oldval != g_nManualSteps[Z_AXIS]){
        HAL::eprSetInt32( EPR_RF_MOD_Z_STEP_SIZE, g_nManualSteps[Z_AXIS] );
        EEPROM::updateChecksum(); //deshalb die prüfung
    }
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
    return;
} // configureMANUAL_STEPS_Z()
/**************************************************************************************************************************************/


short testExtruderTemperature( void )
{
    if( Extruder::current->tempControl.targetTemperatureC > 40 )
    {
        // we have to wait until the target temperature is reached
        if( (Extruder::current->tempControl.currentTemperatureC + TEMP_TOLERANCE) < Extruder::current->tempControl.targetTemperatureC )
        {
            // wait until the extruder has reached its target temperature
            if( Printer::debugInfo() )
            {
                Com::printF( PSTR( "heating: " ), Extruder::current->tempControl.currentTemperatureC, 1 );
                Com::printF( PSTR( " C / " ), Extruder::current->tempControl.targetTemperatureC, 1 );
                Com::printFLN( PSTR( " C" ) );
            }

            UI_STATUS_UPD( UI_TEXT_HEATING_UP );
            return -1;
        }
        if( (Extruder::current->tempControl.currentTemperatureC - TEMP_TOLERANCE) > Extruder::current->tempControl.targetTemperatureC )
        {
            // wait until the extruder has reached its target temperature
            if( Printer::debugInfo() )
            {
                Com::printF( PSTR( "cooling: " ), Extruder::current->tempControl.currentTemperatureC, 1 );
                Com::printF( PSTR( " C / " ), Extruder::current->tempControl.targetTemperatureC, 1 );
                Com::printFLN( PSTR( " C" ) );
            }

            UI_STATUS_UPD( UI_TEXT_COOLING_DOWN );
            return -1;
        }
    }
    else
    {
        // we have to wait until the current temperatur is below something which would be too warm
        if( Extruder::current->tempControl.currentTemperatureC > 65 )
        {
            // wait until the extruder has reached its target temperature
            if( Printer::debugInfo() )
            {
                Com::printF( PSTR( "cooling: " ),Extruder::current->tempControl.currentTemperatureC, 1 );
                Com::printFLN( PSTR( " C" ) );
            }

            UI_STATUS_UPD( UI_TEXT_COOLING_DOWN );
            return -1;
        }
    }

    // at this point we have reached the proper temperature
    return 0;

} // testExtruderTemperature


short testHeatBedTemperature( void )
{
#if HAVE_HEATED_BED
    if( heatedBedController.targetTemperatureC > 40 )
    {
        // we have to wait until the target temperature is reached
        if( (Extruder::getHeatedBedTemperature() + TEMP_TOLERANCE) < heatedBedController.targetTemperatureC )
        {
            // wait until the heat bed has reached its target temperature
            if( Printer::debugInfo() )
            {
                Com::printF( PSTR( "testHeatBedTemperature(): heating: " ), Extruder::getHeatedBedTemperature(), 1 );
                Com::printF( PSTR( " C / " ), heatedBedController.targetTemperatureC, 1 );
                Com::printFLN( PSTR( " C" ) );
            }

            UI_STATUS_UPD( UI_TEXT_HEATING_UP );
            return -1;
        }
        if( (Extruder::getHeatedBedTemperature() - TEMP_TOLERANCE) > heatedBedController.targetTemperatureC )
        {
            // wait until the heat bed has reached its target temperature
            if( Printer::debugInfo() )
            {
                Com::printF( PSTR( "testHeatBedTemperature(): cooling: " ), Extruder::getHeatedBedTemperature(), 1 );
                Com::printF( PSTR( " C / " ), heatedBedController.targetTemperatureC, 1 );
                Com::printFLN( PSTR( " C" ) );
            }

            UI_STATUS_UPD( UI_TEXT_HEATING_UP );
            return -1;
        }
    }
    else
    {
        // we have to wait until the current temperatur is below something which would be too warm
        if( Extruder::getHeatedBedTemperature() > 50 )
        {
            // wait until the heat bed has reached its target temperature
            if( Printer::debugInfo() )
            {
                Com::printF( PSTR( "testHeatBedTemperature(): cooling: " ), Extruder::getHeatedBedTemperature(), 1 );
                Com::printFLN( PSTR( " C" ) );
            }

            UI_STATUS_UPD( UI_TEXT_COOLING_DOWN );
            return -1;
        }
    }
#endif // HAVE_HEATED_BED

    // at this point we have reached the proper temperature
    return 0;

} // testHeatBedTemperature


void doHeatBedZCompensation( void )
{
    long            nCurrentPositionSteps[3];
    unsigned char   nXLeftIndex;
    unsigned char   nXRightIndex = 0;
    unsigned char   nYFrontIndex;
    unsigned char   nYBackIndex = 0;
    long            nXLeftSteps;
    long            nXRightSteps = 0;
    long            nYFrontSteps;
    long            nYBackSteps = 0;
    long            nTemp;
    long            nDeltaX;
    long            nDeltaY;
    long            nDeltaZ;
    long            nStepSizeX;
    long            nStepSizeY;
    long            nNeededZCompensation;
    long            nTempXFront;
    long            nTempXBack;
    //long          nTempZ;
    long            i;


    if( !Printer::doHeatBedZCompensation || (g_pauseStatus != PAUSE_STATUS_NONE && g_pauseStatus != PAUSE_STATUS_GOTO_PAUSE2 && g_pauseStatus != PAUSE_STATUS_TASKGOTO_PAUSE_2) ) // -> weil evtl. bewegung in xy auch solange pausestatus da ist.
    {
        // there is nothing to do at the moment
        return;
    }

    InterruptProtectedBlock noInts; //HAL::forbidInterrupts();
    nCurrentPositionSteps[X_AXIS] = Printer::queuePositionCurrentSteps[X_AXIS];
    nCurrentPositionSteps[Y_AXIS] = Printer::queuePositionCurrentSteps[Y_AXIS];
    nCurrentPositionSteps[Z_AXIS] = Printer::queuePositionCurrentSteps[Z_AXIS];

#if FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING
    nCurrentPositionSteps[X_AXIS] += Printer::directPositionCurrentSteps[X_AXIS];
    nCurrentPositionSteps[Y_AXIS] += Printer::directPositionCurrentSteps[Y_AXIS];
    nCurrentPositionSteps[Z_AXIS] += Printer::directPositionCurrentSteps[Z_AXIS];
#endif // FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING
    noInts.unprotect(); //HAL::allowInterrupts();

#if DEBUG_HEAT_BED_Z_COMPENSATION
    g_nLastZCompensationPositionSteps[X_AXIS] = nCurrentPositionSteps[X_AXIS];
    g_nLastZCompensationPositionSteps[Y_AXIS] = nCurrentPositionSteps[Y_AXIS];
    g_nLastZCompensationPositionSteps[Z_AXIS] = nCurrentPositionSteps[Z_AXIS];
#endif // DEBUG_HEAT_BED_Z_COMPENSATION
    
    // Der Z-Kompensation wird das extruderspezifische Z-Offset des jeweiligen Extruders verschwiegen, sodass dieses die Höhen / Limits nicht beeinflusst. Die X- und Y-Offsets werden behalten, denn das korrigiert Düsen- zu Welligkeitsposition nach Extruderwechsel. Das extruderspezifische Z-Offset Extruder::current->zOffset wird beim Toolchange in nCurrentPositionSteps[Z_AXIS] eingerechnet und verfahren.
    // Extruder::current->zOffset ist negativ, wenn das hotend weiter heruntergedrückt werden kann als 0. -> Bettfahrt nach unten, um auszuweichen.
    if( nCurrentPositionSteps[Z_AXIS] + Extruder::current->zOffset > 0 )
    {
        // check whether we have to perform a compensation in z-direction
        if( nCurrentPositionSteps[Z_AXIS] + Extruder::current->zOffset < g_maxZCompensationSteps )
        {
            // find the rectangle which covers the current position of the extruder
            nXLeftIndex = 1;
            nXLeftSteps = (long)((float)g_ZCompensationMatrix[1][0] * Printer::axisStepsPerMM[X_AXIS]);

            for( i=1; i<=g_uZMatrixMax[X_AXIS]; i++ )
            {
                nTemp = g_ZCompensationMatrix[i][0];
                nTemp = (long)((float)nTemp * Printer::axisStepsPerMM[X_AXIS]);
                if( nCurrentPositionSteps[X_AXIS] <= nTemp )
                {
                    nXRightIndex = i;
                    nXRightSteps = nTemp;
                    break;
                }
                nXLeftIndex = i;
                nXLeftSteps = nTemp;
            }
                    
            nYFrontIndex = 1;
            nYFrontSteps = (long)((float)g_ZCompensationMatrix[0][1] * Printer::axisStepsPerMM[Y_AXIS]);

            for( i=1; i<=g_uZMatrixMax[Y_AXIS]; i++ )
            {
                nTemp = g_ZCompensationMatrix[0][i];
                nTemp = (long)((float)nTemp * Printer::axisStepsPerMM[Y_AXIS]);
                if( nCurrentPositionSteps[Y_AXIS] <= nTemp )
                {
                    nYBackIndex = i;
                    nYBackSteps = nTemp;
                    break;
                }
                nYFrontIndex = i;
                nYFrontSteps = nTemp;
            }

            nDeltaX    = nCurrentPositionSteps[X_AXIS] - nXLeftSteps;
            nDeltaY    = nCurrentPositionSteps[Y_AXIS] - nYFrontSteps;
            nStepSizeX = nXRightSteps - nXLeftSteps;
            nStepSizeY = nYBackSteps - nYFrontSteps;

            // we do a linear interpolation in order to find our exact place within the current rectangle
            nTempXFront = g_ZCompensationMatrix[nXLeftIndex][nYFrontIndex] +
                          (g_ZCompensationMatrix[nXRightIndex][nYFrontIndex] - g_ZCompensationMatrix[nXLeftIndex][nYFrontIndex]) * nDeltaX / nStepSizeX;
            nTempXBack  = g_ZCompensationMatrix[nXLeftIndex][nYBackIndex] +
                          (g_ZCompensationMatrix[nXRightIndex][nYBackIndex] - g_ZCompensationMatrix[nXLeftIndex][nYBackIndex]) * nDeltaX / nStepSizeX;
            nNeededZCompensation = nTempXFront +
                                   (nTempXBack - nTempXFront) * nDeltaY / nStepSizeY;

#if DEBUG_HEAT_BED_Z_COMPENSATION
            g_nDelta[X_AXIS]    = nDeltaX;
            g_nDelta[Y_AXIS]    = nDeltaY;
            g_nStepSize[X_AXIS] = nStepSizeX;
            g_nStepSize[Y_AXIS] = nStepSizeY;
            g_nTempXFront       = nTempXFront;
            g_nTempXBack        = nTempXBack;
            g_nNeededZ          = nNeededZCompensation;
            g_uIndex[0]         = nXLeftIndex;
            g_uIndex[1]         = nXRightIndex;
            g_uIndex[2]         = nYFrontIndex;
            g_uIndex[3]         = nYBackIndex;
            g_nMatrix[0]        = g_ZCompensationMatrix[nXLeftIndex][nYFrontIndex];
            g_nMatrix[1]        = g_ZCompensationMatrix[nXRightIndex][nYFrontIndex];
            g_nMatrix[2]        = g_ZCompensationMatrix[nXLeftIndex][nYBackIndex];
            g_nMatrix[3]        = g_ZCompensationMatrix[nXRightIndex][nYBackIndex];
#endif // DEBUG_HEAT_BED_Z_COMPENSATION

            if( nCurrentPositionSteps[Z_AXIS] + Extruder::current->zOffset <= g_minZCompensationSteps )
            {
                // the printer is very close to the surface - we shall print a layer of exactly the desired thickness
                nNeededZCompensation += g_staticZSteps;
            }
            else
            {
                // the printer is already a bit away from the surface - do the actual compensation
                nDeltaZ = g_maxZCompensationSteps - (nCurrentPositionSteps[Z_AXIS] + Extruder::current->zOffset);
                nNeededZCompensation = g_offsetZCompensationSteps + 
                                       (nNeededZCompensation - g_offsetZCompensationSteps) * nDeltaZ / (g_maxZCompensationSteps - g_minZCompensationSteps);
                nNeededZCompensation += g_staticZSteps;
            }
        }
        else
        {   
            // after the first layers, only the static offset to the surface must be compensated
            nNeededZCompensation = g_offsetZCompensationSteps + g_staticZSteps;
        }
    }
    else
    {
        //RF1000 dev: we do not perform a compensation in case the z-position from the G-code is 0 (because this would drive the extruder against the heat bed)
        //nNeededZCompensation = g_staticZSteps;
        
        //Nibbels: Wenn ich meine Z-Matrix um 4mm ins Plus setze (Oder wohin auch immer ins Plus), dann G1 Z0 -> Fährt -4mm auf Z = 0, dann G1 Z0.2 -> Fährt +4,2mm in Compensationsposition.
        // Das Verhalten ist ziemlich bescheuert, der soll wen möglich immer, wenn Z-Compensation Aktiv ist auf mindestens den höchsten Punkt der Z-Matrix anheben, weil er sonst gegen das Bett crashen könnte.
        // after the first layers, only the static offset to the surface must be compensated
        //Dann evtl. lieber über dem Zenit bleiben + kleines Offset?? 29.05.2017
        nNeededZCompensation = g_offsetZCompensationSteps + g_staticZSteps + (long)(0.001 * Printer::axisStepsPerMM[Z_AXIS]);
    }

#if FEATURE_DIGIT_Z_COMPENSATION
    //Etwa 5500 digits verursachen 0.05 mm tiefere nozzle: ca. 0.00001 = 1/100.000 mm pro digit.
    //0.00001 ist vermutlich konservativ bis ok.
    //Je höher die Kraft nach unten, desto mehr muss das Bett ausweichen: Z nach oben/+.
    
    //VORSICHT: Die Messzellen könnten falsch verbaut sein, darum Digits immer positiv nutzen. Negative Digits würden sowieso in die falsche Richtung tunen. Ein kleiner Versatz der Nullposition wäre beim Druck egal.
    //long nNeededDigitZCompensationSteps = (long)(fabs(g_nDigitZCompensationDigits) * (float)Printer::axisStepsPerMM[Z_AXIS] * 0.00001f);
    if(g_nDigitZCompensationDigits_active){
        long nNeededDigitZCompensationSteps = abs((long)(g_nDigitZCompensationDigits * (float)Printer::axisStepsPerMM[Z_AXIS])); 
        nNeededDigitZCompensationSteps >>= 17; // geteilt durch 131072 statt errechnet ca. 110000 .. wäre 18% überkompensiert aber verdammt schnell gerechnet. evtl. ist überkompensation nicht so schlecht... höhere digits höhere schwankungen, das ist sowieso nur eine ganz grob vermessene kompensation, etwas mehr platz kann für die digits eine dämpfende wirkung haben.
        //sign-extension kann hier, da positiv kein problem sein.: unsigned(x) >> y
        //g_nDigitZCompensationDigits -> max. 32768, axisStepsPerMM[Z_AXIS] -> ca. 2560, also passts in long. -> ca. max 640 steps.
        nNeededDigitZCompensationSteps = constrain(nNeededDigitZCompensationSteps, 0, 600);

        nNeededZCompensation += nNeededDigitZCompensationSteps;
    }else{
        //wie bisher ohne additionszusatz
    }
#endif // FEATURE_DIGIT_Z_COMPENSATION


#if DEBUG_HEAT_BED_Z_COMPENSATION
    long    nZDelta = Printer::compensatedPositionTargetStepsZ - nNeededZCompensation;

    if( nZDelta < g_nZDeltaMin )        g_nZDeltaMin = nZDelta;
    if( nZDelta > g_nZDeltaMax )        g_nZDeltaMax = nZDelta;

    g_nZCompensationUpdateTime = micros();

    if( Printer::compensatedPositionTargetStepsZ != Printer::compensatedPositionCurrentStepsZ )
    {
        g_nTooFast ++;
    }

    if( Printer::compensatedPositionTargetStepsZ != nNeededZCompensation )
    {
        g_nLastZCompensationTargetStepsZ = nNeededZCompensation;
        g_nZCompensationUpdates ++;
        g_debugLog = 2;
    }
#endif // DEBUG_HEAT_BED_Z_COMPENSATION

    noInts.protect(true); //HAL::forbidInterrupts();
    Printer::compensatedPositionTargetStepsZ = nNeededZCompensation;
    noInts.unprotect(); //HAL::allowInterrupts();

    return;

} // doHeatBedZCompensation


long getHeatBedOffset( void )
{
    //Funktion rechnet das Z-Matrix-Korrigierte Offset aus, an der exakten Stelle an der wir stehen.
    long            nCurrentPositionSteps[2];
    long            nOffset;
    unsigned char   nXLeftIndex;
    unsigned char   nXRightIndex = 0;
    unsigned char   nYFrontIndex;
    unsigned char   nYBackIndex = 0;
    long            nXLeftSteps;
    long            nXRightSteps = 0;
    long            nYFrontSteps;
    long            nYBackSteps= 0;
    long            nTemp;
    long            nDeltaX;
    long            nDeltaY;
    long            nStepSizeX;
    long            nStepSizeY;
    long            nTempXFront;
    long            nTempXBack;
    long            i;


    if( !Printer::doHeatBedZCompensation && !( g_nHeatBedScanStatus || g_ZOSScanStatus ) ) //|| g_ZOSScanStatus brauche ich hier vermutlich nicht. Aber ich lasse es mal drin!
    {
        // we determine the offset to the scanned heat bed only in case the heat bed z compensation is active
        return 0; //vermerk Nibbels: Beim HBS -> adjustCompensationMatrix -> short  deltaZ  = nZ - nOffset; -> mit offset ist das verfälscht!! --> return 0
    }

    InterruptProtectedBlock noInts; //HAL::forbidInterrupts();
    nCurrentPositionSteps[X_AXIS] = Printer::queuePositionCurrentSteps[X_AXIS];
    nCurrentPositionSteps[Y_AXIS] = Printer::queuePositionCurrentSteps[Y_AXIS];

#if FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING
    nCurrentPositionSteps[X_AXIS] += Printer::directPositionCurrentSteps[X_AXIS];
    nCurrentPositionSteps[Y_AXIS] += Printer::directPositionCurrentSteps[Y_AXIS];
#endif // FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING
    noInts.unprotect(); //HAL::allowInterrupts();

    // find the rectangle which covers the current position of the extruder
    nXLeftIndex = 1;
    nXLeftSteps = (long)((float)g_ZCompensationMatrix[1][0] * Printer::axisStepsPerMM[X_AXIS]);

    for( i=1; i<=g_uZMatrixMax[X_AXIS]; i++ )
    {
        nTemp = g_ZCompensationMatrix[i][0];
        nTemp = (long)((float)nTemp * Printer::axisStepsPerMM[X_AXIS]);
        if( nCurrentPositionSteps[X_AXIS] <= nTemp )
        {
            nXRightIndex = i;
            nXRightSteps = nTemp;
            break;
        }
        nXLeftIndex = i;
        nXLeftSteps = nTemp;
    }
                    
    nYFrontIndex = 1;
    nYFrontSteps = (long)((float)g_ZCompensationMatrix[0][1] * Printer::axisStepsPerMM[Y_AXIS]);

    for( i=1; i<=g_uZMatrixMax[Y_AXIS]; i++ )
    {
        nTemp = g_ZCompensationMatrix[0][i];
        nTemp = (long)((float)nTemp * Printer::axisStepsPerMM[Y_AXIS]);
        if( nCurrentPositionSteps[Y_AXIS] <= nTemp )
        {
            nYBackIndex = i;
            nYBackSteps = nTemp;
            break;
        }
        nYFrontIndex = i;
        nYFrontSteps = nTemp;
    }

    nDeltaX    = nCurrentPositionSteps[X_AXIS] - nXLeftSteps;
    nDeltaY    = nCurrentPositionSteps[Y_AXIS] - nYFrontSteps;
    nStepSizeX = nXRightSteps - nXLeftSteps;
    nStepSizeY = nYBackSteps - nYFrontSteps;

    // we do a linear interpolation in order to find our exact place within the current rectangle
    nTempXFront = g_ZCompensationMatrix[nXLeftIndex][nYFrontIndex] +
                  (g_ZCompensationMatrix[nXRightIndex][nYFrontIndex] - g_ZCompensationMatrix[nXLeftIndex][nYFrontIndex]) * nDeltaX / nStepSizeX;
    nTempXBack  = g_ZCompensationMatrix[nXLeftIndex][nYBackIndex] +
                  (g_ZCompensationMatrix[nXRightIndex][nYBackIndex] - g_ZCompensationMatrix[nXLeftIndex][nYBackIndex]) * nDeltaX / nStepSizeX;
    nOffset     = nTempXFront +
                  (nTempXBack - nTempXFront) * nDeltaY / nStepSizeY;

#if FEATURE_DIGIT_Z_COMPENSATION //For Comments see doHeatBedZCompensation(){}
    if(g_nDigitZCompensationDigits_active){
        long nNeededDigitZCompensationSteps = abs((long)(g_nDigitZCompensationDigits * (float)Printer::axisStepsPerMM[Z_AXIS])); 
        nNeededDigitZCompensationSteps >>= 17;
        nOffset += constrain(nNeededDigitZCompensationSteps, 0, 600);
    }else{
        //wie bisher ohne addition
    }
#endif // FEATURE_DIGIT_Z_COMPENSATION

    return nOffset;

} // getHeatBedOffset
#endif // FEATURE_HEAT_BED_Z_COMPENSATION


#if FEATURE_WORK_PART_Z_COMPENSATION
void startWorkPartScan( char nMode )
{
    if( g_nWorkPartScanStatus )
    {
        // abort the work part scan
        if( Printer::debugInfo() )
        {
            Com::printFLN( PSTR( "startWorkPartScan(): the scan has been cancelled" ) );
        }
        g_abortZScan = 1;
    }
    else
    {
        if( PrintLine::linesCount )
        {
            // there is some printing in progress at the moment - do not start the heat bed scan in this case
            if( Printer::debugErrors() )
            {
                Com::printFLN( PSTR( "startWorkPartScan(): the scan can not be started while the milling is in progress" ) );
            }

            showError( (void*)ui_text_work_part_scan, (void*)ui_text_operation_denied );
        }
        else
        {
            // start the work part scan
            g_nWorkPartScanStatus = 1;
            g_nWorkPartScanMode   = nMode;
            BEEP_START_WORK_PART_SCAN

            // when the work part is scanned, the z-compensation must be disabled
            if( Printer::doWorkPartZCompensation )
            {
                if( Printer::debugInfo() )
                {
                    Com::printFLN( PSTR( "startWorkPartScan(): the z compensation has been disabled" ) );
                }
                resetZCompensation();
            }
        }
    }

    return;

} // startWorkPartScan


void scanWorkPart( void )
{
    if(g_ZOSScanStatus) return;

    static unsigned char    nIndexX;
    static unsigned char    nIndexY;
    static char             nIndexYDirection;
    static long             nX;
    static long             nY;
    static long             nZ;
    static long             nYDirection;
    static short            nContactPressure = 0;
    //char                  nLastWorkPartScanStatus = g_nWorkPartScanStatus;
    short                   nTempPressure;
    long                    nTempPosition;


    // directions:
    // +x = to the right
    // -x = to the left
    // +y = work part moves to the front
    // -y = work part moves to the back
    // +z = work part moves down
    // -z = work part moves up

    if( g_abortZScan )
    {
        // the scan has been aborted
        g_abortZScan = 0;

        // start at the home position
        if( g_nWorkPartScanMode )
        {
            // also the z-axis shall be homed
            Printer::homeAxis( true, true, true );
        }
        else
        {
            // the z-axis shall not be homed - in this case we must ensure that the tool does not crash against the limit stops at the front/left of the bed
            PrintLine::moveRelativeDistanceInSteps( 0, 0, WORK_PART_SCAN_Z_START_STEPS, 0, MAX_FEEDRATE_Z, true, true );
            Printer::homeAxis( true, true, false );
        }

        // turn off the engines
        Printer::disableXStepper();
        Printer::disableYStepper();
        Printer::disableZStepper();

        if( Printer::debugInfo() )
        {
            Com::printF( Com::tscanWorkPart );
            Com::printFLN( PSTR( "the scan has been aborted" ) );
        }

        UI_STATUS_UPD( UI_TEXT_WORK_PART_SCAN_ABORTED );
        BEEP_ABORT_WORK_PART_SCAN

        // restore the compensation values from the EEPROM
        if( loadCompensationMatrix( 0 ) ) //Nibbels: Ist das hier nicht vieleicht Falsch? Eher das: (EEPROM_SECTOR_SIZE *9) + (unsigned int)(EEPROM_SECTOR_SIZE * g_nActiveWorkPart)? --> Bei Adresse 0 wird in der Funktion ermittelt welche passt.
        {
            // there is no valid compensation matrix available
            initCompensationMatrix();
        }

        g_nWorkPartScanStatus = 0;
        g_nZScanZPosition     = 0;
        g_nLastZScanZPosition = 0;
        g_retryZScan          = 0;
        return;
    }

    // show that we are active
    previousMillisCmd = HAL::timeInMilliseconds();

    if( g_nWorkPartScanStatus )
    {
        UI_STATUS( UI_TEXT_WORK_PART_SCAN );

        if( g_retryZScan )
        {
            // we have to retry to scan the current position
            g_nWorkPartScanStatus = 45;
            g_retryZScan          = 0;
        }

        switch( g_nWorkPartScanStatus )
        {
            case 1:
            {
                g_scanStartTime    = HAL::timeInMilliseconds();
                g_abortZScan       = 0;
                nContactPressure   = 0;
                g_nTempDirectionZ  = 0;

                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanWorkPart );
                    Com::printFLN( PSTR( "the scan has been started" ) );
                }

                // clear all fields of the work part compensation matrix
                initCompensationMatrix();

                g_uZMatrixMax[X_AXIS] =
                g_uZMatrixMax[Y_AXIS] = 0;

                // output the currently used scan parameters
                outputScanParameters();

                g_nWorkPartScanStatus = 10;

#if DEBUG_WORK_PART_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanWorkPart );
                    Com::printFLN( PSTR( "1 -> 10" ) );
                }
#endif // DEBUG_WORK_PART_SCAN
                break;
            }
            case 10:
            {
                // start at the home position
                if( g_nWorkPartScanMode )
                {
                    // also the z-axis shall be homed
                    Printer::homeAxis( true, true, true );
                }
                else
                {
                    // the z-axis shall not be homed - in this case we must ensure that the tool does not crash against the limit stops at the front/left of the bed
                    PrintLine::moveRelativeDistanceInSteps( 0, 0, WORK_PART_SCAN_Z_START_STEPS, 0, MAX_FEEDRATE_Z, true, true );
                    Printer::homeAxis( true, true, false );
                }

                g_nWorkPartScanStatus = 25;
                g_lastScanTime        = HAL::timeInMilliseconds();

#if DEBUG_WORK_PART_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanWorkPart );
                    Com::printFLN( PSTR( "10 -> 25" ) );
                }
#endif // DEBUG_WORK_PART_SCAN
                break;
            }
            case 25:
            {
                // move to the first position
                previousMillisCmd = HAL::timeInMilliseconds();
                Printer::enableZStepper();
                Printer::unsetAllSteppersDisabled();

                PrintLine::moveRelativeDistanceInSteps( g_nScanXStartSteps, 0, 0, 0, MAX_FEEDRATE_X, true, true );
                PrintLine::moveRelativeDistanceInSteps( 0, g_nScanYStartSteps, 0, 0, MAX_FEEDRATE_Y, true, true );

                g_nWorkPartScanStatus = 30;
                g_lastScanTime        = HAL::timeInMilliseconds();

#if DEBUG_WORK_PART_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanWorkPart );
                    Com::printFLN( PSTR( "25 -> 30" ) );
                }
#endif // DEBUG_WORK_PART_SCAN
                break;
            }
            case 30:
            {
                if( (HAL::timeInMilliseconds() - g_lastScanTime) < WORK_PART_SCAN_DELAY )
                {
                    // do not check too early
                    break;
                }

                if( readIdlePressure( &g_nFirstIdlePressure ) )
                {
                    // we were unable to determine the idle pressure
                    break;
                }

                g_nMinPressureContact = g_nFirstIdlePressure - g_nScanContactPressureDelta;
                g_nMaxPressureContact = g_nFirstIdlePressure + g_nScanContactPressureDelta;
                g_nMinPressureRetry   = g_nFirstIdlePressure - g_nScanRetryPressureDelta;
                g_nMaxPressureRetry   = g_nFirstIdlePressure + g_nScanRetryPressureDelta;
                g_nMinPressureIdle    = g_nFirstIdlePressure - g_nScanIdlePressureDelta;
                g_nMaxPressureIdle    = g_nFirstIdlePressure + g_nScanIdlePressureDelta;

                // prepare the direction of the z-axis (we have to move the milling bed up)
                prepareBedUp();
                g_nTempDirectionZ = -1;

                nX               = g_nScanXStartSteps;
                nY               = g_nScanYStartSteps;
                nZ               = 0;
                nYDirection      = g_nScanYStepSizeSteps;   // we start to move the milling bed from the back to the front
                nIndexYDirection = 1;
                nIndexX          = 2;
                nIndexY          = 2;

                // store also the version of this work part compensation matrix
#if DEBUG_REMEMBER_SCAN_PRESSURE
                g_ScanPressure[0][0] = EEPROM_FORMAT;
#endif // DEBUG_REMEMBER_SCAN_PRESSURE

                g_ZCompensationMatrix[0][0] = EEPROM_FORMAT;

                g_nWorkPartScanStatus = 32;

#if DEBUG_WORK_PART_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanWorkPart );
                    Com::printFLN( PSTR( "30 -> 32" ) );
                }
#endif // DEBUG_WORK_PART_SCAN
                break;
            }
            case 32:
            {
                short   nCurrentPressure;


                // move the work part up until we detect the contact pressure
                g_lastScanTime = HAL::timeInMilliseconds();
                while( 1 )
                {
                    nCurrentPressure = readStrainGauge( ACTIVE_STRAIN_GAUGE );

                    if( nCurrentPressure > g_nMaxPressureContact || nCurrentPressure < g_nMinPressureContact )
                    {
                        // we have reached the target pressure
                        g_nWorkPartScanStatus = 33;

#if DEBUG_WORK_PART_SCAN == 2
                        if( Printer::debugInfo() )
                        {
                            Com::printF( Com::tscanWorkPart );
                            Com::printFLN( PSTR( "32 -> 33" ) );
                        }
#endif // DEBUG_WORK_PART_SCAN
                        return;
                    }

                    if( Printer::isZMinEndstopHit() )
                    {
                        // this should never happen
                        if( Printer::debugErrors() )
                        {
                            Com::printF( Com::tscanWorkPart );
                            Com::printFLN( PSTR( "the z-min endstop has been reached" ) );
                        }
                        g_abortZScan = 1;
                        return;
                    }

                    g_nZScanZPosition += moveZ( g_nScanHeatBedUpFastSteps );

                    if( (HAL::timeInMilliseconds() - g_lastScanTime) > SEARCH_Z_ORIGIN_BREAKOUT_DELAY )
                    {
                        // do not stay within this loop forever
                        return;
                    }

                    if( g_abortZScan )
                    {
                        break;
                    }
                }

                // we should never end up here
                break;
            }
            case 33:
            {
                short   nCurrentPressure;


                // move the work part down again until we do not detect any contact anymore
                g_lastScanTime = HAL::timeInMilliseconds();
                while( 1 )
                {
                    nCurrentPressure = readStrainGauge( ACTIVE_STRAIN_GAUGE );

                    if( nCurrentPressure > g_nMinPressureContact && nCurrentPressure < g_nMaxPressureContact )
                    {
                        // we have reached the target pressure / we have found the z-origin
                        if( Printer::debugErrors() )
                        {
                            Com::printF( Com::tscanWorkPart );
                            Com::printFLN( PSTR( "the z-origin has been determined" ) );
                        }

                        setZOrigin();

                        // move away from the surface
                        nZ = moveZDownFast();
                        g_nZScanZPosition = nZ;
                        g_nWorkPartScanStatus = 35;

                        // ensure that we do not remember any previous z-position at this moment
                        g_nLastZScanZPosition = 0;

#if DEBUG_WORK_PART_SCAN == 2
                        if( Printer::debugInfo() )
                        {
                            Com::printF( Com::tscanWorkPart );
                            Com::printFLN( PSTR( "33 -> 35 > " ), nZ );
                        }
#endif // DEBUG_WORK_PART_SCAN
                        return;
                    }

                    if( Printer::isZMaxEndstopHit() )
                    {
                        if( Printer::debugErrors() )
                        {
                            Com::printF( Com::tscanWorkPart );
                            Com::printFLN( PSTR( "the z-max endstop has been reached" ) );
                        }
                        g_abortZScan = 1;
                        return;
                    }

                    g_nZScanZPosition += moveZ( g_nScanHeatBedDownSlowSteps );

                    if( (HAL::timeInMilliseconds() - g_lastScanTime) > SEARCH_Z_ORIGIN_BREAKOUT_DELAY )
                    {
                        // do not stay within this loop forever
                        return;
                    }

                    if( g_abortZScan )
                    {
                        break;
                    }
                }

                // we should never end up here
                break;
            }
            case 35:
            {
                g_nWorkPartScanStatus = 40;

#if DEBUG_WORK_PART_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanWorkPart );
                    Com::printFLN( PSTR( "35 -> 40" ) );
                }
#endif // DEBUG_WORK_PART_SCAN
                break;
            }
            case 39:
            {
                nTempPosition = nX + g_nScanXStepSizeSteps;
                if( nTempPosition > g_nScanXMaxPositionSteps )
                {
                    // we end up here when the scan is complete
                    g_nWorkPartScanStatus = 60;

#if DEBUG_WORK_PART_SCAN == 2
                    if( Printer::debugInfo() )
                    {
                        Com::printF( Com::tscanWorkPart );
                        Com::printFLN( PSTR( "39 -> 60" ) );
                    }
#endif // DEBUG_WORK_PART_SCAN
                    break;
                }

                // move to the next x-position
                PrintLine::moveRelativeDistanceInSteps( g_nScanXStepSizeSteps, 0, 0, 0, MAX_FEEDRATE_X, true, true );
                nX += g_nScanXStepSizeSteps;
                nIndexX ++;

                if( nIndexX > COMPENSATION_MATRIX_MAX_X )
                {
                    if( Printer::debugErrors() )
                    {
                        Com::printF( Com::tscanWorkPart );
                        Com::printFLN( PSTR( "the x-dimension of the z matrix became too big: " ), nIndexX );
                    }
                    g_abortZScan = 1;
                    break;
                }

                if( nYDirection > 0 )
                {
                    // we were moving from the front to the back during this column, so we have to move from the back to the front during the next column
                    nYDirection      = -g_nScanYStepSizeSteps;  // we start to move the milling bed from the back to the front
                    nIndexYDirection = -1;
                }
                else
                {
                    // we were moving from the back to the front during this column, so we have to move from the front to the back during the next column
                    nYDirection      = g_nScanYStepSizeSteps;   // we start to move the milling bed from the back to the front
                    nIndexYDirection = 1;
                }

                g_nWorkPartScanStatus = 40;

#if DEBUG_WORK_PART_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanWorkPart );
                    Com::printFLN( PSTR( "39 -> 40" ) );
                }
#endif // DEBUG_WORK_PART_SCAN
                break;
            }
            case 40:
            {
                // safety checks
                if( nX <= g_nScanXMaxPositionSteps )
                {
                    // remember also the exact x-position of this row/column
#if DEBUG_REMEMBER_SCAN_PRESSURE
                    g_ScanPressure[nIndexX][0] = nX;
#endif // DEBUG_REMEMBER_SCAN_PRESSURE

                    g_ZCompensationMatrix[nIndexX][0] = (short)((float)nX / Printer::axisStepsPerMM[X_AXIS] + 0.5); // convert to mm

                    g_nWorkPartScanStatus = 49;
                    g_lastScanTime        = HAL::timeInMilliseconds();

#if DEBUG_WORK_PART_SCAN == 2
                    if( Printer::debugInfo() )
                    {
                        Com::printF( Com::tscanWorkPart );
                        Com::printFLN( PSTR( "40 -> 49 : X=" ), g_ZCompensationMatrix[nIndexX][0] );
                    }
#endif // DEBUG_WORK_PART_SCAN
                    break;
                }

                // we end up here when the scan is complete
                g_nWorkPartScanStatus = 60;

#if DEBUG_WORK_PART_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanWorkPart );
                    Com::printFLN( PSTR( "40 -> 60" ) );
                }
#endif // DEBUG_WORK_PART_SCAN
                break;
            }
            case 45:
            {
                // move away from the surface
                nZ += moveZ( g_nScanHeatBedDownFastSteps );
                g_nZScanZPosition = nZ;

                g_scanRetries         --;
                g_nWorkPartScanStatus = 46;
                g_lastScanTime        = HAL::timeInMilliseconds();

#if DEBUG_WORK_PART_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanWorkPart );
                    Com::printFLN( PSTR( "45 -> 46 > " ), nZ );
                }
#endif // DEBUG_WORK_PART_SCAN
                break;
            }
            case 46:
            {
                if( (HAL::timeInMilliseconds() - g_lastScanTime) < WORK_PART_SCAN_DELAY )
                {
                    // do not check too early
                    break;
                }

                // try to determine the idle pressure again
                if( readIdlePressure( &g_nFirstIdlePressure ) )
                {
                    // we were unable to determine the idle pressure
                    break;
                }

                g_nMinPressureContact = g_nFirstIdlePressure - g_nScanContactPressureDelta;
                g_nMaxPressureContact = g_nFirstIdlePressure + g_nScanContactPressureDelta;
                g_nMinPressureRetry   = g_nFirstIdlePressure - g_nScanRetryPressureDelta;
                g_nMaxPressureRetry   = g_nFirstIdlePressure + g_nScanRetryPressureDelta;
                g_nMinPressureIdle    = g_nFirstIdlePressure - g_nScanIdlePressureDelta;
                g_nMaxPressureIdle    = g_nFirstIdlePressure + g_nScanIdlePressureDelta;

                g_nWorkPartScanStatus = 50;

#if DEBUG_WORK_PART_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanWorkPart );
                    Com::printFLN( PSTR( "46 -> 50" ) );
                }
#endif // DEBUG_WORK_PART_SCAN
                break;
            }
            case 49:
            {
                g_scanRetries         = WORK_PART_SCAN_RETRIES;
                g_nWorkPartScanStatus = 50;

#if DEBUG_WORK_PART_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanWorkPart );
                    Com::printFLN( PSTR( "49 -> 50" ) );
                }
#endif // DEBUG_WORK_PART_SCAN
                break;
            }
            case 50:
            {
                if( (HAL::timeInMilliseconds() - g_lastScanTime) < g_nScanIdleDelay )
                {
                    // do not check too early
                    break;
                }

                // scan this point
                if( testIdlePressure() )
                {
                    // the current idle pressure is not plausible
                    g_abortZScan = 1;
                    break;
                }

                // we should consider that the idle presse can change slightly
                g_nMinPressureContact = g_nCurrentIdlePressure - g_nScanContactPressureDelta;
                g_nMaxPressureContact = g_nCurrentIdlePressure + g_nScanContactPressureDelta;
                g_nMinPressureRetry   = g_nCurrentIdlePressure - g_nScanRetryPressureDelta;
                g_nMaxPressureRetry   = g_nCurrentIdlePressure + g_nScanRetryPressureDelta;
                g_nMinPressureIdle    = g_nCurrentIdlePressure - g_nScanIdlePressureDelta;
                g_nMaxPressureIdle    = g_nCurrentIdlePressure + g_nScanIdlePressureDelta;

                g_nWorkPartScanStatus = 51;

#if DEBUG_WORK_PART_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanWorkPart );
                    Com::printFLN( PSTR( "50 -> 51" ) );
                }
#endif // DEBUG_WORK_PART_SCAN
                break;
            }
            case 51:
            {
                // move fast to the surface
                nZ += moveZUpFast();
                g_nZScanZPosition = nZ;

                g_nWorkPartScanStatus = 52;

#if DEBUG_WORK_PART_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanWorkPart );
                    Com::printFLN( PSTR( "51 -> 52 > " ), nZ );
                }
#endif // DEBUG_WORK_PART_SCAN
                break;
            }
            case 52:
            {
                // move a little bit away from the surface
                nZ += moveZDownSlow();
                g_nZScanZPosition = nZ;

                g_nWorkPartScanStatus = 53;

#if DEBUG_WORK_PART_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanWorkPart );
                    Com::printFLN( PSTR( "52 -> 53 > " ), nZ );
                }
#endif // DEBUG_WORK_PART_SCAN
                break;
            }
            case 53:
            {
                // move slowly to the surface
                nZ += moveZUpSlow( &nTempPressure );
                g_nZScanZPosition = nZ;

                nContactPressure      = nTempPressure;
                g_nWorkPartScanStatus = 54;

#if DEBUG_WORK_PART_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanWorkPart );
                    Com::printFLN( PSTR( "53 -> 54 > " ), nZ );
                }
#endif // DEBUG_WORK_PART_SCAN
                break;
            }
            case 54:
            {
#if DEBUG_WORK_PART_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( PSTR( "nX;" ), nX );
                    Com::printF( Com::tSemiColon, (float)nX / Printer::axisStepsPerMM[X_AXIS] );
                    Com::printF( PSTR( ";nY;" ), nY );
                    Com::printF( Com::tSemiColon, (float)nY / Printer::axisStepsPerMM[Y_AXIS] );
                    Com::printF( PSTR( ";nZ;" ), nZ );
                    Com::printF( Com::tSemiColon, (float)nZ / Printer::axisStepsPerMM[Z_AXIS] );
                    Com::printF( PSTR( ";Pressure;" ), nContactPressure );

                    Com::printF( PSTR( ";nIndexX;" ), (int)nIndexX );
                    Com::printF( PSTR( ";nIndexY;" ), (int)nIndexY );

/*                  // output the non compensated position values
                    Com::printF( PSTR( ";;" ), Printer::queuePositionCurrentSteps[X_AXIS] );
                    Com::printF( Com::tSemiColon, Printer::queuePositionCurrentSteps[Y_AXIS] );
                    Com::printF( Com::tSemiColon, Printer::queuePositionCurrentSteps[Z_AXIS] );
                    Com::printF( Com::tSemiColon, Printer::compensatedPositionCurrentStepsZ );
*/
                    Com::printFLN( PSTR( " " ) );
                }
#endif // DEBUG_WORK_PART_SCAN

                // remember the z-position and the exact y-position of this row/column
                g_ZCompensationMatrix[nIndexX][nIndexY] = (short)nZ;
                g_ZCompensationMatrix[0][nIndexY]       = (short)((float)nY / Printer::axisStepsPerMM[Y_AXIS] + 0.5);   // convert to mm

#if DEBUG_REMEMBER_SCAN_PRESSURE
                // remember the pressure and the exact y-position of this row/column
                g_ScanPressure[nIndexX][nIndexY] = nContactPressure;
                g_ScanPressure[0][nIndexY]       = nY;
#endif // DEBUG_REMEMBER_SCAN_PRESSURE

                if( nIndexX > g_uZMatrixMax[X_AXIS] )
                {
                    g_uZMatrixMax[X_AXIS] = nIndexX;
                }

                if( nIndexY > g_uZMatrixMax[Y_AXIS] )
                {
                    g_uZMatrixMax[Y_AXIS] = nIndexY;
                }
        
                g_nWorkPartScanStatus = 55;

#if DEBUG_WORK_PART_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanWorkPart );
                    Com::printFLN( PSTR( "54 -> 55 > " ), nZ );
                }
#endif // DEBUG_WORK_PART_SCAN
                break;
            }
            case 55:
            {
                // move away from the surface
                nZ += moveZDownFast();
                g_nZScanZPosition = nZ;

                if( nYDirection > 0 )
                {
                    nTempPosition = nY+nYDirection;

                    if( nTempPosition > g_nScanYMaxPositionSteps )
                    {
                        // we have reached the end of this column
                        g_nWorkPartScanStatus = 39;

#if DEBUG_WORK_PART_SCAN == 2
                        if( Printer::debugInfo() )
                        {
                            Com::printF( Com::tscanWorkPart );
                            Com::printFLN( PSTR( "55 -> 39" ) );
                        }
#endif // DEBUG_WORK_PART_SCAN
                        break;
                    }
                }
                else
                {
                    nTempPosition = nY+nYDirection;

                    if( nTempPosition < g_nScanYStartSteps )
                    {
                        // we have reached the end of this column
                        g_nWorkPartScanStatus = 39;

#if DEBUG_WORK_PART_SCAN == 2
                        if( Printer::debugInfo() )
                        {
                            Com::printF( Com::tscanWorkPart );
                            Com::printFLN( PSTR( "55 -> 39" ) );
                        }
#endif // DEBUG_WORK_PART_SCAN
                        break;
                    }
                }

                // move to the next y-position
                PrintLine::moveRelativeDistanceInSteps( 0, nYDirection, 0, 0, MAX_FEEDRATE_Y, true, true );
                nY      += nYDirection;
                nIndexY += nIndexYDirection;

                if( nIndexY > COMPENSATION_MATRIX_MAX_Y )
                {
                    if( Printer::debugErrors() )
                    {
                        Com::printF( Com::tscanWorkPart );
                        Com::printFLN( PSTR( "the y-dimension of the z matrix became too big: " ), nIndexY );
                    }
                    g_abortZScan = 1;
                    break;
                }

                g_nWorkPartScanStatus = 49;
                g_lastScanTime        = HAL::timeInMilliseconds();

#if DEBUG_WORK_PART_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanWorkPart );
                    Com::printFLN( PSTR( "55 -> 49" ) );
                }
#endif // DEBUG_WORK_PART_SCAN
                break;
            }
            case 60:
            {
                // move back to the home position
                if( g_nWorkPartScanMode )
                {
                    // also the z-axis shall be homed
                    Printer::homeAxis( true, true, true );
                }
                else
                {
                    // the z-axis shall not be homed - in this case we must ensure that the tool does not crash against the limit stops at the front/left of the bed
                    PrintLine::moveRelativeDistanceInSteps( 0, 0, WORK_PART_SCAN_Z_START_STEPS, 0, MAX_FEEDRATE_Z, true, true );
                    Printer::homeAxis( true, true, false );
                }

                // turn off the engines
                Printer::disableXStepper();
                Printer::disableYStepper();
                Printer::disableZStepper();

                g_nWorkPartScanStatus = 65;

#if DEBUG_WORK_PART_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanWorkPart );
                    Com::printFLN( PSTR( "60 -> 65" ) );
                }
#endif // DEBUG_WORK_PART_SCAN
                break;
            }
            case 65:
            {
                if( Printer::debugInfo() )
                {
                    // output the determined compensation
                    Com::printF( Com::tscanWorkPart );
                    Com::printFLN( PSTR( "raw work part z matrix: " ) );
                    outputCompensationMatrix();
                }

                g_nWorkPartScanStatus = 70;

#if DEBUG_WORK_PART_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanWorkPart );
                    Com::printFLN( PSTR( "65 -> 70" ) );
                }
#endif // DEBUG_WORK_PART_SCAN
                break;
            }
            case 70:
            {
                // output the determined pressure
                outputPressureMatrix();

                g_nWorkPartScanStatus = 75;

#if DEBUG_WORK_PART_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanWorkPart );
                    Com::printFLN( PSTR( "70 -> 75" ) );
                }
#endif // DEBUG_WORK_PART_SCAN
                break;
            }
            case 75:
            {
                if( Printer::debugInfo() )
                {
                    // output the pure scan time
                    Com::printF( Com::tscanWorkPart );
                    Com::printF( PSTR( "total scan time: " ), long((HAL::timeInMilliseconds() - g_scanStartTime) / 1000) );
                    Com::printFLN( PSTR( " [s]" ) );
                }

                // prepare the work part compensation matrix for fast usage during the actual milling
                prepareCompensationMatrix();

                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanWorkPart );
                    Com::printFLN( PSTR( "g_uZMatrixMax[Y_AXIS].1 = " ), (int)g_uZMatrixMax[Y_AXIS] );
                }

                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanWorkPart );
                    Com::printFLN( PSTR( "g_uZMatrixMax[Y_AXIS].2 = " ), (int)g_uZMatrixMax[Y_AXIS] );

                    // output the converted work part compensation matrix
                    Com::printF( Com::tscanWorkPart );
                    Com::printFLN( PSTR( "converted work part z matrix: " ) );
                    outputCompensationMatrix();
                }

                // save the determined values to the EEPROM
                if( saveCompensationMatrix( (EEPROM_SECTOR_SIZE *9) + (unsigned int)(EEPROM_SECTOR_SIZE * g_nActiveWorkPart) ) )
                {                    
                    //Ähm.... diesen Fall gibts garnicht laut saveCompensationMatrix()
                }
                else
                {
                    if( Printer::debugInfo() )
                    {
                        Com::printF( Com::tscanWorkPart );
                        Com::printFLN( PSTR( "the work part z matrix has been saved > " ), g_nActiveWorkPart );
                    }
                }

                g_nWorkPartScanStatus = 80;
                g_lastScanTime        = HAL::timeInMilliseconds();

#if DEBUG_WORK_PART_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanWorkPart );
                    Com::printFLN( PSTR( "75 -> 80" ) );
                }
#endif // DEBUG_WORK_PART_SCAN
                break;
            }
            case 80:
            {
                if( (HAL::timeInMilliseconds() - g_lastScanTime) < WORK_PART_SCAN_DELAY )
                {
                    // do not check too early
                    break;
                }

                // compare the idle pressure at the beginning and at the end
                readAveragePressure( &nTempPressure );

                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanWorkPart );
                    Com::printFLN( PSTR( "idle pressure at start: " ), g_nFirstIdlePressure );
                    Com::printF( Com::tscanWorkPart );
                    Com::printFLN( PSTR( "idle pressure at stop: " ), nTempPressure );
                }

                g_nWorkPartScanStatus = 100;

#if DEBUG_WORK_PART_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanWorkPart );
                    Com::printFLN( PSTR( "80 -> 100" ) );
                }
#endif // DEBUG_WORK_PART_SCAN
                break;
            }
            case 100:
            {
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanWorkPart );
                    Com::printFLN( PSTR( "the scan has been completed" ) );
                }
                UI_STATUS_UPD( UI_TEXT_WORK_PART_SCAN_DONE );
                BEEP_STOP_WORK_PART_SCAN

                g_nWorkPartScanStatus = 0;

#if DEBUG_WORK_PART_SCAN == 2
                if( Printer::debugInfo() )
                {
                    Com::printF( Com::tscanWorkPart );
                    Com::printFLN( PSTR( "100 -> 0" ) );
                }
#endif // DEBUG_WORK_PART_SCAN
                break;
            }
        }
    }
    (void)nContactPressure;
    return;

} // scanWorkPart


void doWorkPartZCompensation( void )
{
    long            nCurrentPositionSteps[3];
    unsigned char   nXLeftIndex;
    unsigned char   nXRightIndex = 0;
    unsigned char   nYFrontIndex;
    unsigned char   nYBackIndex = 0;
    long            nXLeftSteps;
    long            nXRightSteps = 0;
    long            nYFrontSteps;
    long            nYBackSteps = 0;
    long            nTemp;
    long            nDeltaX;
    long            nDeltaY;
    //long          nDeltaZ;
    long            nStepSizeX;
    long            nStepSizeY;
    long            nNeededZCompensation;
    long            nTempXFront;
    long            nTempXBack;
    //long          nTempZ;
    long            i;


    if( !Printer::doWorkPartZCompensation || (g_pauseStatus != PAUSE_STATUS_NONE && g_pauseStatus != PAUSE_STATUS_GOTO_PAUSE2 && g_pauseStatus != PAUSE_STATUS_TASKGOTO_PAUSE_2) )
    {
        // there is nothing to do at the moment
        return;
    }

    InterruptProtectedBlock noInts; //HAL::forbidInterrupts();
    nCurrentPositionSteps[X_AXIS] = Printer::queuePositionCurrentSteps[X_AXIS];
    nCurrentPositionSteps[Y_AXIS] = Printer::queuePositionCurrentSteps[Y_AXIS];
    nCurrentPositionSteps[Z_AXIS] = Printer::queuePositionCurrentSteps[Z_AXIS];

#if FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING
    nCurrentPositionSteps[X_AXIS] += Printer::directPositionCurrentSteps[X_AXIS];
    nCurrentPositionSteps[Y_AXIS] += Printer::directPositionCurrentSteps[Y_AXIS];
#endif // FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING
    noInts.unprotect(); //HAL::allowInterrupts();

#if DEBUG_WORK_PART_Z_COMPENSATION
    g_nLastZCompensationPositionSteps[X_AXIS] = nCurrentPositionSteps[X_AXIS];
    g_nLastZCompensationPositionSteps[Y_AXIS] = nCurrentPositionSteps[Y_AXIS];
    g_nLastZCompensationPositionSteps[Z_AXIS] = nCurrentPositionSteps[Z_AXIS];
#endif // DEBUG_WORK_PART_Z_COMPENSATION
    
    if( nCurrentPositionSteps[Z_AXIS] )
    {
        // find the rectangle which covers the current position of the miller
        nXLeftIndex = 1;
        nXLeftSteps = (long)((float)g_ZCompensationMatrix[1][0] * Printer::axisStepsPerMM[X_AXIS]);
        for( i=1; i<=g_uZMatrixMax[X_AXIS]; i++ )
        {
            nTemp = g_ZCompensationMatrix[i][0];
            nTemp = (long)((float)nTemp * Printer::axisStepsPerMM[X_AXIS]);
            if( nCurrentPositionSteps[X_AXIS] <= nTemp )
            {
                nXRightIndex = i;
                nXRightSteps = nTemp;
                break;
            }
            nXLeftIndex = i;
            nXLeftSteps = nTemp;
        }
                    
        nYFrontIndex = 1;
        nYFrontSteps = (long)((float)g_ZCompensationMatrix[0][1] * Printer::axisStepsPerMM[Y_AXIS]);
        for( i=1; i<=g_uZMatrixMax[Y_AXIS]; i++ )
        {
            nTemp = g_ZCompensationMatrix[0][i];
            nTemp = (long)((float)nTemp * Printer::axisStepsPerMM[Y_AXIS]);
            if( nCurrentPositionSteps[Y_AXIS] <= nTemp )
            {
                nYBackIndex = i;
                nYBackSteps = nTemp;
                break;
            }
            nYFrontIndex = i;
            nYFrontSteps = nTemp;
        }

        nDeltaX    = nCurrentPositionSteps[X_AXIS] - nXLeftSteps;
        nDeltaY    = nCurrentPositionSteps[Y_AXIS] - nYFrontSteps;
        nStepSizeX = nXRightSteps - nXLeftSteps;
        nStepSizeY = nYBackSteps - nYFrontSteps;

        // we do a linear interpolation in order to find our exact place within the current rectangle
        nTempXFront = g_ZCompensationMatrix[nXLeftIndex][nYFrontIndex] +
                      (g_ZCompensationMatrix[nXRightIndex][nYFrontIndex] - g_ZCompensationMatrix[nXLeftIndex][nYFrontIndex]) * nDeltaX / nStepSizeX;
        nTempXBack  = g_ZCompensationMatrix[nXLeftIndex][nYBackIndex] +
                      (g_ZCompensationMatrix[nXRightIndex][nYBackIndex] - g_ZCompensationMatrix[nXLeftIndex][nYBackIndex]) * nDeltaX / nStepSizeX;
        nNeededZCompensation = nTempXFront +
                               (nTempXBack - nTempXFront) * nDeltaY / nStepSizeY;
                    
        nNeededZCompensation += g_staticZSteps;

#if FEATURE_FIND_Z_ORIGIN
        nNeededZCompensation -= Printer::staticCompensationZ;
#endif // FEATURE_FIND_Z_ORIGIN

#if DEBUG_WORK_PART_Z_COMPENSATION
            g_nDelta[X_AXIS]    = nDeltaX;
            g_nDelta[Y_AXIS]    = nDeltaY;
            g_nStepSize[X_AXIS] = nStepSizeX;
            g_nStepSize[Y_AXIS] = nStepSizeY;
            g_nTempXFront       = nTempXFront;
            g_nTempXBack        = nTempXBack;
            g_nNeededZ          = nNeededZCompensation;
            g_uIndex[0]         = nXLeftIndex;
            g_uIndex[1]         = nXRightIndex;
            g_uIndex[2]         = nYFrontIndex;
            g_uIndex[3]         = nYBackIndex;
            g_nMatrix[0]        = g_ZCompensationMatrix[nXLeftIndex][nYFrontIndex];
            g_nMatrix[1]        = g_ZCompensationMatrix[nXRightIndex][nYFrontIndex];
            g_nMatrix[2]        = g_ZCompensationMatrix[nXLeftIndex][nYBackIndex];
            g_nMatrix[3]        = g_ZCompensationMatrix[nXRightIndex][nYBackIndex];
#endif // DEBUG_WORK_PART_Z_COMPENSATION

    }
    else
    {
        // we do not perform a compensation in case the z-position from the G-code is 0 (because this would drive the tool against the work part)
        nNeededZCompensation = g_staticZSteps;
    }

#if DEBUG_WORK_PART_Z_COMPENSATION
    long    nZDelta = Printer::compensatedPositionTargetStepsZ - nNeededZCompensation;

    if( nZDelta < g_nZDeltaMin )        g_nZDeltaMin = nZDelta;
    if( nZDelta > g_nZDeltaMax )        g_nZDeltaMax = nZDelta;

    g_nZCompensationUpdateTime = micros();

    if( Printer::compensatedPositionTargetStepsZ != Printer::compensatedPositionCurrentStepsZ )
    {
        g_nTooFast ++;
    }

    if( Printer::compensatedPositionTargetStepsZ != nNeededZCompensation )
    {
        g_nLastZCompensationTargetStepsZ = nNeededZCompensation;
        g_nZCompensationUpdates ++;
        g_debugLog = 3;
    }
#endif // DEBUG_WORK_PART_Z_COMPENSATION

    noInts.protect(); //HAL::forbidInterrupts();
    Printer::compensatedPositionTargetStepsZ = nNeededZCompensation;
    noInts.unprotect(); //HAL::allowInterrupts();

    return;

} // doWorkPartZCompensation


long getWorkPartOffset( void )
{
    long            nCurrentPositionSteps[2];
    long            nOffset;
    unsigned char   nXLeftIndex;
    unsigned char   nXRightIndex = 1;
    unsigned char   nYFrontIndex;
    unsigned char   nYBackIndex = 0;
    long            nXLeftSteps;
    long            nXRightSteps = 0;
    long            nYFrontSteps;
    long            nYBackSteps = 0;
    long            nTemp;
    long            nDeltaX;
    long            nDeltaY;
    long            nStepSizeX;
    long            nStepSizeY;
    long            nTempXFront;
    long            nTempXBack;
    long            i;


    if( !Printer::doWorkPartZCompensation )
    {
        // we determine the offset to the scanned work part only in case the work part z compensation is active
        return 0;
    }

    InterruptProtectedBlock noInts; //HAL::forbidInterrupts();
    nCurrentPositionSteps[X_AXIS] = Printer::queuePositionCurrentSteps[X_AXIS] + Printer::directPositionCurrentSteps[X_AXIS];
    nCurrentPositionSteps[Y_AXIS] = Printer::queuePositionCurrentSteps[Y_AXIS] + Printer::directPositionCurrentSteps[Y_AXIS];

#if FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING
    nCurrentPositionSteps[X_AXIS] += Printer::directPositionCurrentSteps[X_AXIS];
    nCurrentPositionSteps[Y_AXIS] += Printer::directPositionCurrentSteps[Y_AXIS];
#endif // FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING
    noInts.unprotect(); //HAL::allowInterrupts();

    // find the rectangle which covers the current position of the miller
    nXLeftIndex = 1;
    nXLeftSteps = (long)((float)g_ZCompensationMatrix[1][0] * Printer::axisStepsPerMM[X_AXIS]);
    for( i=1; i<=g_uZMatrixMax[X_AXIS]; i++ )
    {
        nTemp = g_ZCompensationMatrix[i][0];
        nTemp = (long)((float)nTemp * Printer::axisStepsPerMM[X_AXIS]);
        if( nCurrentPositionSteps[X_AXIS] <= nTemp )
        {
            nXRightIndex = i;
            nXRightSteps = nTemp;
            break;
        }
        nXLeftIndex = i;
        nXLeftSteps = nTemp;
    }
                    
    nYFrontIndex = 1;
    nYFrontSteps = (long)((float)g_ZCompensationMatrix[0][1] * Printer::axisStepsPerMM[Y_AXIS]);
    for( i=1; i<=g_uZMatrixMax[Y_AXIS]; i++ )
    {
        nTemp = g_ZCompensationMatrix[0][i];
        nTemp = (long)((float)nTemp * Printer::axisStepsPerMM[Y_AXIS]);
        if( nCurrentPositionSteps[Y_AXIS] <= nTemp )
        {
            nYBackIndex = i;
            nYBackSteps = nTemp;
            break;
        }
        nYFrontIndex = i;
        nYFrontSteps = nTemp;
    }

    nDeltaX    = nCurrentPositionSteps[X_AXIS] - nXLeftSteps;
    nDeltaY    = nCurrentPositionSteps[Y_AXIS] - nYFrontSteps;
    nStepSizeX = nXRightSteps - nXLeftSteps;
    nStepSizeY = nYBackSteps - nYFrontSteps;

    // we do a linear interpolation in order to find our exact place within the current rectangle
    nTempXFront = g_ZCompensationMatrix[nXLeftIndex][nYFrontIndex] +
                  (g_ZCompensationMatrix[nXRightIndex][nYFrontIndex] - g_ZCompensationMatrix[nXLeftIndex][nYFrontIndex]) * nDeltaX / nStepSizeX;
    nTempXBack  = g_ZCompensationMatrix[nXLeftIndex][nYBackIndex] +
                  (g_ZCompensationMatrix[nXRightIndex][nYBackIndex] - g_ZCompensationMatrix[nXLeftIndex][nYBackIndex]) * nDeltaX / nStepSizeX;
    nOffset     = nTempXFront +
                  (nTempXBack - nTempXFront) * nDeltaY / nStepSizeY;

/*  Com::printF( PSTR( "getWorkPartOffset();" ), nXLeftIndex );
    Com::printF( Com::tSemiColon, nXRightIndex );
    Com::printF( Com::tSemiColon, nYFrontIndex );
    Com::printF( Com::tSemiColon, nYBackIndex );
    Com::printF( Com::tSemiColon, nOffset );
    Com::printF( Com::tSemiColon, Printer::staticCompensationZ );
    Com::printFLN( PSTR( "" ) );
*/
#if FEATURE_FIND_Z_ORIGIN
    nOffset -= Printer::staticCompensationZ;
#endif // FEATURE_FIND_Z_ORIGIN
    
    return nOffset;

} // getWorkPartOffset


void determineStaticCompensationZ( void )
{
    long    nXLeftIndex;
    long    nXRightIndex = 0;
    long    nYFrontIndex;
    long    nYBackIndex = 0;
    long    nXLeftSteps;
    long    nXRightSteps = 0;
    long    nYFrontSteps;
    long    nYBackSteps = 0;
    long    nTemp;
    long    nDeltaX;
    long    nDeltaY;
    //long  nDeltaZ;
    long    nStepSizeX;
    long    nStepSizeY;
    long    nTempXFront;
    long    nTempXBack;
    long    i;


    // find the rectangle which covers the current position of the tool
    nXLeftIndex = 1;
    nXLeftSteps = (long)((float)g_ZCompensationMatrix[1][0] * Printer::axisStepsPerMM[X_AXIS]);
    for( i=1; i<=g_uZMatrixMax[X_AXIS]; i++ )
    {
        nTemp = g_ZCompensationMatrix[i][0];
        nTemp = (long)((float)nTemp * Printer::axisStepsPerMM[X_AXIS]);
        if( (g_nZOriginPosition[X_AXIS]) <= nTemp )
        {
            nXRightIndex = i;
            nXRightSteps = nTemp;
            break;
        }
        nXLeftIndex = i;
        nXLeftSteps = nTemp;
    }
                    
    nYFrontIndex = 1;
    nYFrontSteps = (long)((float)g_ZCompensationMatrix[0][1] * Printer::axisStepsPerMM[Y_AXIS]);
    for( i=1; i<=g_uZMatrixMax[Y_AXIS]; i++ )
    {
        nTemp = g_ZCompensationMatrix[0][i];
        nTemp = (long)((float)nTemp * Printer::axisStepsPerMM[Y_AXIS]);
        if( (g_nZOriginPosition[Y_AXIS]) <= nTemp )
        {
            nYBackIndex = i;
            nYBackSteps = nTemp;
            break;
        }
        nYFrontIndex = i;
        nYFrontSteps = nTemp;
    }

    nDeltaX    = g_nZOriginPosition[X_AXIS] - nXLeftSteps;
    nDeltaY    = g_nZOriginPosition[Y_AXIS] - nYFrontSteps;
    nStepSizeX = nXRightSteps - nXLeftSteps;
    nStepSizeY = nYBackSteps - nYFrontSteps;

    // we do a linear interpolation in order to find our exact place within the current rectangle
    nTempXFront = g_ZCompensationMatrix[nXLeftIndex][nYFrontIndex] +
                  (g_ZCompensationMatrix[nXRightIndex][nYFrontIndex] - g_ZCompensationMatrix[nXLeftIndex][nYFrontIndex]) * nDeltaX / nStepSizeX;
    nTempXBack  = g_ZCompensationMatrix[nXLeftIndex][nYBackIndex] +
                  (g_ZCompensationMatrix[nXRightIndex][nYBackIndex] - g_ZCompensationMatrix[nXLeftIndex][nYBackIndex]) * nDeltaX / nStepSizeX;
        
    Printer::staticCompensationZ = nTempXFront +
                                   (nTempXBack - nTempXFront) * nDeltaY / nStepSizeY;
    
    return;

} // determineStaticCompensationZ

#endif // FEATURE_WORK_PART_Z_COMPENSATION


#if FEATURE_HEAT_BED_Z_COMPENSATION || FEATURE_WORK_PART_Z_COMPENSATION
short readIdlePressure( short* pnIdlePressure )
{
    short   nTempPressure;
    char    nTemp;


    // determine the pressure when the heat bed is far away - wait until the measured pressure is rather stable
    nTempPressure = 0;
    if( readAveragePressure( pnIdlePressure ) )
    {
        // we were unable to determine the pressure
        if( Printer::debugErrors() )
        {
            Com::printFLN( PSTR( "readIdlePressure(): the pressure could not be determined" ) );
        }
        return -1;
    }

    nTemp = 0;
    while( abs( nTempPressure - *pnIdlePressure) > 5 )
    {
        if( Printer::debugInfo() )
        {
            Com::printF( PSTR( "readIdlePressure(): pressure calibration: " ), nTempPressure );
            Com::printFLN( PSTR( " / " ), *pnIdlePressure );
        }

        nTemp ++;
        if( nTemp >= 5 )
        {
            // we are unable to receive stable values - do not hang here forever
            if( Printer::debugErrors() )
            {
                Com::printF( PSTR( "readIdlePressure(): the pressure is not constant: " ), nTempPressure );
                Com::printFLN( PSTR( " / " ), *pnIdlePressure );
            }
            return -1;
        }
    
        nTempPressure = *pnIdlePressure;
        if( readAveragePressure( pnIdlePressure ) )
        {
            // we were unable to determine the pressure
            if( Printer::debugErrors() )
            {
                Com::printFLN( PSTR( "readIdlePressure(): the pressure could not be determined" ) );
            }
            return -1;
        }

        // wait some extra amount of time in case our results were not constant enough
        HAL::delayMilliseconds( 500 );
        
        //runStandardTasks();
        Commands::checkForPeriodicalActions(); 
    }

    if( Printer::debugInfo() )
    {
        Com::printFLN( PSTR( "readIdlePressure(): idle pressure: " ), *pnIdlePressure );
    }

    if( *pnIdlePressure < g_nScanIdlePressureMin || *pnIdlePressure > g_nScanIdlePressureMax )
    {
        // the idle pressure is out of range
        if( Printer::debugErrors() )
        {
            Com::printFLN( PSTR( "readIdlePressure(): the idle pressure is out of range" ) );
        }
        return -1;
    }

    // at this point we know the idle pressure
    return 0;

} // readIdlePressure


short testIdlePressure( void )
{
    short   nTempPressure;
    //short nTemp;


    if( readAveragePressure( &nTempPressure ) )
    {
        // some error has occurred
        if( Printer::debugErrors() )
        {
            Com::printFLN( PSTR( "testIdlePressure(): the pressure could not be determined" ) );
        }
        return -1;
    }
    g_nCurrentIdlePressure = nTempPressure;
    return 0;

} // testIdlePressure


short readAveragePressure( short* pnAveragePressure )
{
    short   i;
    short   nTempPressure;
    short   nMinPressure;
    short   nMaxPressure;
    long    nPressureSum;
    char    nTemp;


    nTemp = 0;
    while( 1 )
    {
        // we read the strain gauge multiple times and check the variance
        nPressureSum = 0;
        nMinPressure = 32000;
        nMaxPressure = -32000;
        for( i=0; i<g_nScanPressureReads; i++)
        {
            HAL::delayMilliseconds( g_nScanPressureReadDelay );
            nTempPressure =  readStrainGauge( ACTIVE_STRAIN_GAUGE );
            nPressureSum  += nTempPressure;
            if( nTempPressure < nMinPressure )  nMinPressure = nTempPressure;
            if( nTempPressure > nMaxPressure )  nMaxPressure = nTempPressure;
        }
        nTempPressure = (short)(nPressureSum / g_nScanPressureReads);

        if( (nMaxPressure - nMinPressure) < g_nScanPressureTolerance )
        {
            // we have good results
            *pnAveragePressure = nTempPressure;
            return 0;
        }

        nTemp ++;
        if( nTemp >= 5 )
        {
            // we are unable to receive stable values - do not hang here forever
            if( Printer::debugErrors() )
            {
                Com::printF( PSTR( "readAveragePressure(): the pressure is not constant: " ), nMinPressure );
                Com::printF( PSTR( " / " ), nTempPressure );
                Com::printFLN( PSTR( " / " ), nMaxPressure );
            }
            break;
        }
    
        // wait some extra amount of time in case our results were not constant enough
        HAL::delayMilliseconds( 100 );
        
        //runStandardTasks();
        Commands::checkForPeriodicalActions(); 
    }

    if( Printer::debugErrors() )
    {
        Com::printFLN( PSTR( "readAveragePressure(): the pressure is not plausible" ) );
    }
    g_abortZScan       = 1;
    *pnAveragePressure = 0;
    return -1;

} // readAveragePressure


short moveZUpFast( bool execRunStandardTasks )
{
    short   nTempPressure;
    short   nZ = 0;
    short   nSteps;


    // move the heat bed up until we detect the contact pressure (fast speed)
    while( 1 )
    {
        HAL::delayMilliseconds( g_nScanFastStepDelay );
        if( readAveragePressure( &nTempPressure ) )
        {
            // some error has occurred
            break;
        }

        if( nTempPressure > g_nMaxPressureContact || nTempPressure < g_nMinPressureContact )
        {
            // we have reached the target pressure
            break;
        }

        nSteps            =  moveZ( g_nScanHeatBedUpFastSteps );
        nZ                += nSteps;
        g_nZScanZPosition += nSteps;

        if(execRunStandardTasks) {
          runStandardTasks();
        }
        else {
          Commands::checkForPeriodicalActions();
          GCode::keepAlive( Processing );
        }

        if( g_abortZScan )
        {
            break;
        }

        if( g_nZScanZPosition < -g_nScanZMaxCompensationSteps || g_nZScanZPosition > HEAT_BED_SCAN_Z_START_STEPS )
        {
            if( Printer::debugErrors() )
            {
                Com::printFLN( PSTR( "moveZUpFast(): the z position went out of range, retries = " ), (int)g_scanRetries );
            }
            
            if( g_scanRetries ) g_retryZScan = 1;
            else                g_abortZScan = 1;
            break;
        }
    }

    return nZ;

} // moveZUpFast


short moveZDownSlow( bool execRunStandardTasks )
{
    short   nTempPressure;
    short   nZ = 0;
    short   nSteps;


    // move the heat bed down until we detect the retry pressure (slow speed)
    while( 1 )
    {
        HAL::delayMilliseconds( g_nScanSlowStepDelay );
        if( readAveragePressure( &nTempPressure ) )
        {
            // some error has occurred
            break;
        }

        if( nTempPressure < g_nMaxPressureRetry && nTempPressure > g_nMinPressureRetry )
        {
            // we have reached the target pressure
            break;
        }

        nSteps            =  moveZ( g_nScanHeatBedDownSlowSteps );
        nZ                += nSteps;
        g_nZScanZPosition += nSteps;

        if(execRunStandardTasks) {
          runStandardTasks();
        }
        else {
          Commands::checkForPeriodicalActions();
        }

        if( g_abortZScan )
        {
            break;
        }

        if( g_nZScanZPosition < -g_nScanZMaxCompensationSteps || g_nZScanZPosition > HEAT_BED_SCAN_Z_START_STEPS )
        {
            if( Printer::debugErrors() )
            {
                Com::printFLN( PSTR( "moveZDownSlow(): the z position went out of range, retries = " ), g_scanRetries );
            }
            
            if( g_scanRetries ) g_retryZScan = 1;
            else                g_abortZScan = 1;
            break;
        }
        if( g_nLastZScanZPosition )
        {
            if( (g_nZScanZPosition > g_nLastZScanZPosition && (g_nZScanZPosition - g_nLastZScanZPosition) > g_nScanHeatBedDownFastSteps) ||
                (g_nZScanZPosition < g_nLastZScanZPosition && (g_nLastZScanZPosition - g_nZScanZPosition) > g_nScanHeatBedDownFastSteps) )
            {
                if( Printer::debugErrors() )
                {
                    Com::printFLN( PSTR( "moveZDownSlow(): the z position delta went out of range, retries = " ), g_scanRetries );
                }
            
                if( g_scanRetries ) g_retryZScan = 1;
                else                g_abortZScan = 1;
                break;
            }
        }
    }

    return nZ;

} // moveZDownSlow


short moveZUpSlow( short* pnContactPressure, bool execRunStandardTasks )
{
    short   nTempPressure;
    short   nZ = 0;
    short   nSteps;


    // move the heat bed up until we detect the contact pressure (slow speed)
    while( 1 )
    {
        HAL::delayMilliseconds( g_nScanSlowStepDelay );
        if( readAveragePressure( &nTempPressure ) )
        {
            // some error has occurred
            break;
        }

        if( nTempPressure > g_nMaxPressureContact || nTempPressure < g_nMinPressureContact )
        {
            // we have found the proper pressure
            break;
        }

        nSteps            =  moveZ( g_nScanHeatBedUpSlowSteps );
        nZ                += nSteps;
        g_nZScanZPosition += nSteps;

        if(execRunStandardTasks) {
          runStandardTasks();
        }
        else {
          Commands::checkForPeriodicalActions();
        }

        if( g_abortZScan )
        {
            break;
        }

        if( g_nZScanZPosition < -g_nScanZMaxCompensationSteps || g_nZScanZPosition > HEAT_BED_SCAN_Z_START_STEPS )
        {
            if( Printer::debugErrors() )
            {
                Com::printFLN( PSTR( "moveZUpSlow(): the z position went out of range, retries = " ), g_scanRetries );
            }
            
            if( g_scanRetries ) g_retryZScan = 1;
            else                g_abortZScan = 1;
            break;
        }
    }

    *pnContactPressure = nTempPressure;
    return nZ;

} // moveZUpSlow


short moveZDownFast( bool execRunStandardTasks )
{
    short   nTempPressure;
    short   nZ = 0;
    short   nSteps;

    // move the heat bed down so that we won't hit it when we move to the next position
    g_nLastZScanZPosition = g_nZScanZPosition;
    HAL::delayMilliseconds( g_nScanFastStepDelay );

    nSteps            =  moveZ( g_nScanHeatBedDownFastSteps );
    nZ                += nSteps;
    g_nZScanZPosition += nSteps;

    if(execRunStandardTasks) {
      runStandardTasks();
    }
    else {
      Commands::checkForPeriodicalActions();
    }

    if( readAveragePressure( &nTempPressure ) )
    {
        // some error has occurred
        if( Printer::debugErrors() )
        {
            Com::printFLN( PSTR( "moveZDownFast(): the pressure could not be determined" ) );
        }
        g_abortZScan = 1;
        return nZ;
    }

#if DEBUG_HEAT_BED_SCAN || DEBUG_WORK_PART_SCAN
    if( Printer::debugInfo() )
    {
        Com::printFLN( PSTR( "moveZDownFast(): " ), (int)nTempPressure );
    }
#endif // DEBUG_HEAT_BED_SCAN || DEBUG_WORK_PART_SCAN
    return nZ;

} // moveZDownFast


int moveZ( int nSteps )
{
    /*
    Warning 03.11.2017 : Do not try to make more steps than < 10mm in one row. Some printers will get a watchdog reset.
    When choosing 10mm one printer crashed while others still worked.
    We changed Scan PLA/ABS to 2x 5mm and it worked.
    Reason is because we removed watchdog-ping from HAL::delayMicroseconds (which was good!)
    */
    
    int     i;
    int     nMaxLoops;
    char    bBreak;
    

    // Warning: this function does not check any end stops
    // choose the direction
    if( nSteps >= 0 )
    {
        nMaxLoops = nSteps;

        if( g_nTempDirectionZ != 1 )
        {
            prepareBedDown();

            HAL::delayMicroseconds( XYZ_DIRECTION_CHANGE_DELAY );
            g_nTempDirectionZ = 1;
        }
    }
    else
    {
        nMaxLoops = -nSteps;

        if( g_nTempDirectionZ != -1 )
        {
            prepareBedUp();

            HAL::delayMicroseconds( XYZ_DIRECTION_CHANGE_DELAY );
            g_nTempDirectionZ = -1;
        }
    }
    
    // perform the steps
    for( i=0; i<nMaxLoops; i++ )
    {
        bBreak = 0;

#if FEATURE_HEAT_BED_Z_COMPENSATION || FEATURE_WORK_PART_Z_COMPENSATION
        if( g_abortZScan )
        {
            bBreak = 1;
        }
#endif // FEATURE_HEAT_BED_Z_COMPENSATION || FEATURE_WORK_PART_Z_COMPENSATION
        
#if FEATURE_FIND_Z_ORIGIN
        if( g_abortSearch )
        {
            bBreak = 1;
        }
#endif // FEATURE_FIND_Z_ORIGIN

        if( bBreak )
        {
            // do not continue here in case the current operation has been cancelled
            if( nSteps > 0 )    nSteps = i;
            else                nSteps = -i;
            break;
        }

        if( nSteps >= 0 )
        {
            if( READ( Z_DIR_PIN ) != !INVERT_Z_DIR )
            {
                prepareBedDown();

                HAL::delayMicroseconds( XYZ_DIRECTION_CHANGE_DELAY );
                g_nTempDirectionZ = 1;

                Com::printFLN( PSTR( "moveZ(): prepareBedDown()" ) );
            }
        }
        else
        {
            if( READ( Z_DIR_PIN ) != INVERT_Z_DIR )
            {
                prepareBedUp();

                HAL::delayMicroseconds( XYZ_DIRECTION_CHANGE_DELAY );
                g_nTempDirectionZ = -1;

                Com::printFLN( PSTR( "moveZ(): prepareBedUp()" ) );
            }
        }

        HAL::delayMicroseconds( XYZ_STEPPER_HIGH_DELAY );
        startZStep( g_nTempDirectionZ );

        HAL::delayMicroseconds( XYZ_STEPPER_LOW_DELAY );
        endZStep();
    }

    return nSteps;

} // moveZ


void restoreDefaultScanParameters( void )
{
#if FEATURE_MILLING_MODE
    if( Printer::operatingMode == OPERATING_MODE_PRINT )
    {
#if FEATURE_HEAT_BED_Z_COMPENSATION
        // we must restore the default scan parameters for the heat bed scan
        g_nScanXStartSteps           = HEAT_BED_SCAN_X_START_STEPS;
        g_nScanXStepSizeMm           = HEAT_BED_SCAN_X_STEP_SIZE_MM;
        g_nScanXStepSizeSteps        = HEAT_BED_SCAN_X_STEP_SIZE_STEPS;
        g_nScanXEndSteps             = HEAT_BED_SCAN_X_END_STEPS;
        g_nScanXMaxPositionSteps     = HEAT_BED_SCAN_X_MAX_POSITION_STEPS;

        g_nScanYStartSteps           = HEAT_BED_SCAN_Y_START_STEPS;
        g_nScanYStepSizeMm           = HEAT_BED_SCAN_Y_STEP_SIZE_MM;
        g_nScanYStepSizeSteps        = HEAT_BED_SCAN_Y_STEP_SIZE_STEPS;
        g_nScanYEndSteps             = HEAT_BED_SCAN_Y_END_STEPS;
        g_nScanYMaxPositionSteps     = HEAT_BED_SCAN_Y_MAX_POSITION_STEPS;

        g_nScanHeatBedUpFastSteps    = HEAT_BED_SCAN_UP_FAST_STEPS;
        g_nScanHeatBedUpSlowSteps    = HEAT_BED_SCAN_UP_SLOW_STEPS;
        g_nScanHeatBedDownFastSteps  = HEAT_BED_SCAN_DOWN_FAST_STEPS;
        g_nScanHeatBedDownSlowSteps  = HEAT_BED_SCAN_DOWN_SLOW_STEPS;
        g_nScanZMaxCompensationSteps = HEAT_BED_SCAN_Z_SCAN_MAX_STEPS;
        g_nScanFastStepDelay         = HEAT_BED_SCAN_FAST_STEP_DELAY_MS;
        g_nScanSlowStepDelay         = HEAT_BED_SCAN_SLOW_STEP_DELAY_MS;
        g_nScanIdleDelay             = HEAT_BED_SCAN_IDLE_DELAY_MS;

        g_nScanContactPressureDelta  = HEAT_BED_SCAN_CONTACT_PRESSURE_DELTA;
        g_nScanRetryPressureDelta    = HEAT_BED_SCAN_RETRY_PRESSURE_DELTA;
        g_nScanIdlePressureDelta     = HEAT_BED_SCAN_IDLE_PRESSURE_DELTA;
        g_nScanIdlePressureMin       = HEAT_BED_SCAN_IDLE_PRESSURE_MIN;
        g_nScanIdlePressureMax       = HEAT_BED_SCAN_IDLE_PRESSURE_MAX;

        g_nScanPressureReads         = HEAT_BED_SCAN_PRESSURE_READS;
        g_nScanPressureTolerance     = HEAT_BED_SCAN_PRESSURE_TOLERANCE;
        g_nScanPressureReadDelay     = HEAT_BED_SCAN_PRESSURE_READ_DELAY_MS;
#endif // FEATURE_HEAT_BED_Z_COMPENSATION
    }
    else
    {
#if FEATURE_WORK_PART_Z_COMPENSATION
        // we must restore the default scan parameters for the work part scan
        g_nScanXStartSteps           = WORK_PART_SCAN_X_START_STEPS;
        g_nScanXStepSizeMm           = WORK_PART_SCAN_X_STEP_SIZE_MM;
        g_nScanXStepSizeSteps        = WORK_PART_SCAN_X_STEP_SIZE_STEPS;
        g_nScanXEndSteps             = WORK_PART_SCAN_X_END_STEPS;
        g_nScanXMaxPositionSteps     = WORK_PART_SCAN_X_MAX_POSITION_STEPS;

        g_nScanYStartSteps           = WORK_PART_SCAN_Y_START_STEPS;
        g_nScanYStepSizeMm           = WORK_PART_SCAN_Y_STEP_SIZE_MM;
        g_nScanYStepSizeSteps        = WORK_PART_SCAN_Y_STEP_SIZE_STEPS;
        g_nScanYEndSteps             = WORK_PART_SCAN_Y_END_STEPS;
        g_nScanYMaxPositionSteps     = WORK_PART_SCAN_Y_MAX_POSITION_STEPS;

        g_nScanHeatBedUpFastSteps    = WORK_PART_SCAN_UP_FAST_STEPS;
        g_nScanHeatBedUpSlowSteps    = WORK_PART_SCAN_UP_SLOW_STEPS;
        g_nScanHeatBedDownFastSteps  = WORK_PART_SCAN_DOWN_FAST_STEPS;
        g_nScanHeatBedDownSlowSteps  = WORK_PART_SCAN_DOWN_SLOW_STEPS;
        g_nScanZMaxCompensationSteps = WORK_PART_Z_COMPENSATION_MAX_STEPS;
        g_nScanFastStepDelay         = WORK_PART_SCAN_FAST_STEP_DELAY_MS;
        g_nScanSlowStepDelay         = WORK_PART_SCAN_SLOW_STEP_DELAY_MS;
        g_nScanIdleDelay             = WORK_PART_SCAN_IDLE_DELAY_MS;

#if FEATURE_CONFIGURABLE_MILLER_TYPE
        if( Printer::MillerType == MILLER_TYPE_ONE_TRACK )
        {
            g_nScanContactPressureDelta = MT1_WORK_PART_SCAN_CONTACT_PRESSURE_DELTA;
            g_nScanRetryPressureDelta   = MT1_WORK_PART_SCAN_RETRY_PRESSURE_DELTA;
        }
        else
        {
            g_nScanContactPressureDelta = MT2_WORK_PART_SCAN_CONTACT_PRESSURE_DELTA;
            g_nScanRetryPressureDelta   = MT2_WORK_PART_SCAN_RETRY_PRESSURE_DELTA;
        }
#else
        g_nScanContactPressureDelta = WORK_PART_SCAN_CONTACT_PRESSURE_DELTA;
        g_nScanRetryPressureDelta   = WORK_PART_SCAN_RETRY_PRESSURE_DELTA;
#endif // FEATURE_CONFIGURABLE_MILLER_TYPE

        g_nScanIdlePressureDelta    = WORK_PART_SCAN_IDLE_PRESSURE_DELTA;
        g_nScanIdlePressureMin      = WORK_PART_SCAN_IDLE_PRESSURE_MIN;
        g_nScanIdlePressureMax      = WORK_PART_SCAN_IDLE_PRESSURE_MAX;

        g_nScanPressureReads        = WORK_PART_SCAN_PRESSURE_READS;
        g_nScanPressureTolerance    = WORK_PART_SCAN_PRESSURE_TOLERANCE;
        g_nScanPressureReadDelay    = WORK_PART_SCAN_PRESSURE_READ_DELAY_MS;
#endif // FEATURE_WORK_PART_Z_COMPENSATION
    }
#else
#if FEATURE_HEAT_BED_Z_COMPENSATION
    // we must restore the default scan parameters for the heat bed scan
    g_nScanXStartSteps           = HEAT_BED_SCAN_X_START_STEPS;
    g_nScanXStepSizeMm           = HEAT_BED_SCAN_X_STEP_SIZE_MM;
    g_nScanXStepSizeSteps        = HEAT_BED_SCAN_X_STEP_SIZE_STEPS;
    g_nScanXEndSteps             = HEAT_BED_SCAN_X_END_STEPS;
    g_nScanXMaxPositionSteps     = HEAT_BED_SCAN_X_MAX_POSITION_STEPS;

    g_nScanYStartSteps           = HEAT_BED_SCAN_Y_START_STEPS;
    g_nScanYStepSizeMm           = HEAT_BED_SCAN_Y_STEP_SIZE_MM;
    g_nScanYStepSizeSteps        = HEAT_BED_SCAN_Y_STEP_SIZE_STEPS;
    g_nScanYEndSteps             = HEAT_BED_SCAN_Y_END_STEPS;
    g_nScanYMaxPositionSteps     = HEAT_BED_SCAN_Y_MAX_POSITION_STEPS;

    g_nScanHeatBedUpFastSteps    = HEAT_BED_SCAN_UP_FAST_STEPS;
    g_nScanHeatBedUpSlowSteps    = HEAT_BED_SCAN_UP_SLOW_STEPS;
    g_nScanHeatBedDownFastSteps  = HEAT_BED_SCAN_DOWN_FAST_STEPS;
    g_nScanHeatBedDownSlowSteps  = HEAT_BED_SCAN_DOWN_SLOW_STEPS;
    g_nScanZMaxCompensationSteps = HEAT_BED_SCAN_Z_SCAN_MAX_STEPS;
    g_nScanFastStepDelay         = HEAT_BED_SCAN_FAST_STEP_DELAY_MS;
    g_nScanSlowStepDelay         = HEAT_BED_SCAN_SLOW_STEP_DELAY_MS;
    g_nScanIdleDelay             = HEAT_BED_SCAN_IDLE_DELAY_MS;

    g_nScanContactPressureDelta  = HEAT_BED_SCAN_CONTACT_PRESSURE_DELTA;
    g_nScanRetryPressureDelta    = HEAT_BED_SCAN_RETRY_PRESSURE_DELTA;
    g_nScanIdlePressureDelta     = HEAT_BED_SCAN_IDLE_PRESSURE_DELTA;
    g_nScanIdlePressureMin       = HEAT_BED_SCAN_IDLE_PRESSURE_MIN;
    g_nScanIdlePressureMax       = HEAT_BED_SCAN_IDLE_PRESSURE_MAX;

    g_nScanPressureReads         = HEAT_BED_SCAN_PRESSURE_READS;
    g_nScanPressureTolerance     = HEAT_BED_SCAN_PRESSURE_TOLERANCE;
    g_nScanPressureReadDelay     = HEAT_BED_SCAN_PRESSURE_READ_DELAY_MS;
#endif // FEATURE_HEAT_BED_Z_COMPENSATION
#endif // FEATURE_MILLING_MODE

/*  if( Printer::debugInfo() )
    {
        Com::printFLN( PSTR( "restoreDefaultScanParameters(): the default scan parameters have been restored" ) );
    }
*/  return;

} // restoreDefaultScanParameters


void outputScanParameters( void )
{
    if( Printer::debugInfo() )
    {
        Com::printFLN( PSTR( "outputScanParameters(): current scan parameters:" ) );

        Com::printF( PSTR( "" ), Printer::axisStepsPerMM[X_AXIS] );     Com::printFLN( PSTR( ";[steps];axisStepsPerMM[X_AXIS]" ) );
        Com::printF( PSTR( "" ), Printer::axisStepsPerMM[Y_AXIS] );     Com::printFLN( PSTR( ";[steps];axisStepsPerMM[Y_AXIS]" ) );
        Com::printF( PSTR( "" ), Printer::axisStepsPerMM[Z_AXIS] );     Com::printFLN( PSTR( ";[steps];axisStepsPerMM[Z_AXIS]" ) );

        Com::printF( PSTR( "" ), g_nScanXStartSteps );                  Com::printFLN( PSTR( ";[steps];g_nScanXStartSteps" ) );
        Com::printF( PSTR( "" ), g_nScanXStepSizeSteps );               Com::printFLN( PSTR( ";[steps];g_nScanXStepSizeSteps" ) );
        Com::printF( PSTR( "" ), g_nScanXEndSteps );                    Com::printFLN( PSTR( ";[steps];g_nScanXEndSteps" ) );
        Com::printF( PSTR( "" ), g_nScanXMaxPositionSteps );            Com::printFLN( PSTR( ";[steps];g_nScanXMaxPositionSteps" ) );

        Com::printF( PSTR( "" ), g_nScanYStartSteps );                  Com::printFLN( PSTR( ";[steps];g_nScanYStartSteps" ) );
        Com::printF( PSTR( "" ), g_nScanYStepSizeSteps );               Com::printFLN( PSTR( ";[steps];g_nScanYStepSizeSteps" ) );
        Com::printF( PSTR( "" ), g_nScanYEndSteps );                    Com::printFLN( PSTR( ";[steps];g_nScanYEndSteps" ) );
        Com::printF( PSTR( "" ), g_nScanYMaxPositionSteps );            Com::printFLN( PSTR( ";[steps];g_nScanYMaxPositionSteps" ) );

        Com::printF( PSTR( "" ), (int)g_nScanHeatBedUpFastSteps );      Com::printFLN( PSTR( ";[steps];g_nScanHeatBedUpFastSteps" ) );
        Com::printF( PSTR( "" ), (int)g_nScanHeatBedUpSlowSteps );      Com::printFLN( PSTR( ";[steps];g_nScanHeatBedUpSlowSteps" ) );
        Com::printF( PSTR( "" ), (int)g_nScanHeatBedDownFastSteps );    Com::printFLN( PSTR( ";[steps];g_nScanHeatBedDownFastSteps" ) );
        Com::printF( PSTR( "" ), (int)g_nScanHeatBedDownSlowSteps );    Com::printFLN( PSTR( ";[steps];g_nScanHeatBedDownSlowSteps" ) );
        Com::printF( PSTR( "" ), (int)g_nScanFastStepDelay );           Com::printFLN( PSTR( ";[ms];g_nScanFastStepDelay" ) );
        Com::printF( PSTR( "" ), (int)g_nScanSlowStepDelay );           Com::printFLN( PSTR( ";[ms];g_nScanSlowStepDelay" ) );
        Com::printF( PSTR( "" ), (int)g_nScanIdleDelay );               Com::printFLN( PSTR( ";[ms];g_nScanIdleDelay" ) );

        Com::printF( PSTR( "" ), (int)g_nScanContactPressureDelta );    Com::printFLN( PSTR( ";[digits];g_nScanContactPressureDelta" ) );
        Com::printF( PSTR( "" ), (int)g_nScanRetryPressureDelta );      Com::printFLN( PSTR( ";[digits];g_nScanRetryPressureDelta" ) );
        Com::printF( PSTR( "" ), (int)g_nScanIdlePressureDelta );       Com::printFLN( PSTR( ";[digits];g_nScanIdlePressureDelta" ) );

        Com::printF( PSTR( "" ), (int)g_nScanPressureReads );           Com::printFLN( PSTR( ";[-];g_nScanPressureReads" ) );
        Com::printF( PSTR( "" ), (int)g_nScanPressureTolerance );       Com::printFLN( PSTR( ";[digits];g_nScanPressureTolerance" ) );
        Com::printF( PSTR( "" ), (int)g_nScanPressureReadDelay );       Com::printFLN( PSTR( ";[ms];g_nScanPressureReadDelay" ) );
    }
    return;

} // outputScanParameters


void initCompensationMatrix( void )
{
    // clear all fields of the compensation matrix
    memset( g_ZCompensationMatrix, 0, COMPENSATION_MATRIX_MAX_X*COMPENSATION_MATRIX_MAX_Y*2 );
    return;

} // initCompensationMatrix


void outputCompensationMatrix( char format )
{
    if( Printer::debugInfo() )
    {
        short   x;
        short   y;


//      Com::printFLN( PSTR( "z compensation matrix:" ) );
        Com::printFLN( PSTR( "front left ... front right" ) );
        Com::printFLN( PSTR( "...        ...         ..." ) );
        Com::printFLN( PSTR( "back left  ...  back right" ) );

        for( y=0; y<=g_uZMatrixMax[Y_AXIS]; y++ )
        {
            for( x=0; x<=g_uZMatrixMax[X_AXIS]; x++ )
            {
                if( x == 0 || y == 0 )
                {
                    Com::printF( Com::tSemiColon, g_ZCompensationMatrix[x][y] );
                }
                else
                {
                    if( format )
                    {
                        // output in [mm]
                        Com::printF( Com::tSemiColon, g_ZCompensationMatrix[x][y] / Printer::axisStepsPerMM[Z_AXIS] );
                    }
                    else
                    {
                        // output in [steps]
                        Com::printF( Com::tSemiColon, g_ZCompensationMatrix[x][y] );
                    }
                }
            }
            Com::printFLN( PSTR( " " ) );
        }

#if FEATURE_HEAT_BED_Z_COMPENSATION
        Com::printF( PSTR( "offset = " ), g_offsetZCompensationSteps );
        Com::printF( PSTR( " [steps] (= " ), (float)g_offsetZCompensationSteps * Printer::invAxisStepsPerMM[Z_AXIS] );
        Com::printFLN( PSTR( " [mm])" ) );
#endif // FEATURE_HEAT_BED_Z_COMPENSATION

        Com::printFLN( PSTR( "g_uZMatrixMax[X_AXIS] = " ), g_uZMatrixMax[X_AXIS] );
        Com::printFLN( PSTR( "g_uZMatrixMax[Y_AXIS] = " ), g_uZMatrixMax[Y_AXIS] );

#if FEATURE_HEAT_BED_Z_COMPENSATION
#if FEATURE_MILLING_MODE
        if( Printer::operatingMode == OPERATING_MODE_PRINT )
#endif // FEATURE_MILLING_MODE
        {
            Com::printFLN( PSTR( "g_nActiveHeatBed = " ), g_nActiveHeatBed );
        }
#endif // FEATURE_HEAT_BED_Z_COMPENSATION

#if FEATURE_WORK_PART_Z_COMPENSATION && FEATURE_MILLING_MODE
        if( Printer::operatingMode == OPERATING_MODE_MILL )
        {
            Com::printFLN( PSTR( "g_nActiveWorkPart = " ), g_nActiveWorkPart );
            Com::printF( PSTR( "scan start: x = " ), (float)g_nScanXStartSteps / Printer::axisStepsPerMM[X_AXIS] );
            Com::printF( PSTR( ", y = " ), (float)g_nScanYStartSteps / Printer::axisStepsPerMM[Y_AXIS] );
            Com::printFLN( PSTR( " [mm]" ) );
            Com::printF( PSTR( "scan steps: x = " ), (float)g_nScanXStepSizeMm );
            Com::printF( PSTR( ", y = " ), (float)g_nScanYStepSizeMm );
            Com::printFLN( PSTR( " [mm]" ) );
            Com::printF( PSTR( "scan end: x = " ), (float)g_nScanXMaxPositionSteps / Printer::axisStepsPerMM[X_AXIS] );
            Com::printF( PSTR( ", y = " ), (float)g_nScanYMaxPositionSteps / Printer::axisStepsPerMM[Y_AXIS] );
            Com::printFLN( PSTR( " [mm]" ) );
        }
#endif // FEATURE_WORK_PART_Z_COMPENSATION && FEATURE_MILLING_MODE
    }

    return;

} // outputCompensationMatrix


char prepareCompensationMatrix( void )
{
    short   x;
    short   y;


    // perform some safety checks first
    if( g_ZCompensationMatrix[0][0] != EEPROM_FORMAT )
    {
        if( Printer::debugErrors() )
        {
            Com::printF( PSTR( "prepareCompensationMatrix(): invalid version detected: " ), g_ZCompensationMatrix[0][0] );
            Com::printF( PSTR( " (expected: " ), EEPROM_FORMAT );
            Com::printFLN( PSTR( ")" ) );
        }
        return -1;
    }
    
    if( g_uZMatrixMax[X_AXIS] >= COMPENSATION_MATRIX_MAX_X || g_uZMatrixMax[X_AXIS] < 2 )
    {
        if( Printer::debugErrors() )
        {
            Com::printF( PSTR( "prepareCompensationMatrix(): invalid x dimension detected: " ), g_uZMatrixMax[X_AXIS] );
            Com::printF( PSTR( " (max expected: " ), COMPENSATION_MATRIX_MAX_X -1 );
            Com::printFLN( PSTR( ")" ) );
        }
        return -1;
    }

    if( g_uZMatrixMax[Y_AXIS] >= COMPENSATION_MATRIX_MAX_Y || g_uZMatrixMax[Y_AXIS] < 2 )
    {
        if( Printer::debugErrors() )
        {
            Com::printF( PSTR( "prepareCompensationMatrix(): invalid y dimension detected: " ), g_uZMatrixMax[Y_AXIS] );
            Com::printF( PSTR( " (max expected: " ), COMPENSATION_MATRIX_MAX_Y -1 );
            Com::printFLN( PSTR( ")" ) );
        }
        return -1;
    }

    if( g_ZCompensationMatrix[2][0] > 0 )
    {
        // we have to fill x[1] with the values of x[2]
/*      if( Printer::debugInfo() )
        {
            Com::printFLN( PSTR( "prepareCompensationMatrix(): x[2] > 0" ) );
        }
*/      g_ZCompensationMatrix[1][0] = 0;
        for( y=1; y<=g_uZMatrixMax[Y_AXIS]; y++ )
        {
            g_ZCompensationMatrix[1][y] = g_ZCompensationMatrix[2][y];
        }
    }
    else
    {
        // we have to shift all x columns one index
/*      if( Printer::debugInfo() )
        {
            Com::printFLN( PSTR( "prepareCompensationMatrix(): x[2] = 0" ) );
        }
*/      for( x=1; x<g_uZMatrixMax[X_AXIS]; x++ )
        {
            for( y=0; y<=g_uZMatrixMax[Y_AXIS]; y++ )
            {
                g_ZCompensationMatrix[x][y] = g_ZCompensationMatrix[x+1][y];
            }
        }

        // we have one x column less now
        g_uZMatrixMax[X_AXIS] --;
    }

    if( g_ZCompensationMatrix[g_uZMatrixMax[X_AXIS]][0] < (short)Printer::lengthMM[X_AXIS] )
    {
        // we have to fill x[g_uZMatrixMax[X_AXIS]] with the values of x[g_uZMatrixMax[X_AXIS]-1]
/*      if( Printer::debugInfo() )
        {
            Com::printFLN( PSTR( "prepareCompensationMatrix(): x[g_uZMatrixMax[X_AXIS]-1] < Printer::lengthMM[X_AXIS]" ) );
        }
*/      g_ZCompensationMatrix[g_uZMatrixMax[X_AXIS]+1][0] = short(Printer::lengthMM[X_AXIS]);
        for( y=1; y<=g_uZMatrixMax[Y_AXIS]; y++ )
        {
            g_ZCompensationMatrix[g_uZMatrixMax[X_AXIS]+1][y] = g_ZCompensationMatrix[g_uZMatrixMax[X_AXIS]][y];
        }

        // we have one x column more now
        g_uZMatrixMax[X_AXIS] ++;
    }
    else
    {
        // there is nothing else to do here
/*      if( Printer::debugInfo() )
        {
            Com::printFLN( PSTR( "prepareCompensationMatrix(): x[g_uZMatrixMax[X_AXIS]-1] = Printer::lengthMM[X_AXIS]" ) );
        }
*/  }

    if( g_ZCompensationMatrix[0][2] > 0 )
    {
        // we have to fill y[1] with the values of y[2]
/*      if( Printer::debugInfo() )
        {
            Com::printFLN( PSTR( "prepareCompensationMatrix(): y[2] > 0" ) );
        }
*/      g_ZCompensationMatrix[0][1] = 0;
        for( x=1; x<=g_uZMatrixMax[X_AXIS]; x++ )
        {
            g_ZCompensationMatrix[x][1] = g_ZCompensationMatrix[x][2];
        }
    }
    else
    {
        // we have to shift all y columns one index
/*      if( Printer::debugInfo() )
        {
            Com::printFLN( PSTR( "prepareCompensationMatrix(): y[2] = 0" ) );
        }
*/      for( x=0; x<=g_uZMatrixMax[X_AXIS]; x++ )
        {
            for( y=1; y<g_uZMatrixMax[Y_AXIS]; y++ )
            {
                g_ZCompensationMatrix[x][y] = g_ZCompensationMatrix[x][y+1];
            }
        }

        // we have one y column less now
        g_uZMatrixMax[Y_AXIS] --;
    }

    if( g_ZCompensationMatrix[0][g_uZMatrixMax[Y_AXIS]] < short(Printer::lengthMM[Y_AXIS]) )
    {
        // we have to fill y[g_uZMatrixMax[Y_AXIS]] with the values of y[g_uZMatrixMax[Y_AXIS]-1]
/*      if( Printer::debugInfo() )
        {
            Com::printFLN( PSTR( "prepareCompensationMatrix(): y[g_uZMatrixMax[Y_AXIS]-1] < Printer::lengthMM[Y_AXIS]" ) );
        }
*/      g_ZCompensationMatrix[0][g_uZMatrixMax[Y_AXIS]+1] = short(Printer::lengthMM[Y_AXIS]);
        for( x=1; x<=g_uZMatrixMax[X_AXIS]; x++ )
        {
            g_ZCompensationMatrix[x][g_uZMatrixMax[Y_AXIS]+1] = g_ZCompensationMatrix[x][g_uZMatrixMax[Y_AXIS]];
        }

        // we have one y column more now
        g_uZMatrixMax[Y_AXIS] ++;
    }
    else
    {
        // there is nothing else to do here
/*      if( Printer::debugInfo() )
        {
            Com::printFLN( PSTR( "prepareCompensationMatrix(): y[g_uZMatrixMax[Y_AXIS]-1] = Printer::lengthMM[Y_AXIS]" ) );
        }
*/  }

    // determine the minimal distance between extruder and heat bed
    determineCompensationOffsetZ();

    return 0;

} // prepareCompensationMatrix


char determineCompensationOffsetZ( void )
{
#if FEATURE_HEAT_BED_Z_COMPENSATION
    short   x;
    short   y;
    short   uMax = -32768;


    for( x=1; x<=g_uZMatrixMax[X_AXIS]; x++ )
    {
        for( y=1; y<=g_uZMatrixMax[Y_AXIS]; y++ )
        {
            if( g_ZCompensationMatrix[x][y] > uMax )
            {
                uMax = g_ZCompensationMatrix[x][y];
            }
        }
    }

    g_offsetZCompensationSteps = uMax;
#endif // FEATURE_HEAT_BED_Z_COMPENSATION

    return 0;

} // determineCompensationOffsetZ


char adjustCompensationMatrix( short nZ )
{
    short   x;
    short   y;
    short   nOffset = getHeatBedOffset();
    short   deltaZ  = nZ - nOffset;
    

    if( Printer::debugInfo() )
    {
        Com::printF( PSTR( "adjustCompensationMatrix(): nOffset = " ), (int)nOffset );
        Com::printF( PSTR( ", nZ = " ), (int)nZ );
        Com::printFLN( PSTR( ", deltaZ = " ), (int)deltaZ );
    }

    for( x=1; x<=g_uZMatrixMax[X_AXIS]; x++ )
    {
        for( y=1; y<=g_uZMatrixMax[Y_AXIS]; y++ )
        {
            g_ZCompensationMatrix[x][y] += deltaZ;
        }
    }

    // determine the minimal distance between extruder and heat bed
    determineCompensationOffsetZ();

    return 0;

} // adjustCompensationMatrix


char saveCompensationMatrix( unsigned int uAddress )
{
    unsigned int    uOffset;
    short           uTemp;
    short           uMax = -32000;
    short           x;
    short           y;


    if( g_ZCompensationMatrix[0][0] && g_uZMatrixMax[X_AXIS] && g_uZMatrixMax[Y_AXIS] ) //valid in RAM means writing ok
    {
        // we have valid compensation values
        if( Printer::debugInfo() )
        {
            Com::printFLN( PSTR( "saveCompensationMatrix(): valid data detected" ) );
        }

        // write the current header version
        writeWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, EEPROM_OFFSET_HEADER_FORMAT, EEPROM_FORMAT );
        
        // write the current sector version
        writeWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, uAddress + EEPROM_OFFSET_SECTOR_FORMAT, EEPROM_FORMAT );
        
        // write the current x dimension
        writeWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, uAddress + EEPROM_OFFSET_DIMENSION_X, g_uZMatrixMax[X_AXIS] );

        // write the current y dimension
        writeWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, uAddress + EEPROM_OFFSET_DIMENSION_Y, g_uZMatrixMax[Y_AXIS] );

        // write the current micro steps
        writeWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, uAddress + EEPROM_OFFSET_MICRO_STEPS, RF_MICRO_STEPS );

        // write some information about the scanning area - note that this information is read only in case of work part z-compensation matrixes later
        writeWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, uAddress + EEPROM_OFFSET_X_START_MM, (short)(g_nScanXStartSteps / Printer::axisStepsPerMM[X_AXIS]) );
        writeWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, uAddress + EEPROM_OFFSET_Y_START_MM, (short)(g_nScanYStartSteps / Printer::axisStepsPerMM[Y_AXIS]) );
        writeWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, uAddress + EEPROM_OFFSET_X_STEP_MM, (short)g_nScanXStepSizeMm );
        writeWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, uAddress + EEPROM_OFFSET_Y_STEP_MM, (short)g_nScanYStepSizeMm );
        writeWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, uAddress + EEPROM_OFFSET_X_END_MM, (short)(g_nScanXMaxPositionSteps / Printer::axisStepsPerMM[X_AXIS]) );
        writeWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, uAddress + EEPROM_OFFSET_Y_END_MM, (short)(g_nScanYMaxPositionSteps / Printer::axisStepsPerMM[Y_AXIS]) );

        uOffset = uAddress + EEPROM_OFFSET_MATRIX_START;
        for( x=0; x<=g_uZMatrixMax[X_AXIS]; x++ )
        {
            for( y=0; y<=g_uZMatrixMax[Y_AXIS]; y++ )
            {
                uTemp = g_ZCompensationMatrix[x][y];
                writeWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, uOffset, uTemp );
                uOffset += 2;

                if( x>0 && y>0 )
                {
                    // the first column and row is used for version and position information
                    if( uTemp > uMax )  uMax = uTemp;
                }
            }
            GCode::keepAlive( Processing );
        }
    }
    else
    {
        // we do not have valid heat bed compensation values - clear the EEPROM data
        if( Printer::debugErrors() )
        {
            Com::printF( PSTR( "saveCompensationMatrix(): invalid data detected ( " ), g_ZCompensationMatrix[0][0] );
            Com::printF( PSTR( " / " ), g_uZMatrixMax[X_AXIS] );
            Com::printF( PSTR( " / " ), g_uZMatrixMax[Y_AXIS] );
            Com::printFLN( PSTR( " )" ) );
        }

        // write the current version
        writeWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, EEPROM_OFFSET_HEADER_FORMAT, EEPROM_FORMAT );
        
        // write the current sector version
        writeWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, uAddress + EEPROM_OFFSET_SECTOR_FORMAT, 0 );
        
        // write the current x dimension
        writeWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, uAddress + EEPROM_OFFSET_DIMENSION_X, 0 );

        // write the current y dimension
        writeWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, uAddress + EEPROM_OFFSET_DIMENSION_Y, 0 );

        // write the current micro steps
        writeWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, uAddress + EEPROM_OFFSET_MICRO_STEPS, 0 );

        // write some information about the scanning area - note that this information is read only in case of work part z-compensation matrixes later
        writeWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, uAddress + EEPROM_OFFSET_X_START_MM, 0 );
        writeWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, uAddress + EEPROM_OFFSET_Y_START_MM, 0 );
        writeWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, uAddress + EEPROM_OFFSET_X_STEP_MM, 0 );
        writeWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, uAddress + EEPROM_OFFSET_Y_STEP_MM, 0 );
        writeWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, uAddress + EEPROM_OFFSET_X_END_MM, 0 );
        writeWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, uAddress + EEPROM_OFFSET_Y_END_MM, 0 );

        uOffset = uAddress + EEPROM_OFFSET_MATRIX_START;
        for( x=0; x<COMPENSATION_MATRIX_MAX_X; x++ )
        {
            for( y=0; y<COMPENSATION_MATRIX_MAX_Y; y++ )
            {
                writeWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, uOffset, 0 );
                uOffset += 2;
            }
            GCode::keepAlive( Processing );
        }
    }

#if FEATURE_HEAT_BED_Z_COMPENSATION
    g_offsetZCompensationSteps = uMax;
#endif // FEATURE_HEAT_BED_Z_COMPENSATION
    g_ZMatrixChangedInRam = 0;

    return 0;

} // saveCompensationMatrix


char loadCompensationMatrix( unsigned int uAddress )
{
    unsigned short  uTemp;
    unsigned short  uDimensionX;
    unsigned short  uDimensionY;
    unsigned short  uMicroSteps;
    unsigned int    uOffset;
    short           nTemp;
    short           uMax = -32000;
    short           x;
    short           y;
    float           fMicroStepCorrection;


    // check the stored header format
    uTemp = readWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, EEPROM_OFFSET_HEADER_FORMAT );

    if( uTemp != EEPROM_FORMAT )
    {
        if( Printer::debugErrors() )
        {
            Com::printF( PSTR( "loadCompensationMatrix(): invalid header format detected: " ), (int)uTemp );
            Com::printF( PSTR( " (expected: " ), EEPROM_FORMAT );
            Com::printFLN( PSTR( ")" ) );
        }
        return -1;
    }

    if( !uAddress )
    {
        // we have to detect the to-be-loaded compensation matrix automatically
#if FEATURE_MILLING_MODE
        if( Printer::operatingMode == OPERATING_MODE_PRINT )
        {
#if FEATURE_HEAT_BED_Z_COMPENSATION
            // load the currently active heat bed compensation matrix
            uTemp = readWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, EEPROM_OFFSET_ACTIVE_HEAT_BED_Z_MATRIX );

            if( uTemp < 1 || uTemp > EEPROM_MAX_HEAT_BED_SECTORS )
            {
                if( Printer::debugErrors() )
                {
                    Com::printFLN( PSTR( "loadCompensationMatrix(): invalid active heat bed z matrix detected: " ), (int)uTemp );
                }
                return -1;
            }

            g_nActiveHeatBed    = (char)uTemp;
            uAddress            = (unsigned int)(EEPROM_SECTOR_SIZE * uTemp);

            if( Printer::debugErrors() )
            {
                Com::printFLN( PSTR( "loadCompensationMatrix(): active heat bed z matrix: " ), (int)g_nActiveHeatBed );
            }
#else
            // we do not support the heat bed compensation
            return -1;
#endif // FEATURE_HEAT_BED_Z_COMPENSATION
        }
        else
        {
#if FEATURE_WORK_PART_Z_COMPENSATION
            // load the currently active work part compensation matrix
            uTemp = readWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, EEPROM_OFFSET_ACTIVE_WORK_PART_Z_MATRIX );

            if( uTemp < 1 || uTemp > EEPROM_MAX_WORK_PART_SECTORS )
            {
                if( Printer::debugErrors() )
                {
                    Com::printFLN( PSTR( "loadCompensationMatrix(): invalid active work part detected: " ), (int)uTemp );
                }
                return -1;
            }

            g_nActiveWorkPart = (char)uTemp;
            uAddress          = (EEPROM_SECTOR_SIZE *9) + (unsigned int)(EEPROM_SECTOR_SIZE * uTemp);
#else
            // we do not support the work part compensation
            return -1;
#endif // FEATURE_WORK_PART_Z_COMPENSATION
        }
#else
#if FEATURE_HEAT_BED_Z_COMPENSATION
        // load the heat bed compensation matrix
        uAddress = EEPROM_SECTOR_SIZE;
#else
        // we do not support the heat bed compensation
        return -1;
#endif // FEATURE_HEAT_BED_Z_COMPENSATION
#endif // FEATURE_MILLING_MODE
    }

#if FEATURE_WORK_PART_Z_COMPENSATION && FEATURE_MILLING_MODE
    if( Printer::operatingMode == OPERATING_MODE_MILL )
    {
        if( Printer::debugErrors() )
        {
            Com::printFLN( PSTR( "loadCompensationMatrix(): active work part: " ), (int)g_nActiveWorkPart );
        }
    }
#endif // FEATURE_WORK_PART_Z_COMPENSATION && FEATURE_MILLING_MODE

    // check the stored sector format
    uTemp = readWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, uAddress + EEPROM_OFFSET_SECTOR_FORMAT );

    if( uTemp != EEPROM_FORMAT )
    {
        if( Printer::debugErrors() )
        {
            Com::printF( PSTR( "loadCompensationMatrix(): invalid sector format detected: " ), (int)uTemp );
            Com::printF( PSTR( " (expected: " ), EEPROM_FORMAT );
            Com::printFLN( PSTR( ")" ) );
        }
        return -1;
    }

    // check the stored x dimension
    uDimensionX = readWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, uAddress + EEPROM_OFFSET_DIMENSION_X );

    if( uDimensionX > COMPENSATION_MATRIX_MAX_X )
    {
        if( Printer::debugErrors() )
        {
            Com::printF( PSTR( "loadCompensationMatrix(): invalid x dimension detected: " ), (int)uDimensionX );
            Com::printF( PSTR( " (max expected: " ), COMPENSATION_MATRIX_MAX_X );
            Com::printFLN( PSTR( ")" ) );
        }
        return -1;
    }

    // check the stored y dimension
    uDimensionY = readWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, uAddress + EEPROM_OFFSET_DIMENSION_Y );

    if( uDimensionY > COMPENSATION_MATRIX_MAX_Y )
    {
        if( Printer::debugErrors() )
        {
            Com::printF( PSTR( "loadCompensationMatrix(): invalid y dimension detected: " ), (int)uDimensionY );
            Com::printF( PSTR( " (max expected: " ), COMPENSATION_MATRIX_MAX_Y );
            Com::printFLN( PSTR( ")" ) );
        }
        return -1;
    }

    g_uZMatrixMax[X_AXIS] = (unsigned char)uDimensionX;
    g_uZMatrixMax[Y_AXIS] = (unsigned char)uDimensionY;

    // check the stored microsteps
    uMicroSteps = readWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, uAddress + EEPROM_OFFSET_MICRO_STEPS );

    if( uMicroSteps == RF_MICRO_STEPS )
    {
        // the current z-compensation matrix has been determined with the current micro step setting, there is nothing to recalculate
        fMicroStepCorrection = 1.0;
    }
    else if( uMicroSteps > RF_MICRO_STEPS )
    {
        // the current z-compensation matrix has been determined with a higher than the current micro step setting, we must divide all z-correction values
        fMicroStepCorrection = (float)RF_MICRO_STEPS / (float)uMicroSteps;

        Com::printF( PSTR( "loadCompensationMatrix(): micro step correction = " ), fMicroStepCorrection );
        Com::printF( PSTR( " (stored = " ), (int)uMicroSteps );
        Com::printF( PSTR( ", current = )" ), (int)RF_MICRO_STEPS );
        Com::printFLN( PSTR( ")" ) );
    }
    else
    {
        // the current z-compensation matrix has been determined with a smaller than the current micro step setting, we must multiply all z-correction values
        fMicroStepCorrection = (float)RF_MICRO_STEPS / (float)uMicroSteps;
    }

    if( uAddress > (EEPROM_SECTOR_SIZE *9) )
    {
        // in case we are reading a work part z-compensation matrix, we have to read out some information about the scanning area
        g_nScanXStartSteps       = (long)readWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, uAddress + EEPROM_OFFSET_X_START_MM ) * Printer::axisStepsPerMM[X_AXIS];
        g_nScanYStartSteps       = (long)readWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, uAddress + EEPROM_OFFSET_Y_START_MM ) * Printer::axisStepsPerMM[Y_AXIS];
        g_nScanXStepSizeMm       = (long)readWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, uAddress + EEPROM_OFFSET_X_STEP_MM );
        g_nScanYStepSizeMm       = (long)readWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, uAddress + EEPROM_OFFSET_Y_STEP_MM );
        g_nScanXMaxPositionSteps = (long)readWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, uAddress + EEPROM_OFFSET_X_END_MM ) * Printer::axisStepsPerMM[X_AXIS];
        g_nScanYMaxPositionSteps = (long)readWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, uAddress + EEPROM_OFFSET_Y_END_MM ) * Printer::axisStepsPerMM[Y_AXIS];
        g_nScanXStepSizeSteps    = g_nScanXStepSizeMm * Printer::axisStepsPerMM[X_AXIS];
        g_nScanYStepSizeSteps    = g_nScanYStepSizeMm * Printer::axisStepsPerMM[Y_AXIS];
    }

    // read out the actual compensation values
    uOffset = uAddress + EEPROM_OFFSET_MATRIX_START;
    for( x=0; x<=g_uZMatrixMax[X_AXIS]; x++ )
    {
        for( y=0; y<=g_uZMatrixMax[Y_AXIS]; y++ )
        {
            nTemp = readWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, uOffset );

            if( x == 0 || y == 0 )
            {
                // we must not modify our header row/column
                g_ZCompensationMatrix[x][y] = nTemp;
            }
            else
            {
                // we may have to update all z-compensation values
                g_ZCompensationMatrix[x][y] = (short)((float)nTemp * fMicroStepCorrection);
            }
            uOffset += 2;

            if( x>0 && y>0 )
            {
                // the first column and row is used for version and position information
                if( nTemp > uMax )  uMax = nTemp;
            }
        }
        GCode::keepAlive( Processing );
    }

#if FEATURE_HEAT_BED_Z_COMPENSATION
    g_offsetZCompensationSteps = uMax;
#endif // FEATURE_HEAT_BED_Z_COMPENSATION

    g_ZMatrixChangedInRam = 0; //Nibbels: Marker, dass die Matrix gespeichert werden kann oder eben nicht, weils unverändert keinen Sinn macht.

    resetZCompensation();
    return 0;

} // loadCompensationMatrix


void clearCompensationMatrix( unsigned int uAddress )
{
    // clear all fields of the compensation matrix
    initCompensationMatrix();

    // store the cleared compensation matrix to the EEPROM
    saveCompensationMatrix( uAddress );

    if( Printer::debugInfo() )
    {
        Com::printFLN( PSTR( "clearCompensationMatrix(): the compensation matrix has been cleared" ) );
    }
    return;

} // clearCompensationMatrix


void outputPressureMatrix( void )
{
    if( Printer::debugInfo() )
    {
#if DEBUG_REMEMBER_SCAN_PRESSURE
        short   i;
        short   j;


        Com::printFLN( PSTR( "Pressure matrix:" ) );
        Com::printFLN( PSTR( "front left ... front right" ) );
        Com::printFLN( PSTR( "...        ...         ..." ) );
        Com::printFLN( PSTR( "back left  ...  back right" ) );
        for( i=0; i<COMPENSATION_MATRIX_MAX_Y; i++ )
        {
            for( j=0; j<COMPENSATION_MATRIX_MAX_X; j++ )
            {
                Com::printF( Com::tSemiColon, g_ScanPressure[j][i] );
            }
            Com::printFLN( PSTR( " " ) );
        }
#endif // DEBUG_REMEMBER_SCAN_PRESSURE
    }

    return;

} // outputPressureMatrix
#endif // FEATURE_HEAT_BED_Z_COMPENSATION || FEATURE_WORK_PART_Z_COMPENSATION


char clearExternalEEPROM( void )
{
    unsigned short  i;
    unsigned short  uMax = 32768;
    unsigned short  uTemp;
    unsigned short  uLast = 0;


    if( Printer::debugInfo() )
    {
        Com::printFLN( PSTR( "clearExternalEEPROM(): erasing external memory ..." ) );
    }

    // the external EEPROM is able to store 262.144 kBit (= 32.768 kByte)
    for( i=0; i<uMax; i++ )
    {
        writeByte24C256( I2C_ADDRESS_EXTERNAL_EEPROM, i, 0 );
        Commands::checkForPeriodicalActions();

        if( Printer::debugInfo() )
        {
            uTemp = i / 100;
            if( uTemp != uLast )
            {
                Com::printF( PSTR( "clearExternalEEPROM(): " ), (int)i );
                Com::printFLN( PSTR( " / " ), (long)uMax );
                uLast = uTemp;
            }
        }
        GCode::keepAlive( Processing );
    }

    if( Printer::debugInfo() )
    {
        Com::printFLN( PSTR( "clearExternalEEPROM(): erasing complete" ) );
    }
    return 0;

} // clearExternalEEPROM


void writeByte24C256( int addressI2C, unsigned int addressEEPROM, unsigned char data )
{
    HAL::delayMilliseconds( EEPROM_DELAY );
    Wire.beginTransmission( addressI2C );
    Wire.write( int(addressEEPROM >> 8));       // MSB
    Wire.write( int(addressEEPROM & 0xFF));     // LSB
    Wire.write( data );
    Wire.endTransmission();
    return;
    
} // writeByte24C256


void writeWord24C256( int addressI2C, unsigned int addressEEPROM, unsigned short data )
{
    unsigned short  Temp;


    Temp = byte(data >> 8);
    writeByte24C256( addressI2C, addressEEPROM, Temp );
    Temp = byte(data & 0x00FF);
    writeByte24C256( addressI2C, addressEEPROM+1, Temp );
    return;

} // writeWord24C256


unsigned char readByte24C256( int addressI2C, unsigned int addressEEPROM )
{
    HAL::delayMilliseconds( EEPROM_DELAY );
    Wire.beginTransmission( addressI2C );
    Wire.write( int(addressEEPROM >> 8));       // MSB
    Wire.write( int(addressEEPROM & 0xFF));     // LSB
    Wire.endTransmission();
    Wire.requestFrom( addressI2C, 1 );
    
    return Wire.read();
    
} // readByte24C256


unsigned short readWord24C256( int addressI2C, unsigned int addressEEPROM )
{
    unsigned short  data;
    byte            Temp;


    Temp = readByte24C256( addressI2C, addressEEPROM );
    data = Temp;
    data = data << 8;
    Temp = readByte24C256( addressI2C, addressEEPROM+1 );

    return data + Temp;

} // readWord24C256


void doZCompensation( void )
{
#if FEATURE_MILLING_MODE

    if( Printer::operatingMode == OPERATING_MODE_MILL )
    {
#if FEATURE_WORK_PART_Z_COMPENSATION
        doWorkPartZCompensation();
#endif // FEATURE_WORK_PART_Z_COMPENSATION
    }
    else

#endif // FEATURE_MILLING_MODE

    {
#if FEATURE_HEAT_BED_Z_COMPENSATION
        doHeatBedZCompensation();
#endif // FEATURE_HEAT_BED_Z_COMPENSATION
    }

} // doZCompensation


void loopRF( void ) //wird so aufgerufen, dass es ein ~100ms takt sein sollte.
{
    static char     nEntered = 0;
    if( nEntered ) return; // do not enter more than once
    nEntered ++;

    unsigned long   uTime = HAL::timeInMilliseconds();
    short           nPressure;

    if( g_uStartOfIdle )
    {
        if( (uTime - g_uStartOfIdle) > MINIMAL_IDLE_TIME ) //500ms nach config
        {
            // show that we are idle for a while already
            showIdle();
            g_uStartOfIdle  = 0;
            g_nPrinterReady = 1;
            Printer::setPrinting(false);
        }
    }

    if( PrintLine::linesCount > 5 )
    {
        // this check shall be done only during the printing (for example, it shall not be done in case filament is extruded manually)
        Printer::setPrinting(true);
    }

#if FEATURE_CASE_FAN && !CASE_FAN_ALWAYS_ON
    if( Printer::prepareFanOff )
    {
        if( (uTime - Printer::prepareFanOff) > Printer::fanOffDelay ) //60s wäre standard nach config
        {
            // it is time to turn the case fan off
            Printer::prepareFanOff = 0;
            if( !Printer::ignoreFanOn ) WRITE( CASE_FAN_PIN, 0 );
        }
    }
#endif // FEATURE_CASE_FAN && !CASE_FAN_ALWAYS_ON

#if FEATURE_MILLING_MODE

    if( Printer::operatingMode == OPERATING_MODE_PRINT )
    {        
#if FEATURE_HEAT_BED_Z_COMPENSATION
        if( g_nHeatBedScanStatus )
        {
            scanHeatBed();
        }
        if( g_ZOSScanStatus )
        {
            searchZOScan();
        }
#endif // FEATURE_HEAT_BED_Z_COMPENSATION
    }
    else
    {
#if FEATURE_WORK_PART_Z_COMPENSATION
        if( g_nWorkPartScanStatus )
        {
            scanWorkPart();
        }
#endif // FEATURE_WORK_PART_Z_COMPENSATION
    }

#else

#if FEATURE_HEAT_BED_Z_COMPENSATION
    if( g_nHeatBedScanStatus )
    {
        scanHeatBed();
    }
    if( g_ZOSScanStatus )
    {
        searchZOScan();
    }
#endif // FEATURE_HEAT_BED_Z_COMPENSATION

#endif // FEATURE_MILLING_MODE

#if FEATURE_FIND_Z_ORIGIN
    if( g_nFindZOriginStatus )
    {
        findZOrigin();
    }
#endif // FEATURE_FIND_Z_ORIGIN

#if FEATURE_PAUSE_PRINTING
    if( g_pauseMode != PAUSE_MODE_NONE )
    {
        // show that we are paused
        GCode::keepAlive( Paused ); //keepAlive limitiert seine Ausführzeit selbst: alle 2s, darunter wird geskipped.
    }

    if( g_uPauseTime )
    {
        if( !g_pauseBeepDone )
        {
            BEEP_PAUSE
            g_pauseBeepDone = 1;
        }

        if( g_pauseStatus == PAUSE_STATUS_PAUSED ) //and absolutly not PAUSE_STATUS_HEATING
        {
#if EXTRUDER_CURRENT_PAUSE_DELAY
            if( (uTime - g_uPauseTime) > EXTRUDER_CURRENT_PAUSE_DELAY ) //das sind alle 5s 
            {
                char    nProcessExtruder = 0;
#if FEATURE_MILLING_MODE
                if( Printer::operatingMode == OPERATING_MODE_PRINT )
                {
                    // process the extruder only in case we are in mode "print"
                    nProcessExtruder = 1;
                }
#else
                nProcessExtruder = 1;
#endif // FEATURE_MILLING_MODE

                if( nProcessExtruder )
                {
                    // we have paused a few moments ago - reduce the current of the extruder motor in order to avoid unwanted heating of the filament for use cases where the printing is paused for several minutes
#if NUM_EXTRUDER > 0
                    for(uint8_t i = 0; i < NUM_EXTRUDER; i++) {
#if EXTRUDER_CURRENT_PAUSE_DELAY
                        setExtruderCurrent( i, EXTRUDER_CURRENT_PAUSED );
#endif //EXTRUDER_CURRENT_PAUSE_DELAY
                        if(!extruder[i].paused){ //temperaturminimierung in paused ablegen. maximal -255 °C als Zahl 255. Config aktuell nur über RFx000.h bei PAUSE_COOLDOWN
                            extruder[i].paused = (PAUSE_COOLDOWN > 255) ? 255 : ( ( extruder[i].tempControl.targetTemperatureC > PAUSE_COOLDOWN ) ? PAUSE_COOLDOWN : extruder[i].tempControl.targetTemperatureC );
                            extruder[i].tempControl.targetTemperatureC -= (float)extruder[i].paused;
                            //laden bei continuePrint(), indem paused addiert und gewartet wird.
                        }
                    }
#endif
                }
                g_uPauseTime = 0;
            }
#endif // EXTRUDER_CURRENT_PAUSE_DELAY
        }
        else
        {
            // we are not paused any more
            g_uPauseTime = 0;
        }
    }
#endif // FEATURE_PAUSE_PRINTING

/* Change: 09_06_2017 Never read straingauge twice in a row: test if this helps avoiding my watchdog problem
           Thatwhy I bring the statics up and preread the value for both FEATURE_EMERGENCY_PAUSE and FEATURE_EMERGENCY_STOP_ALL */
/* Update: 19_06_2017 This is really nice and clean but it has not been the problem. */
#if FEATURE_EMERGENCY_PAUSE || FEATURE_EMERGENCY_STOP_ALL
    bool i_need_strain_value = 0;
#endif //FEATURE_EMERGENCY_PAUSE || FEATURE_EMERGENCY_STOP_ALL

#if FEATURE_SENSIBLE_PRESSURE
    static unsigned long   nSensiblePressureTime   = 0;
    static long            nSensiblePressureSum    = 0;
    static char            nSensiblePressureChecks = 0;
    if( (uTime - nSensiblePressureTime) > SENSIBLE_PRESSURE_INTERVAL ) //jede 100ms -> das macht hier drin wenig sinn. 
    {
        i_need_strain_value = 1;
    }
#endif // FEATURE_SENSIBLE_PRESSURE

#if FEATURE_EMERGENCY_PAUSE
    static unsigned long   uLastPressureTime         = 0;
    static long            nPressureSum              = 0;
    static char            nPressureChecks           = 0;
    if( (uTime - uLastPressureTime) > EMERGENCY_PAUSE_INTERVAL ) //jede 100ms -> das macht hier drin wenig sinn.
    {
        i_need_strain_value = 1;
    }
#endif // FEATURE_EMERGENCY_PAUSE

#if FEATURE_EMERGENCY_STOP_ALL
    static unsigned long   uLastZPressureTime        = 0;
    static long            nZPressureSum             = 0;
    static char            nZPressureChecks          = 0;
    if( (uTime - uLastZPressureTime) > EMERGENCY_STOP_INTERVAL ) //jede 10ms -> das macht hier drin überhaupt garkeinen sinn. : kurz, das heißt "absolut immer" jede 100ms.
    {
        i_need_strain_value = 1;
    }
#endif // FEATURE_EMERGENCY_STOP_ALL

#if FEATURE_EMERGENCY_PAUSE || FEATURE_EMERGENCY_STOP_ALL || FEATURE_SENSIBLE_PRESSURE
    static short pressure = 0;
    if( i_need_strain_value ){
        pressure = readStrainGauge( ACTIVE_STRAIN_GAUGE );
    }
#endif //FEATURE_EMERGENCY_PAUSE || FEATURE_EMERGENCY_STOP_ALL || FEATURE_SENSIBLE_PRESSURE

#if FEATURE_SENSIBLE_PRESSURE
    //ohne Z-Kompensation kein SensiblePressure!
    if( g_nSensiblePressureDigits && !Printer::doHeatBedZCompensation ){

        g_nSensiblePressureDigits = 0;
        nSensiblePressureSum = 0;  //close down counters if function deactivated.
        nSensiblePressureChecks = 0;
        if(g_nSensiblePressureOffset){
            Com::printF( PSTR( "SensiblePressure(): offset reverted from " ), g_nSensiblePressureOffset );
            Com::printFLN( PSTR( " [um] to 0 [um] " ) );
            //OFFSET-RESET nur in SAFE-Zustand!! -> Wenn z-Compensation sowieso deaktiviert. oder per G-Code.
            g_nSensiblePressureOffset = 0;
            g_staticZSteps = ((Printer::ZOffset+g_nSensiblePressureOffset) * Printer::axisStepsPerMM[Z_AXIS]) / 1000;
        }
        Com::printFLN( PSTR( "SensiblePressure(): is now disabled because of no z-compensation. " ), g_nSensiblePressureOffset );

    }

    if( (uTime - nSensiblePressureTime) > SENSIBLE_PRESSURE_INTERVAL ) 
    {
        nSensiblePressureTime = uTime;
        /* brief: This is for correcting too close Z at first layer // Idee Wessix, coded by Nibbels  */

        if(g_nSensiblePressureDigits && Printer::doHeatBedZCompensation){ //activate feature with G-Code.
            /* 
            Still testing: Gesucht ist ein Limit des der Abstands zum Druckbett, also die jeweils richtige Layerbegrenzung::

            #das Extruder::current->zOffset ist negativ und in Steps. Ist ein Extruder weiter unten, per T1-Offset, muss das bedacht werden.
            #Die Höhe über Grund, kompensiert sollte unterhalb g_maxZCompensationSteps sein.
            #queuePositionCurrentSteps = Achsenziel + Achsenoffset, aber g_minZCompensationSteps/g_maxZCompensationSteps kennen das Achsenoffset nicht ohne Hilfe: Das gehört hier her, wenn man die Layerhöhe abgleichen will.
            */
            if( Printer::queuePositionCurrentSteps[Z_AXIS] <= g_minZCompensationSteps - Extruder::current->zOffset ) 
            {
                g_nSensiblePressure1stMarke = 1; //marker für display: wir sind in regelhöhe
                //wenn durch Gcode gefüllt, prüfe, ob Z-Korrektur (weg vom Bett) notwendig ist, in erstem Layer.
                nSensiblePressureSum += pressure;
                nSensiblePressureChecks += 1;
                //jede 1 sekunden, bzw 0.5sekunden. => 100ms * 10 ::
                if( nSensiblePressureChecks >= 10 ){ 

                    nPressure = (short)(nSensiblePressureSum / nSensiblePressureChecks);

                    static short g_nSensibleLastPressure = 0;
                    //half interval, remember old values 50% -> gibt etwas value-trägheit in den regler -> aber verursacht doppelte schrittgeschwindigkeit bei 0,5                         
                    nSensiblePressureSum *= 0.5; 
                    nSensiblePressureChecks *= 0.5;

                    //größer, kleiner, falls Messzellen falschrum eingebaut. -> abs() // später einfacher zu verrechnen.
                    nPressure = abs(nPressure);
                    if( (nPressure > g_nSensiblePressureDigits) )
                    {
                        /* here the action is defined if digits are > g_nSensiblePressureDigits while printing */

                        /*
                        Z-OFFSETs :: Mir sind aktuell folgende Offsets in Z bekannt:
                            long Printer::ZOffset;                                                          == OFFSET IN MENÜ / M3006 in [um]  
                            g_staticZSteps = (Printer::ZOffset * Printer::axisStepsPerMM[Z_AXIS]) / 1000;   == OFFSET-STEPS errechnet aus Printer::ZOffset -> Und NEU zusätzlich aus SenseOffset
                            g_offsetZCompensationSteps                                                      == OFFSET AUS zMATRIX (Minimaler Bett-Hotend-Abstand)
                            nNeededZCompensation                                                            == In der Z-Compensations-Funktion: Das ist der Matrixanteil an exaktem Punkt x/y
                            
                            Strategie: Wir haben unser eigenes Sensible-Offset "g_nSensiblePressureOffset" mit Beschränkung und rechnen g_staticZSteps neu aus.
                            g_nSensiblePressureOffset
                        */                              

                        //wie stark reagieren?
                        //PID -Regler wollen wir uns aber sparen...
                        unsigned char step = 1; //0..255!! standardincrement
                        float inc = 0.0;
                        //mehr offsetincrement je höher digits oberhalb limit. linear
                        if(nPressure > g_nSensiblePressureDigits*1.1){
                            //je größer der Fehler, desto beschränkt größer der step
                            inc =   (float)nPressure / (float)g_nSensiblePressureDigits - 1; //0.1 ... riesig
                            if(inc > 2.0) inc = 2.0;
                            inc *= 12; // 1,2..24,0
                            step += (short)inc;
                        }

                        if(g_nSensibleLastPressure - nPressure > 0){
                            //digits sinken gerade -> egal wie hoch, regelspeed raus!
                            step = 1;
                        }else{
                            //digits steigen gerade -> je höher mein aktuelles zusätzliches offset, desto weniger regelspeed.
                            inc = (float)g_nSensiblePressureOffset / (float)g_nSensiblePressureOffsetMax; //0.25 .. 1
                            if( inc >= 0.25 ) { 
                                step = step - (short)(inc * (step - 1)); //über der viertel offsetstrecke, das zusätzliche offset stark ausbremsen.
                            }
                            if( inc >= 0.33){
                                if(nPressure < g_nSensiblePressureDigits*1.1) step = 0; //wenn schon etwas offset da und die kräfte nur um +10% rumpendeln, dann nicht weiter erhöhen.
                            }
                        }       

                        if (step > 0) { 
                            //um == mikrometer -> Offsetbereich beschränkt auf 0..0,10mm
                            if(g_nSensiblePressureOffset+step <= (long)g_nSensiblePressureOffsetMax){
                                g_nSensiblePressureOffset += step;
                            }else{
                                g_nSensiblePressureOffset = (long)g_nSensiblePressureOffsetMax;
                            }
                            g_staticZSteps = ((Printer::ZOffset+g_nSensiblePressureOffset) * Printer::axisStepsPerMM[Z_AXIS]) / 1000;
                        }
                    }else{
                        //kleinere digits sind natürlich zugelassen, runtergeregelt wird nicht wegen kleinerer digits bei z.B. retracts
                    }
                    g_nSensibleLastPressure = nPressure; //save last pressure.
                }
            }else{
                g_nSensiblePressure1stMarke = 0; //marker für display: auf dieser Höhe regeln wir nichts mehr oder noch nichts.
                // if sensible not active 
                // if z-compensation not active
                if(nSensiblePressureSum > 0){
                    nSensiblePressureSum = 0;  //close down counters if function deactivated.
                    nSensiblePressureChecks = 0;
                }
                //offset muss bleiben! g_nSensiblePressureOffset != 0
            }
        }
    }
#endif // FEATURE_SENSIBLE_PRESSURE


#if FEATURE_EMERGENCY_PAUSE
    if( g_nEmergencyPauseDigitsMin || g_nEmergencyPauseDigitsMax )
    {
        if( (uTime - uLastPressureTime) > EMERGENCY_PAUSE_INTERVAL ) //jede 100ms -> das macht hier drin wenig sinn.
        {
            uLastPressureTime = uTime;

            if( g_pauseStatus == PAUSE_STATUS_NONE && g_pauseMode == PAUSE_MODE_NONE && Printer::isPrinting() )
            {
                // this check shall be done only during the printing (for example, it shall not be done in case filament is extruded manually)
                nPressureSum    += pressure;
                nPressureChecks += 1;                

                if( nPressureChecks == EMERGENCY_PAUSE_CHECKS )
                {
                    nPressure        = (short)(nPressureSum / nPressureChecks);
                    nPressureSum    = 0;
                    nPressureChecks = 0;

                    if( (nPressure < g_nEmergencyPauseDigitsMin) ||
                        (nPressure > g_nEmergencyPauseDigitsMax) )
                    {
                        // the pressure is outside the allowed range, we must perform the emergency pause
                        if( Printer::debugErrors() )
                        {
                            Com::printF( PSTR( "emergency pause: " ), nPressure );
                            Com::printFLN( PSTR( " / " ), PrintLine::linesCount );
                        }

                        showWarning( (void*)ui_text_emergency_pause );
                        pausePrint();
                        pausePrint();
                    }
                }
            }
            else
            {
                nPressureSum    = 0;
                nPressureChecks = 0;
            }
        }
    }
#endif // FEATURE_EMERGENCY_PAUSE


#if FEATURE_EMERGENCY_STOP_ALL
    if( (uTime - uLastZPressureTime) > EMERGENCY_STOP_INTERVAL ) //jede 10ms -> das macht hier drin überhaupt garkeinen sinn.
    {
        uLastZPressureTime = uTime;

        if( Printer::stepperDirection[Z_AXIS] && !Extruder::current->stepperDirection )
        {
            // this check shall be done only when there is some moving into z-direction in progress and the extruder is not doing anything
            nZPressureSum    += pressure; //readStrainGauge( ACTIVE_STRAIN_GAUGE );
            nZPressureChecks += 1;

            if( nZPressureChecks == EMERGENCY_STOP_CHECKS )
            {
                nPressure        = (short)(nZPressureSum / nZPressureChecks);

                nZPressureSum    = 0;
                nZPressureChecks = 0;
                
                if(uLastZPressureTime_IgnoreUntil < uTime){
                    if( (nPressure < g_nZEmergencyStopAllMin) ||
                        (nPressure > g_nZEmergencyStopAllMax) )
                    {
                        // the pressure is outside the allowed range, we must perform the emergency stop
                        doEmergencyStop( STOP_BECAUSE_OF_Z_BLOCK );
                    }
                }else{
                    //Manchmal ist es bescheuert, wenn man das niedrige er Limit hat. OutputObject z.B. bei einem Druck der mit kleiner Nozzle und Digits ~ 5000...9000
                    //  uLastZPressureTime_IgnoreUntil = HAL::timeInMilliseconds() + 1000; setzt diese scharfe prüfung z.b. für 1s ausser kraft und lässt mehr zu.
                    //Das ist besser als Nutzer, die das Limit über die Config voll aushebeln.
                    //31_07_17----> wurde in der config verändert und es gibt einen gcode
                    if( (nPressure < RMath::min(EMERGENCY_STOP_DIGITS_MIN,g_nZEmergencyStopAllMin) ) ||
                        (nPressure > RMath::max(EMERGENCY_STOP_DIGITS_MAX,g_nZEmergencyStopAllMax) ) )
                    {
                        // the pressure is outside the allowed range, we must perform the emergency stop
                        doEmergencyStop( STOP_BECAUSE_OF_Z_BLOCK );
                    }
                }
                
            }
        }
        else
        {
            nZPressureSum    = 0;
            nZPressureChecks = 0;
        }
    }
#endif // FEATURE_EMERGENCY_STOP_ALL

    if( g_uStopTime )
    {
        if( (uTime - g_uStopTime) > CLEAN_UP_DELAY_AFTER_STOP_PRINT ) //jede 1 sekunden  wäre standard nach config
        {
            // we have stopped the printing a few moments ago, output the object now
            if( PrintLine::linesCount )
            {
                // wait until all moves are done
                g_uStopTime = uTime;
            }
            else
            {
                // there is no printing in progress any more, do all clean-up now
                g_uStopTime = 0;

                // disable all heaters
                Extruder::setHeatedBedTemperature( 0, false );
                Extruder::setTemperatureForExtruder( 0, 0, false );

#if NUM_EXTRUDER == 2
                Extruder::setTemperatureForExtruder( 0, 1, false );
#endif // #if NUM_EXTRUDER == 2

#if FEATURE_MILLING_MODE
                if ( Printer::operatingMode == OPERATING_MODE_MILL )
                {
                    EEPROM::updatePrinterUsage();
                }
#endif // FEATURE_MILLING_MODE

                g_uBlockSDCommands = HAL::timeInMilliseconds();
            }
        }
    }

    if( g_uBlockSDCommands > 1 )
    {
        if( (uTime - g_uBlockSDCommands) > COMMAND_BLOCK_DELAY ) //jede 1 sekunden wäre standard nach config
        {
#if FEATURE_OUTPUT_FINISHED_OBJECT
            // output the object
            outputObject();
#else
            // disable all steppers
            Printer::setAllSteppersDisabled();
            Printer::disableXStepper();
            Printer::disableYStepper();
            Printer::disableZStepper();
            Extruder::disableAllExtruders();
#if FAN_PIN>-1 && FEATURE_FAN_CONTROL
            // disable the fan
            Commands::setFanSpeed(0,false);
#endif // FAN_PIN>-1 && FEATURE_FAN_CONTROL

#endif // FEATURE_OUTPUT_FINISHED_OBJECT

            cleanupXPositions();
            cleanupYPositions();
            cleanupZPositions();
            g_uBlockSDCommands = 0;
        }
    }
    
#if FEATURE_ABORT_PRINT_AFTER_TEMPERATURE_ERROR
    if( Printer::isAnyTempsensorDefect() && sd.sdmode && PrintLine::linesCount )
    {
        // we are printing from the SD card and a temperature sensor got defect - abort the current printing
        if( Printer::debugErrors() )
        {
            Com::printFLN( PSTR( "loopRF(): aborting print because of a temperature sensor defect" ) );
        }
        Com::printFLN( PSTR( "RequestStop:" ) ); //tell repetierserver to stop.
        Com::printFLN( PSTR( "// action:disconnect" ) ); //tell octoprint to disconnect
        sd.abortPrint();
    }
#endif // FEATURE_ABORT_PRINT_AFTER_TEMPERATURE_ERROR

#if FEATURE_SERVICE_INTERVAL
    if ( !g_nEnteredService )
    {
        if ( ( HAL::timeInMilliseconds() - g_nlastServiceTime ) > 5000 )
        {
            char    mode = OPERATING_MODE_PRINT;

#if FEATURE_MILLING_MODE
            mode = Printer::operatingMode;
#endif // FEATURE_MILLING_MODE

            g_uStartOfIdle = HAL::timeInMilliseconds();

            if( mode == OPERATING_MODE_PRINT )   
            {
                if( READ(5) == 0 && READ(11) == 0 && READ(42) == 0 )
                {
                    if ( g_nServiceRequest == 1 )
                    {
                        HAL::eprSetInt32(EPR_PRINTING_TIME_SERVICE,0);
                        EEPROM::updateChecksum();
                        HAL::eprSetFloat(EPR_PRINTING_DISTANCE_SERVICE,0);
                        EEPROM::updateChecksum();
                        Com::printF( PSTR( "Service Reset" ) );
                    }
                }
                else
                {
                    g_nServiceRequest = 0;
                }
            }
            else
            {
                if( READ(5) == 0 && READ(11) == 0 && READ(42) == 0 )
                {
                    if ( g_nServiceRequest == 1 )
                    {
                        HAL::eprSetInt32(EPR_MILLING_TIME_SERVICE,0);
                        EEPROM::updateChecksum();
                        Com::printF( PSTR( " Service Reset = OK " ) );
                    }
                }
                else
                {
                    g_nServiceRequest = 0;
                }
            }
            g_nEnteredService  = 1;
        }
    }
#endif // FEATURE_SERVICE_INTERVAL

#if FEATURE_PAUSE_PRINTING
    checkPauseStatus_fromTask();
#endif // FEATURE_PAUSE_PRINTING

#if FEATURE_RGB_LIGHT_EFFECTS
    updateRGBLightStatus();
#endif // FEATURE_RGB_LIGHT_EFFECTS

    nEntered --;
    return;

} // loopRF

#if FEATURE_OUTPUT_FINISHED_OBJECT
void outputObject( void )
{
    char    unlock = !uid.locked;


    if( PrintLine::linesCount )
    {
        // there is some printing in progress at the moment - do not park the printer in this case
        if( Printer::debugErrors() )
        {
            Com::printFLN( PSTR( "outputObject(): the object can not be output while the printing is in progress" ) );
        }

        showError( (void*)ui_text_output_object, (void*)ui_text_operation_denied );
        return;
    }

    if( !Printer::areAxisHomed() )
    {
        // the printer does not know its home position, thus we can not output the object
        if( Printer::debugErrors() )
        {
            Com::printFLN( PSTR( "outputObject(): the object can not be output because the home position is unknown" ) );
        }

        showError( (void*)ui_text_output_object, (void*)ui_text_home_unknown );
        return;
    }

    UI_STATUS_UPD( UI_TEXT_OUTPUTTING_OBJECT );
    uid.lock();

    if( Printer::debugInfo() )
    {
        Com::printFLN( PSTR( "outputObject()" ) );
    }

#if FAN_PIN>-1 && FEATURE_FAN_CONTROL
    // disable the fan
    Commands::setFanSpeed(0,false);
#endif // FAN_PIN>-1 && FEATURE_FAN_CONTROL

    Commands::printCurrentPosition();

    uLastZPressureTime_IgnoreUntil = HAL::timeInMilliseconds()+10000L;
    
#if FEATURE_MILLING_MODE
    if( Printer::operatingMode == OPERATING_MODE_MILL )
    {
        GCode::executeFString(Com::tOutputObjectMill);
    }
    else
    {
        GCode::executeFString(Com::tOutputObjectPrint);
    }
#else
    GCode::executeFString(Com::tOutputObjectPrint);
#endif // FEATURE_MILLING_MODE

#if FEATURE_CONFIGURABLE_Z_ENDSTOPS && DEBUG_CONFIGURABLE_Z_ENDSTOPS
    unsigned long   uStart = HAL::timeInMilliseconds();
    while( PrintLine::hasLines() )
    {
        if( (HAL::timeInMilliseconds() - uStart) > 250 )
        {
            Com::printF( PSTR( "Z endstop type= "), Printer::ZEndstopType );
            Com::printF( PSTR( ", Z-Min= "), Printer::isZMinEndstopHit() );
            Com::printF( PSTR( ", Z-Max= "), Printer::isZMaxEndstopHit() );
            Com::printF( PSTR( ", lastZDirection= "), Printer::lastZDirection );
            Com::printF( PSTR( ", endstopZMinHit= "), Printer::endstopZMinHit );
            Com::printF( PSTR( ", endstopZMaxHit= "), Printer::endstopZMaxHit );
            Com::printF( PSTR( ", ZEndstopUnknown= "), Printer::ZEndstopUnknown );
            Com::printFLN( PSTR( "") );

            uStart = HAL::timeInMilliseconds();
        }

        GCode::readFromSerial();
        Commands::checkForPeriodicalActions();
        UI_MEDIUM;
    }
#endif // FEATURE_CONFIGURABLE_Z_ENDSTOPS && DEBUG_CONFIGURABLE_Z_ENDSTOPS

    Commands::waitUntilEndOfAllMoves();
    
    uLastZPressureTime_IgnoreUntil = 0;
    
    Commands::printCurrentPosition();
    
    // disable all steppers
    Printer::setAllSteppersDisabled();
    Printer::disableXStepper();
    Printer::disableYStepper();
    Printer::disableZStepper();
    Extruder::disableAllExtruders();

    if( unlock )
    {
        uid.unlock();
    }
    g_uStartOfIdle = HAL::timeInMilliseconds();

} // outputObject
#endif // FEATURE_OUTPUT_FINISHED_OBJECT


#if FEATURE_PARK
void parkPrinter( void )
{
    if( PrintLine::linesCount )
    {
        // there is some printing in progress at the moment - do not park the printer in this case
        if( Printer::debugErrors() )
        {
            Com::printFLN( PSTR( "parkPrinter(): the printer can not be parked while the printing is in progress" ) );
        }

        showError( (void*)ui_text_park_heat_bed, (void*)ui_text_operation_denied );
        return;
    }

    // it does not make sense to update the status here because the following homing operations will update the status by themselves
    //UI_STATUS_UPD( );

    if( Printer::debugInfo() )
    {
        Com::printFLN( PSTR( "parkPrinter()" ) );
    }

    Printer::homeAxis( true, true, true );

    Printer::moveToReal( g_nParkPosition[X_AXIS], g_nParkPosition[Y_AXIS], g_nParkPosition[Z_AXIS], IGNORE_COORDINATE, Printer::homingFeedrate[0]);

} // parkPrinter
#endif // FEATURE_PARK

#if FEATURE_PAUSE_PRINTING
inline bool processingDirectMove(){
    return  (
             (Printer::directPositionTargetSteps[X_AXIS] != Printer::directPositionCurrentSteps[X_AXIS]) ||
             (Printer::directPositionTargetSteps[Y_AXIS] != Printer::directPositionCurrentSteps[Y_AXIS]) ||
             (Printer::directPositionTargetSteps[Z_AXIS] != Printer::directPositionCurrentSteps[Z_AXIS]) ||
             (Printer::directPositionTargetSteps[E_AXIS] != Printer::directPositionCurrentSteps[E_AXIS]) ||
              PrintLine::direct.stepsRemaining > 0
            );
}

inline void checkPauseStatus_fromTask(){
    switch( g_pauseStatus )
    {
        case PAUSE_STATUS_TASKGOTO_PAUSE_1:
        {
            if( (Printer::directPositionTargetSteps[E_AXIS] == Printer::directPositionCurrentSteps[E_AXIS]) )
            {
                // we have reached the pause position - nothing except the extruder can have been moved
                g_pauseStatus = PAUSE_STATUS_PAUSED;
                g_uStartOfIdle = 0;
                uid.menuLevel = 0; //uid.executeAction(UI_ACTION_TOP_MENU);
                uid.menuPos[uid.menuLevel] = 0;
                UI_STATUS_UPD( UI_TEXT_PAUSED );
                Com::printFLN( PSTR("RequestPause:") ); //repetier
                Com::printFLN( PSTR( "// action:pause" ) ); //octoprint
                Printer::setMenuMode( MENU_MODE_PAUSED, true );
            }
            break;
        }
        case PAUSE_STATUS_TASKGOTO_PAUSE_2:
        {
#if FEATURE_MILLING_MODE
            // we have reached the pause position 1
            if( Printer::operatingMode == OPERATING_MODE_MILL )
            {
                if( !processingDirectMove() )
                {
                    // in operating mode mill, we have 2 pause positions because we have to leave the work part before we shall move into x/y direction
                    g_pauseStatus = PAUSE_STATUS_TASKGOTO_PAUSE_3;
                    determinePausePosition();
                    PrintLine::prepareDirectMove();
                }
            }else{
#endif // FEATURE_MILLING_MODE
                g_pauseStatus = PAUSE_STATUS_PAUSED;
                g_uStartOfIdle = 0;
                uid.menuLevel = 0; //uid.executeAction(UI_ACTION_TOP_MENU);
                uid.menuPos[uid.menuLevel] = 0;
                UI_STATUS_UPD( UI_TEXT_PAUSED );
                Com::printFLN( PSTR("RequestPause:") ); //repetier
                Com::printFLN( PSTR( "// action:pause" ) ); //octoprint
                Printer::setMenuMode( MENU_MODE_PAUSED, true );
#if FEATURE_MILLING_MODE
            }
#endif // FEATURE_MILLING_MODE
            break;
        }
#if FEATURE_MILLING_MODE
        case PAUSE_STATUS_TASKGOTO_PAUSE_3:
        {
            if( Printer::operatingMode == OPERATING_MODE_MILL )
            {
                if( !processingDirectMove() )
                {
                    g_pauseStatus = PAUSE_STATUS_PAUSED;
                    g_uStartOfIdle = 0;
                    uid.menuLevel = 0; //uid.executeAction(UI_ACTION_TOP_MENU);
                    uid.menuPos[uid.menuLevel] = 0;
                    UI_STATUS_UPD( UI_TEXT_PAUSED );
                    Com::printFLN( PSTR("RequestPause:") ); //repetier
                    Com::printFLN( PSTR( "// action:pause" ) ); //octoprint
                    Printer::setMenuMode( MENU_MODE_PAUSED, true );
                }
            }
            break;
        }
#endif // FEATURE_MILLING_MODE
    }
}

inline void waitforPauseStatus_fromButton(char Status){
    g_pauseStatus = Status; //give job to interrupt //g_pauseStatus is volatile ;)
    // wait until the current move is completed
    // performQueueMove sees that pauseStatus is altered and switches to strategy + calculates direct move which has to end later + sets pause status to PAUSE_STATUS_PAUSED
    while( g_pauseStatus != PAUSE_STATUS_PAUSED || PrintLine::direct.stepsRemaining ) //warte auf queue befehlsende
    {
        HAL::delayMilliseconds( 1 );
        Commands::checkForPeriodicalActions();
        GCode::keepAlive( Paused );
    }
}

void pausePrint( void )
{
    if( g_pauseMode == PAUSE_MODE_NONE )
    {
        if( PrintLine::linesCount ) // the printing is not paused at the moment
        {
            if( !Printer::areAxisHomed() ) // this should never happen
            {
                if( Printer::debugErrors() ) Com::printFLN( PSTR( "pausePrint(): home position is unknown" ) );
                showError( (void*)ui_text_pause, (void*)ui_text_home_unknown );
                return;
            }
            if( Printer::debugErrors() ) Com::printFLN( PSTR( "pausing..." ) );
            g_pauseMode   = PAUSE_MODE_PAUSED;
            g_uStartOfIdle  = 0;
            uid.menuLevel = 0; //uid.executeAction(UI_ACTION_TOP_MENU);
            uid.menuPos[uid.menuLevel] = 0;
            UI_STATUS_UPD( UI_TEXT_PAUSING );
            Com::printFLN( PSTR("RequestPause:") ); //repetier
            Com::printFLN( PSTR( "// action:pause" ) ); //octoprint
            waitforPauseStatus_fromButton(PAUSE_STATUS_GOTO_PAUSE1);
            g_uPauseTime    = HAL::timeInMilliseconds();
            g_pauseBeepDone = 0;

            if( Printer::debugInfo() ) Com::printFLN( PSTR( "pausePrint(): paused" ) );
            UI_STATUS_UPD( UI_TEXT_PAUSED );
            Printer::setMenuMode( MENU_MODE_PAUSED, true );
        }
        else
        {
            if( Printer::debugErrors() ) Com::printFLN( PSTR( "pausePrint(): nothing is printed" ) );
            showError( (void*)ui_text_pause, (void*)ui_text_operation_denied );
        }
        return;
    }

    if( g_pauseMode == PAUSE_MODE_PAUSED )
    {
        g_pauseMode   = PAUSE_MODE_PAUSED_AND_MOVED;
        g_uStartOfIdle  = 0;
        uid.menuLevel = 0; //uid.executeAction(UI_ACTION_TOP_MENU);
        uid.menuPos[uid.menuLevel] = 0;
        UI_STATUS_UPD( UI_TEXT_PAUSING );
        // in case the print is paused already, we move the printer head to the pause position
        if( Printer::debugInfo() ) Com::printFLN( PSTR( "pausePrint(): moving..." ) );

        waitforPauseStatus_fromButton(PAUSE_STATUS_GOTO_PAUSE2);
#if FEATURE_MILLING_MODE
        if( Printer::operatingMode == OPERATING_MODE_MILL )
        { // we do not process the extruder in case we are not in operating mode "print"
            waitforPauseStatus_fromButton(PAUSE_STATUS_GOTO_PAUSE3);
        }
#endif // FEATURE_MILLING_MODE

        if( Printer::debugInfo() ) Com::printFLN( PSTR( "pausePrint(): position reached" ) );
        UI_STATUS_UPD( UI_TEXT_PAUSED );
        return;
    }
} // pausePrint


void continuePrint( void )
{
    static char countplays = 1;
    if(g_pauseMode == PAUSE_MODE_NONE || g_pauseStatus != PAUSE_STATUS_PAUSED){
        if( Printer::debugErrors() ) Com::printFLN( PSTR( "continuePrint(): we are not paused." ) );
        if(countplays++ >= 10){
             Com::printFLN( PSTR( "LCD re-initialization") );
             countplays = 1;
             showInformation( PSTR(UI_TEXT_MANUAL), PSTR(UI_TEXT_Z_CIRCUIT), PSTR(UI_TEXT_RESET) );
             initializeLCD();
        } 
        return;
    }
    countplays = 1;

    g_uStartOfIdle    = 0;
    UI_STATUS_UPD( UI_TEXT_CONTINUING );
    BEEP_CONTINUE
#if FEATURE_MILLING_MODE
    bool nPrintingMode = ( Printer::operatingMode == OPERATING_MODE_PRINT );
#else
    bool nPrintingMode = true;
#endif // FEATURE_MILLING_MODE

    if( g_pauseMode == PAUSE_MODE_PAUSED )
    {
        if( nPrintingMode )
        {
            // process the extruder only in case we are in mode "print"
#if NUM_EXTRUDER > 0
            g_pauseStatus = PAUSE_STATUS_HEATING;
            bool wait = false; 
            for(uint8_t i = 0; i < NUM_EXTRUDER; i++){
#if EXTRUDER_CURRENT_PAUSE_DELAY
                setExtruderCurrent( i, Printer::motorCurrent[E_AXIS+i] );
#endif //EXTRUDER_CURRENT_PAUSE_DELAY
                if(extruder[i].paused){ //temperaturminimierung in paused wieder laden wenn gesetzt. Config aktuell nur über RFx000.h bei PAUSE_COOLDOWN
                    if(extruder[i].tempControl.targetTemperatureC + (float)extruder[i].paused <= EXTRUDER_MAX_TEMP){
                        extruder[i].tempControl.targetTemperatureC += (float)extruder[i].paused;
                        wait = true;
                    }
                    extruder[i].paused = 0;
                }
            }
            if(wait){
                for(uint8_t i = 0; i < NUM_EXTRUDER; i++) {
                    extruder[i].tempControl.waitForTargetTemperature();
                }
            }
#endif //NUM_EXTRUDER > 0

            if( g_nContinueSteps[E_AXIS] )
            {
                // continue to take back retract for pause
                waitforPauseStatus_fromButton(PAUSE_STATUS_PREPARE_CONTINUE1);
            }
        }
    }
    else if( g_pauseMode == PAUSE_MODE_PAUSED_AND_MOVED )
    {
        // move to the continue position
        if( Printer::debugInfo() ) Com::printFLN( PSTR( "continuePrint(): moving..." ) );
        if( nPrintingMode )
        {
#if NUM_EXTRUDER > 0
            g_pauseStatus = PAUSE_STATUS_HEATING;
            bool wait = false; 
            for(uint8_t i = 0; i < NUM_EXTRUDER; i++){
#if EXTRUDER_CURRENT_PAUSE_DELAY
                setExtruderCurrent( i, Printer::motorCurrent[E_AXIS+i] );
#endif //EXTRUDER_CURRENT_PAUSE_DELAY
                if(extruder[i].paused){ //temperaturminimierung in paused wieder laden wenn gesetzt. Config aktuell nur über RFx000.h bei PAUSE_COOLDOWN
                    if(extruder[i].tempControl.targetTemperatureC + (float)extruder[i].paused <= EXTRUDER_MAX_TEMP){
                        extruder[i].tempControl.targetTemperatureC += (float)extruder[i].paused;
                        wait = true;
                    }
                    extruder[i].paused = 0;
                }
            }
            if(wait){
                for(uint8_t i = 0; i < NUM_EXTRUDER; i++) {
                    extruder[i].tempControl.waitForTargetTemperature(ADD_CONTINUE_AFTER_PAUSE_TEMP_TOLERANCE);
                }
            }
#endif //NUM_EXTRUDER > 0
        }
        waitforPauseStatus_fromButton(PAUSE_STATUS_PREPARE_CONTINUE2_1);
        if( !nPrintingMode && g_nContinueSteps[Z_AXIS] )
        {
            // we are in operating mode mill - get back into the work part now
            waitforPauseStatus_fromButton(PAUSE_STATUS_PREPARE_CONTINUE2_2);
        }
    }

    Com::printFLN( PSTR("RequestContinue:") ); //repetier
    Com::printFLN( PSTR( "// action:resume" ) ); //octoprint
    // wait until the next move is started
    g_pauseMode   = PAUSE_MODE_NONE;
    g_pauseStatus = PAUSE_STATUS_NONE;

    if( Printer::debugInfo() )  Com::printFLN( PSTR( "continuePrint(): waiting for next move" ) );

    unsigned long   startTime = HAL::timeInMilliseconds();
    char            timeout   = 0;
    while( !PrintLine::cur )
    {
        if( !PrintLine::linesCount )
        {
            // the printing won't continue in case there is nothing else to do
            break;
        }
        HAL::delayMilliseconds( 1 );
        Commands::checkForPeriodicalActions();
        GCode::keepAlive( Paused );

        if( (HAL::timeInMilliseconds() - startTime) > 5000 )
        {
            // do not loop forever
            g_uStartOfIdle    = HAL::timeInMilliseconds();
            timeout = 1;
            break;
        }
    }

    if( Printer::debugInfo() ){
          if( timeout ) Com::printFLN( PSTR( "continuePrint(): the printing has been continued (timeout)" ) );
          else Com::printFLN( PSTR( "continuePrint(): the printing has been continued" ) );
    }

    if( nPrintingMode ){ UI_STATUS_UPD( UI_TEXT_PRINT_POS ); }
    else{ UI_STATUS_UPD( UI_TEXT_MILL_POS ); }
    Printer::setMenuMode( MENU_MODE_PAUSED, false );

} // continuePrint

void determinePausePosition( void )
{
#if FEATURE_MILLING_MODE
    if( Printer::operatingMode == OPERATING_MODE_PRINT )
    {
        determineZPausePositionForPrint();
    }
    else //Printer::operatingMode == OPERATING_MODE_MILL
    {
        // in operating mode "mill", we must move only into z direction first in order to get the tool out of the work part
        if( g_pauseStatus == PAUSE_STATUS_GOTO_PAUSE2 || g_pauseStatus == PAUSE_STATUS_TASKGOTO_PAUSE_2 )
        {
            g_nContinueSteps[X_AXIS] = 0;
            g_nContinueSteps[Y_AXIS] = 0;
            determineZPausePositionForMill();
            return;
        }
    }
#else
    determineZPausePositionForPrint();
#endif // FEATURE_MILLING_MODE

    if( g_nPauseSteps[X_AXIS] )
    {
        long Temp = g_nPauseSteps[X_AXIS];
        Temp += Printer::queuePositionCurrentSteps[X_AXIS];
        Temp += Printer::directPositionTargetSteps[X_AXIS];

        if( g_nPauseSteps[X_AXIS] < 0 )
        {
            if( Temp < PAUSE_X_SPACING )
            {
                // we can move only partially
                Temp = PAUSE_X_SPACING - Printer::directPositionTargetSteps[X_AXIS];
                Temp -= Printer::queuePositionCurrentSteps[X_AXIS];

                Printer::directPositionTargetSteps[X_AXIS] += Temp;
                g_nContinueSteps[X_AXIS]                   =  -Temp;
            }
            else
            {
                Printer::directPositionTargetSteps[X_AXIS] += g_nPauseSteps[X_AXIS];
                g_nContinueSteps[X_AXIS]                   =  -g_nPauseSteps[X_AXIS];
            }
        }
        else if( g_nPauseSteps[X_AXIS] > 0 )
        {
            long  Max = long(Printer::lengthMM[X_AXIS] * Printer::axisStepsPerMM[X_AXIS]) - PAUSE_X_SPACING;
            if( Temp > Max )
            {
                // we can move only partially
                Temp =  Max - Printer::directPositionTargetSteps[X_AXIS];
                Temp -= Printer::queuePositionCurrentSteps[X_AXIS];

                Printer::directPositionTargetSteps[X_AXIS] += Temp;
                g_nContinueSteps[X_AXIS]                   =  -Temp;
            }
            else
            {
                Printer::directPositionTargetSteps[X_AXIS] += g_nPauseSteps[X_AXIS];
                g_nContinueSteps[X_AXIS]                   =  -g_nPauseSteps[X_AXIS];
            }
        }
    }else{
        g_nContinueSteps[X_AXIS] = 0;
    }

    if( g_nPauseSteps[Y_AXIS] )
    {
        long Temp = g_nPauseSteps[Y_AXIS];
        Temp += Printer::queuePositionCurrentSteps[Y_AXIS];
        Temp += Printer::directPositionTargetSteps[Y_AXIS];

        if( g_nPauseSteps[Y_AXIS] < 0 )
        {
            if( Temp < PAUSE_Y_SPACING )
            {
                // we can move only partially
                Temp =  PAUSE_Y_SPACING - Printer::directPositionTargetSteps[Y_AXIS];
                Temp -= Printer::queuePositionCurrentSteps[Y_AXIS];

                Printer::directPositionTargetSteps[Y_AXIS] += Temp;
                g_nContinueSteps[Y_AXIS]                   =  -Temp;
            }
            else
            {
                Printer::directPositionTargetSteps[Y_AXIS] += g_nPauseSteps[Y_AXIS];
                g_nContinueSteps[Y_AXIS]                   =  -g_nPauseSteps[Y_AXIS];
            }
        }
        else if( g_nPauseSteps[Y_AXIS] > 0 )
        {
            long  Max = long(Printer::lengthMM[Y_AXIS] * Printer::axisStepsPerMM[Y_AXIS]) - PAUSE_Y_SPACING;
            if( Temp > Max )
            {
                // we can move only partially
                Temp =  Max - Printer::directPositionTargetSteps[Y_AXIS];
                Temp -= Printer::queuePositionCurrentSteps[Y_AXIS];

                Printer::directPositionTargetSteps[Y_AXIS] += Temp;
                g_nContinueSteps[Y_AXIS]                   =  -Temp;
            }
            else
            {
                Printer::directPositionTargetSteps[Y_AXIS] += g_nPauseSteps[Y_AXIS];
                g_nContinueSteps[Y_AXIS]                   =  -g_nPauseSteps[Y_AXIS];
            }
        }
    }else{
        g_nContinueSteps[Y_AXIS] = 0;
    }
} // determinePausePosition

void determineZPausePositionForPrint( void )
{
    // in operating mode "print", pausing drives from the current position downwards the specified g_nPauseSteps[Z_AXIS]
    if( g_nPauseSteps[Z_AXIS] )
    {
        long Temp =  g_nPauseSteps[Z_AXIS];
        Temp += Printer::queuePositionCurrentSteps[Z_AXIS];

#if FEATURE_HEAT_BED_Z_COMPENSATION || FEATURE_WORK_PART_Z_COMPENSATION
        Temp += Printer::compensatedPositionCurrentStepsZ;
#endif // FEATURE_HEAT_BED_Z_COMPENSATION || FEATURE_WORK_PART_Z_COMPENSATION
        Temp += Printer::directPositionTargetSteps[Z_AXIS];

        long Max  =  long(Printer::lengthMM[Z_AXIS] * Printer::axisStepsPerMM[Z_AXIS]) - PAUSE_Z_MAX_SPACING;

        if( Temp <= Max )
        {
            Printer::directPositionTargetSteps[Z_AXIS] += g_nPauseSteps[Z_AXIS];
            g_nContinueSteps[Z_AXIS]                   =  -g_nPauseSteps[Z_AXIS];
        }
        else
        {
            // we can move only partially
            Temp =  Max - Printer::directPositionTargetSteps[Z_AXIS];
            Temp -= Printer::queuePositionCurrentSteps[Z_AXIS];

#if FEATURE_HEAT_BED_Z_COMPENSATION || FEATURE_WORK_PART_Z_COMPENSATION
            Temp -= Printer::compensatedPositionCurrentStepsZ;
#endif // FEATURE_HEAT_BED_Z_COMPENSATION || FEATURE_WORK_PART_Z_COMPENSATION

            Printer::directPositionTargetSteps[Z_AXIS] += Temp;
            g_nContinueSteps[Z_AXIS]                   =  -Temp;
        }
    }else{
        g_nContinueSteps[Z_AXIS] = 0;
    }
    return;

} // determineZPausePositionForPrint

void determineZPausePositionForMill( void )
{
    long    Temp;

    // in operating mode "mill", pausing drives from the current position downwards the specified g_nPauseSteps[Z_AXIS] + queuePositionCurrentSteps[Z_AXIS] because we must drive the tool out of the work part before we can move into x or y direction
    Temp =  g_nPauseSteps[Z_AXIS];
    Temp -= Printer::queuePositionCurrentSteps[Z_AXIS]; // in operating mode "mill", the bed/work part moves upwards while the milling is in progress - Printer::queuePositionCurrentSteps[Z_AXIS] is negative

    Printer::directPositionTargetSteps[Z_AXIS] += Temp;
    g_nContinueSteps[Z_AXIS]                   =  -Temp;
    return;

} // determineZPausePositionForMill

void waitUntilContinue( void ) //Nibbels: Verstehe ich nicht! Man sollte Pause und Continue nutzen?? Aber warum das? Wegen der Gcode-Queue? Aber Pause hält auch die Queue an. .... TODO-> 03.09.2017 Das ist ein Warte-GCode 3071 der aufs Auflösen der Pause wartet, aber auch andere GCodes blockt. Kann mir nur gerade keine Anwendung dafür ausdenken.
{
    if( g_pauseStatus == PAUSE_STATUS_NONE )
    {
        // we are not paused at the moment
        return;
    }
    
    while ( g_pauseStatus != PAUSE_STATUS_NONE )
    {
        GCode::readFromSerial();
        Commands::checkForPeriodicalActions();
        GCode::keepAlive( Paused );
        UI_MEDIUM;
    }
} // waitUntilContinue
#endif // FEATURE_PAUSE_PRINTING


void setExtruderCurrent( uint8_t nr, uint8_t current )
{
    if(nr > NUM_EXTRUDER - 1) nr = NUM_EXTRUDER - 1; //0 ist T0, 1 ist T1

#if FEATURE_MILLING_MODE
    if( Printer::operatingMode != OPERATING_MODE_PRINT )
    {
        // we have no extruder when we are not in print mode
        return;
    }
#endif // FEATURE_MILLING_MODE

    // set the current for the extruder motor

    setMotorCurrent( 1+E_AXIS+nr , current ); //das ist Extruder-ID+1, also nicht [0,1,2] -> Extruder 4 = [3], sondern [4]. Extruder5 wäre [5]

    if( Printer::debugInfo() )
    {
        Com::printF( PSTR( "Extruder" ), (short)nr );
        Com::printFLN( PSTR( " = " ), (uint32_t)current );
    }
    return;

} // setExtruderCurrent


void processCommand( GCode* pCommand )
{
    long    nTemp;


    if( pCommand->hasM() )
    {
        switch( pCommand->M )
        {
#if FEATURE_HEAT_BED_Z_COMPENSATION
            case 3000: // M3000 - turn the z-compensation off
            {
                if( isSupportedMCommand( pCommand->M, OPERATING_MODE_PRINT ) )
                {
                    if( Printer::debugInfo() )
                    {
                        Com::printFLN( PSTR( "M3000: disabling z compensation" ) );
                    }
                    queueTask( TASK_DISABLE_Z_COMPENSATION );
                }
                break;
            }
            case 3001: // M3001 - turn the z-compensation on
            {
                if( isSupportedMCommand( pCommand->M, OPERATING_MODE_PRINT ) )
                {
                    if( Printer::doHeatBedZCompensation )
                    {
                        if( Printer::debugInfo() )
                        {
                            Com::printFLN( PSTR( "M3001: the z compensation is enabled already" ) );
                        }
                        break;
                    }

                    if( Printer::areAxisHomed() )
                    {
                        if( g_ZCompensationMatrix[0][0] != EEPROM_FORMAT )
                        {
                            // we load the z compensation matrix before its first usage because this can take some time
                            prepareZCompensation();
                        }

                        if( g_ZCompensationMatrix[0][0] == EEPROM_FORMAT )
                        {
                            // enable the z compensation only in case we have valid compensation values
                            if( Printer::debugInfo() )
                            {
                                Com::printFLN( PSTR( "M3001: enabling z compensation" ) );
                            }
                            Commands::waitUntilEndOfAllMoves();
                            queueTask( TASK_ENABLE_Z_COMPENSATION );
                            Commands::waitUntilEndOfAllMoves();
                        }
                        else
                        {
                            if( Printer::debugErrors() )
                            {
                                Com::printF( PSTR( "M3001: the z compensation can not be enabled because the heat bed compensation matrix is not valid ( " ), g_ZCompensationMatrix[0][0] );
                                Com::printF( PSTR( " / " ), EEPROM_FORMAT );
                                Com::printFLN( PSTR( " )" ) );
                            }

                            showError( (void*)ui_text_z_compensation, (void*)ui_text_invalid_matrix );
                        }
                    }
                    else
                    {
                        if( Printer::debugErrors() )
                        {
                            Com::printFLN( PSTR( "M3001: the z compensation can not be enabled because the home position is unknown" ) );
                        }

                        showError( (void*)ui_text_z_compensation, (void*)ui_text_home_unknown );
                    }
                }
                break;
            }
            case 3002: // M3002 [S] - configure the min z-compensation scope ( S - units are [steps] )
            {
                if( isSupportedMCommand( pCommand->M, OPERATING_MODE_PRINT ) )
                {
                    if( pCommand->hasS() )
                    {
                        // test and take over the specified value
                        nTemp = pCommand->S;
                        if( nTemp < (Printer::axisStepsPerMM[Z_AXIS] /10) ) nTemp = Printer::axisStepsPerMM[Z_AXIS] /10;
                        if( nTemp > (Printer::axisStepsPerMM[Z_AXIS] *10) ) nTemp = Printer::axisStepsPerMM[Z_AXIS] *10;

                        if( nTemp > g_maxZCompensationSteps )
                        {
                            // the minimal z-compensation scope can not be bigger than the maximal z-compensation scope
                            nTemp = g_maxZCompensationSteps;
                        }

                        g_minZCompensationSteps = nTemp;
                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3002: new min z-compensation scope: " ), g_minZCompensationSteps );
                            Com::printFLN( PSTR( " [steps]" ) );
                        }

                        g_diffZCompensationSteps = g_maxZCompensationSteps - g_minZCompensationSteps;
                    }
                    else
                    {
                        showInvalidSyntax( pCommand->M );
                    }
                }
                break;
            }
            case 3003: // M3003 [S] - configure the max z-compensation scope ( S - units are [steps] )
            {
                if( isSupportedMCommand( pCommand->M, OPERATING_MODE_PRINT ) )
                {
                    if( pCommand->hasS() )
                    {
                        // test and take over the specified value
                        nTemp = pCommand->S;
                        if( nTemp < (Printer::axisStepsPerMM[Z_AXIS] /10) ) nTemp = Printer::axisStepsPerMM[Z_AXIS] /10;
                        if( nTemp > (Printer::axisStepsPerMM[Z_AXIS] *10) ) nTemp = Printer::axisStepsPerMM[Z_AXIS] *10;

                        if( nTemp < g_minZCompensationSteps )
                        {
                            // the maximal z-compensation scope can not be smaller than the minimal z-compensation scope
                            nTemp = g_minZCompensationSteps;
                        }

                        g_maxZCompensationSteps = nTemp;
                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3003: new max z-compensation scope: " ), g_maxZCompensationSteps );
                            Com::printFLN( PSTR( " [steps]" ) );
                        }

                        g_diffZCompensationSteps = g_maxZCompensationSteps - g_minZCompensationSteps;
                    }
                    else
                    {
                        showInvalidSyntax( pCommand->M );
                    }
                }
                break;
            }
            case 3007: // M3007 [S] [Z] - configure the min z-compensation scope ( S - units are [um], Z - units are [mm] )
            {
                if( isSupportedMCommand( pCommand->M, OPERATING_MODE_PRINT ) )
                {
                    if( pCommand->hasZ() )
                    {
                        // test and take over the specified value
                        if( pCommand->Z < 0.1 )     pCommand->Z = 0.1;
                        if( pCommand->Z > 10 )      pCommand->Z = 10;

                        g_minZCompensationSteps = long(pCommand->Z * Printer::axisStepsPerMM[Z_AXIS]);
                        if( g_minZCompensationSteps > g_maxZCompensationSteps )
                        {
                            // the minimal z-compensation scope can not be bigger than the maximal z-compensation scope
                            g_minZCompensationSteps = g_maxZCompensationSteps;
                        }

                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3007: new min z-compensation scope: " ), pCommand->Z );
                            Com::printF( PSTR( " [mm]" ) );
                            Com::printF( PSTR( " / " ), g_minZCompensationSteps );
                            Com::printFLN( PSTR( " [steps]" ) );
                        }

                        g_diffZCompensationSteps = g_maxZCompensationSteps - g_minZCompensationSteps;
                    }
                    else if( pCommand->hasS() )
                    {
                        // test and take over the specified value
                        nTemp = pCommand->S;
                        if( nTemp < 100 )       nTemp = 100;
                        if( nTemp > 10000 )     nTemp = 10000;

                        g_minZCompensationSteps = (nTemp * Printer::axisStepsPerMM[Z_AXIS]) / 1000;
                        if( g_minZCompensationSteps > g_maxZCompensationSteps )
                        {
                            // the minimal z-compensation scope can not be bigger than the maximal z-compensation scope
                            g_minZCompensationSteps = g_maxZCompensationSteps;
                        }

                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3007: new min z-compensation scope: " ), nTemp );
                            Com::printF( PSTR( " [um]" ) );
                            Com::printF( PSTR( " / " ), g_minZCompensationSteps );
                            Com::printFLN( PSTR( " [steps]" ) );
                        }

                        g_diffZCompensationSteps = g_maxZCompensationSteps - g_minZCompensationSteps;
                    }
                    else
                    {
                        showInvalidSyntax( pCommand->M );
                    }
                }
                break;
            }
            case 3008: // M3008 [S] [Z] - configure the max z-compensation scope ( S - units are [um], Z - units are [mm] )
            {
                if( isSupportedMCommand( pCommand->M, OPERATING_MODE_PRINT ) )
                {
                    if( pCommand->hasZ() )
                    {
                        // test and take over the specified value
                        if( pCommand->Z < 0.1 )     pCommand->Z = 0.1;
                        if( pCommand->Z > 10 )      pCommand->Z = 10;

                        g_maxZCompensationSteps = long(pCommand->Z * Printer::axisStepsPerMM[Z_AXIS]);
                        if( g_maxZCompensationSteps < g_minZCompensationSteps )
                        {
                            // the maximal z-compensation scope can not be smaller than the minimal z-compensation scope
                            g_maxZCompensationSteps = g_minZCompensationSteps;
                        }

                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3008: new max z-compensation scope: " ), pCommand->Z );
                            Com::printF( PSTR( " [mm]" ) );
                            Com::printF( PSTR( " / " ), g_maxZCompensationSteps );
                            Com::printFLN( PSTR( " [steps]" ) );
                        }

                        g_diffZCompensationSteps = g_maxZCompensationSteps - g_minZCompensationSteps;
                    }
                    else if( pCommand->hasS() )
                    {
                        // test and take over the specified value
                        nTemp = pCommand->S;
                        if( nTemp < 100 )       nTemp = 100;
                        if( nTemp > 10000 )     nTemp = 10000;

                        g_maxZCompensationSteps = (nTemp * Printer::axisStepsPerMM[Z_AXIS]) / 1000;
                        if( g_maxZCompensationSteps < g_minZCompensationSteps )
                        {
                            // the maximal z-compensation scope can not be smaller than the minimal z-compensation scope
                            g_maxZCompensationSteps = g_minZCompensationSteps;
                        }

                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3008: new max z-compensation scope: " ), nTemp );
                            Com::printF( PSTR( " [um]" ) );
                            Com::printF( PSTR( " / " ), g_maxZCompensationSteps );
                            Com::printFLN( PSTR( " [steps]" ) );
                        }

                        g_diffZCompensationSteps = g_maxZCompensationSteps - g_minZCompensationSteps;
                    }
                    else
                    {
                        showInvalidSyntax( pCommand->M );
                    }
                }
                break;
            }
#endif // FEATURE_HEAT_BED_Z_COMPENSATION

            case 3005: // M3005 [S] - enable custom debug outputs
            {
                if( pCommand->hasS() )
                {
                    // test and take over the specified value
                    nTemp = pCommand->S;
                    if( nTemp < 0 ) nTemp = 0;

                    g_debugLevel = (char)nTemp;
                    if( Printer::debugInfo() )
                    {
                        Com::printFLN( PSTR( "M3005: new debug level: " ), g_debugLevel );
                    }
                }
                else
                {
                    showInvalidSyntax( pCommand->M );
                }
                break;
            }

#if FEATURE_HEAT_BED_Z_COMPENSATION
            case 3006: // M3006 [S] [Z] - configure the static z-offset ( S - units are [um], Z - units are [mm] )
            {
                if( isSupportedMCommand( pCommand->M, OPERATING_MODE_PRINT ) )
                {
                    if( pCommand->hasZ() )
                    {
                        // test and take over the specified value
                        if( pCommand->Z < -HEAT_BED_Z_COMPENSATION_MAX_MM )     pCommand->Z = -HEAT_BED_Z_COMPENSATION_MAX_MM;
                        if( pCommand->Z > HEAT_BED_Z_COMPENSATION_MAX_MM )      pCommand->Z = HEAT_BED_Z_COMPENSATION_MAX_MM;

                        g_staticZSteps = long(pCommand->Z * Printer::axisStepsPerMM[Z_AXIS]);
                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3006: new static z-offset: " ), pCommand->Z );
                            Com::printF( PSTR( " [mm]" ) );
                            Com::printF( PSTR( " / " ), g_staticZSteps );
                            Com::printFLN( PSTR( " [steps]" ) );
                        }
                        Printer::ZOffset = long(pCommand->Z * 1000);

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                        if( HAL::eprGetInt32( EPR_RF_Z_OFFSET ) != Printer::ZOffset )
                        {
                            HAL::eprSetInt32( EPR_RF_Z_OFFSET, Printer::ZOffset );
                            EEPROM::updateChecksum();
                        }
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
                    }
                    else if( pCommand->hasS() )
                    {
                        // test and take over the specified value
                        nTemp = pCommand->S;

                        if( nTemp < -(HEAT_BED_Z_COMPENSATION_MAX_MM * 1000) )  nTemp = -(HEAT_BED_Z_COMPENSATION_MAX_MM * 1000);
                        if( nTemp > (HEAT_BED_Z_COMPENSATION_MAX_MM * 1000) )   nTemp = (HEAT_BED_Z_COMPENSATION_MAX_MM * 1000);

                        g_staticZSteps = (nTemp * Printer::axisStepsPerMM[Z_AXIS]) / 1000;
                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3006: new static z-offset: " ), nTemp );
                            Com::printF( PSTR( " [um]" ) );
                            Com::printF( PSTR( " / " ), g_staticZSteps );
                            Com::printFLN( PSTR( " [steps]" ) );
                        }
                        Printer::ZOffset = nTemp;

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                        if( HAL::eprGetInt32( EPR_RF_Z_OFFSET ) != Printer::ZOffset )
                        {
                            HAL::eprSetInt32( EPR_RF_Z_OFFSET, Printer::ZOffset );
                            EEPROM::updateChecksum();
                        }
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
                    }
                    else
                    {
                        showInvalidSyntax( pCommand->M );
                    }
                }
                break;
            }
            case 3009: // M3009 [S] - get/choose the active heat bed z-compensation matrix
            {
                if( isSupportedMCommand( pCommand->M, OPERATING_MODE_PRINT ) )
                {
                    if( Printer::doHeatBedZCompensation )
                    {
                        // do not allow to change the current heat bed z-compensation matrix while the z-compensation is active
                        if( Printer::debugErrors() )
                        {
                            Com::printFLN( PSTR( "M3009: the heat bed z matrix can not be changed while the z-compensation is active" ) );
                        }

                        showError( (void*)ui_text_z_compensation, (void*)ui_text_operation_denied );
                        break;
                    }

                    if( pCommand->hasS() )
                    {
                        nTemp = pCommand->S;

                        if( nTemp < 1 || nTemp > EEPROM_MAX_HEAT_BED_SECTORS )
                        {
                            if( Printer::debugErrors() )
                            {
                                Com::printF( PSTR( "M3009: invalid heat bed z matrix (" ), nTemp );
                                Com::printFLN( PSTR( ")" ) );
                            }
                            break;
                        }

                        if( g_nActiveHeatBed != nTemp )
                        {
                            // we have to switch to another heat bed z-compensation matrix
                            switchActiveHeatBed( (char)nTemp );
                        }
                    }

                    if( Printer::debugInfo() )
                    {
                        Com::printFLN( PSTR( "M3009: currently active heat bed z matrix: " ), g_nActiveHeatBed );
                    }
                }
                break;
            }
            case 3010: // M3010 [S] - start/abort the heat bed scan
            {
                if( isSupportedMCommand( pCommand->M, OPERATING_MODE_PRINT ) )
                {
#if FEATURE_PRECISE_HEAT_BED_SCAN
                    if( pCommand->hasS() )
                    {
                        if( pCommand->S == HEAT_BED_SCAN_MODE_PLA ||
                            pCommand->S == HEAT_BED_SCAN_MODE_ABS )
                        {
                            g_nHeatBedScanMode = (char)pCommand->S;
                        }
                        else
                        {
                            g_nHeatBedScanMode = 0;
                        }
                    }
                    else
                    {
                        g_nHeatBedScanMode = 0;
                    } 
#endif // FEATURE_PRECISE_HEAT_BED_SCAN

                    startHeatBedScan(); 
                }
                break;
            }
            case 3011: // M3011 [S] - clear the specified z-compensation matrix from the EEPROM
            {
                if( isSupportedMCommand( pCommand->M, OPERATING_MODE_PRINT ) )
                {
                    if( pCommand->hasS() )
                    {
                        nTemp = pCommand->S;
                    }
                    else
                    {
                        // we clear the current z-compensation matrix in case no other z-compensation matrix is specified
                        nTemp = g_nActiveHeatBed;
                    }

                    if( nTemp < 1 || nTemp > EEPROM_MAX_HEAT_BED_SECTORS )
                    {
                        if( Printer::debugErrors() )
                        {
                            Com::printF( PSTR( "M3011: invalid heat bed z matrix (" ), nTemp );
                            Com::printFLN( PSTR( ")" ) );
                        }
                        break;
                    }

                    // switch to the specified z-compensation matrix
                    g_nActiveHeatBed = (char)nTemp;
                    writeWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, EEPROM_OFFSET_ACTIVE_HEAT_BED_Z_MATRIX, g_nActiveHeatBed );
                    clearCompensationMatrix( (unsigned int)(EEPROM_SECTOR_SIZE * g_nActiveHeatBed) );

                    if( Printer::debugInfo() )
                    {
                        Com::printFLN( PSTR( "M3011: cleared heat bed z matrix: " ), nTemp );
                    }

                    // TODO: in case the z-compensation matrix is active at the moment, this command should not work
                }
                break;
            }
            case 3012: // M3012 - restore the default scan parameters
            {
                if( isSupportedMCommand( pCommand->M, OPERATING_MODE_PRINT ) )
                {
                    restoreDefaultScanParameters();
                }
                break;
            }
            case 3013: // M3013 [S] [P] - output the current heat bed z-compensation matrix
            {
                if( isSupportedMCommand( pCommand->M, OPERATING_MODE_PRINT ) )
                {
                    char    format;


                    if( pCommand->hasS() )
                    {
                        nTemp = pCommand->S;
                    }
                    else
                    {
                        // we output the current z-compensation matrix in case no other z-compensation matrix is specified
                        nTemp = g_nActiveHeatBed;
                    }

                    if( pCommand->hasP() )
                    {
                        format = pCommand->P ? 1 : 0;
                    }
                    else
                    {
                        format = 0;
                    }

                    if( nTemp < 0 || nTemp > EEPROM_MAX_HEAT_BED_SECTORS )
                    {
                        if( Printer::debugErrors() )
                        {
                            Com::printF( PSTR( "M3013: invalid heat bed z matrix (" ), nTemp );
                            Com::printFLN( PSTR( ")" ) );
                        }
                        break;
                    }

                    if( g_nActiveHeatBed != nTemp )
                    {
                        // we have to switch to another z-compensation matrix
                        switchActiveHeatBed( (char)nTemp );
                    }

                    if( g_ZCompensationMatrix[0][0] != EEPROM_FORMAT )
                    {
                        // we load the z-compensation matrix before its first usage because this can take some time
                        prepareZCompensation();
                    }

                    if( g_ZCompensationMatrix[0][0] == EEPROM_FORMAT )
                    {
                        if( Printer::debugInfo() )
                        {
                            Com::printFLN( PSTR( "M3013: current heat bed z-compensation matrix: " ) );
                        }
                        outputCompensationMatrix( format );
                    }
                    else
                    {
                        if( Printer::debugErrors() )
                        {
                            Com::printF( PSTR( "M3013: the heat bed z-compensation matrix is not valid ( " ), g_ZCompensationMatrix[0][0] );
                            Com::printF( PSTR( " / " ), EEPROM_FORMAT );
                            Com::printFLN( PSTR( " )" ) );
                        }

                        showError( (void*)ui_text_z_compensation, (void*)ui_text_invalid_matrix );
                    }
                }
                break;
            }
            case 3020: // M3020 [S] - configure the x start position for the heat bed scan ( units are [mm] )
            {
                if( isSupportedMCommand( pCommand->M, OPERATING_MODE_PRINT ) )
                {
                    if( pCommand->hasS() )
                    {
                        // test and take over the specified value
                        nTemp = pCommand->S;
                        if( nTemp < 5 )                                 nTemp = 5;
                        if( nTemp > (Printer::lengthMM[X_AXIS] -5) )    nTemp = Printer::lengthMM[X_AXIS] -5;

                        g_nScanXStartSteps = (long)((float)nTemp * Printer::axisStepsPerMM[X_AXIS]);
                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3020: new x start position: " ), nTemp );
                            Com::printF( PSTR( " [mm], " ), (int)g_nScanXStartSteps );
                            Com::printFLN( PSTR( " [steps]" ) );
                        }               
                    }
                    else
                    {
                        showInvalidSyntax( pCommand->M );
                    }
                }
                break;
            }
            case 3021: // M3021 [S] - configure the y start position for the heat bed scan ( units are [mm] )
            {
                if( isSupportedMCommand( pCommand->M, OPERATING_MODE_PRINT ) )
                {
                    if( pCommand->hasS() )
                    {
                        // test and take over the specified value
                        nTemp = pCommand->S;
                        if( nTemp < 5 )                     nTemp = 5;
                        if( nTemp > (Y_MAX_LENGTH -5 ) )    nTemp = Y_MAX_LENGTH -5;

                        g_nScanYStartSteps = (long)((float)nTemp * Printer::axisStepsPerMM[Y_AXIS]);
                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3021: new y start position: " ), nTemp );
                            Com::printF( PSTR( " [mm], " ), (int)g_nScanYStartSteps );
                            Com::printFLN( PSTR( " [steps]" ) );
                        }
                    }
                    else
                    {
                        showInvalidSyntax( pCommand->M );
                    }
                }
                break;
            }
            case 3022: // M3022 [S] - configure the x step size for the heat bed scan ( units are [mm] )
            {
                if( isSupportedMCommand( pCommand->M, OPERATING_MODE_PRINT ) )
                {
                    if( pCommand->hasS() )
                    {
                        // test and take over the specified value
                        nTemp = pCommand->S;
                        if( nTemp < HEAT_BED_SCAN_X_STEP_SIZE_MIN_MM )  nTemp = HEAT_BED_SCAN_X_STEP_SIZE_MIN_MM;
                        if( nTemp > 100 )                               nTemp = 100;

                        g_nScanXStepSizeSteps = (long)((float)nTemp * Printer::axisStepsPerMM[X_AXIS]);
                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3022: new x step size: " ), (int)nTemp );
                            Com::printF( PSTR( " [mm], " ), (int)g_nScanXStepSizeSteps );
                            Com::printFLN( PSTR( " [steps]" ) );
                        }
                    }
                    else
                    {
                        showInvalidSyntax( pCommand->M );
                    }
                }
                break;
            }
            case 3023: // M3023 [S] - configure the y step size for the heat bed scan ( units are [mm] )
            {
                if( isSupportedMCommand( pCommand->M, OPERATING_MODE_PRINT ) )
                {
                    if( pCommand->hasS() )
                    {
                        // test and take over the specified value
                        nTemp = pCommand->S;
                        if( nTemp < HEAT_BED_SCAN_Y_STEP_SIZE_MIN_MM )  nTemp = HEAT_BED_SCAN_Y_STEP_SIZE_MIN_MM;
                        if( nTemp > 100 )                               nTemp = 100;

                        g_nScanYStepSizeSteps = (long)((float)nTemp * Printer::axisStepsPerMM[Y_AXIS]);
                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3023: new y step size: " ), (int)nTemp );
                            Com::printF( PSTR( " [mm], " ), (int)g_nScanYStepSizeSteps );
                            Com::printFLN( PSTR( " [steps]" ) );
                        }
                    }
                    else
                    {
                        showInvalidSyntax( pCommand->M );
                    }
                }
                break;
            }
            case 3024: // M3024 [S] - configure the x end position for the heat bed scan ( units are [mm] )
            {
                if( isSupportedMCommand( pCommand->M, OPERATING_MODE_PRINT ) )
                {
                    if( pCommand->hasS() )
                    {
                        // test and take over the specified value
                        nTemp = pCommand->S;
                        if( nTemp < 5 )                                 nTemp = 5;
                        if( nTemp > (Printer::lengthMM[X_AXIS] -5) )    nTemp = Printer::lengthMM[X_AXIS] -5;

                        g_nScanXEndSteps = (long)((float)nTemp * Printer::axisStepsPerMM[X_AXIS]);
                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3024: new x end position: " ), nTemp );
                            Com::printF( PSTR( " [mm], " ), (int)g_nScanXEndSteps );
                            Com::printFLN( PSTR( " [steps]" ) );
                        }

                        g_nScanXMaxPositionSteps = long(Printer::lengthMM[X_AXIS] * Printer::axisStepsPerMM[X_AXIS] - g_nScanXEndSteps);
                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3024: new x max position: " ), (int)g_nScanXMaxPositionSteps );
                            Com::printFLN( PSTR( " [steps]" ) );
                        }
                    }
                    else
                    {
                        showInvalidSyntax( pCommand->M );
                    }
                }
                break;
            }
            case 3025: // M3025 [S] - configure the y end position for the heat bed scan ( units are [mm] )
            {
                if( isSupportedMCommand( pCommand->M, OPERATING_MODE_PRINT ) )
                {
                    if( pCommand->hasS() )
                    {
                        // test and take over the specified value
                        nTemp = pCommand->S;
                        if( nTemp < 5 )                     nTemp = 5;
                        if( nTemp > (Y_MAX_LENGTH -5 ) )    nTemp = Y_MAX_LENGTH -5;

                        g_nScanYEndSteps = (long)((float)nTemp * Printer::axisStepsPerMM[Y_AXIS]);
                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3025: new y end position: " ), nTemp );
                            Com::printF( PSTR( " [mm], " ), (int)g_nScanYEndSteps );
                            Com::printFLN( PSTR( " [steps]" ) );
                        }

                        g_nScanYMaxPositionSteps = long(Y_MAX_LENGTH * Printer::axisStepsPerMM[Y_AXIS] - g_nScanYEndSteps);
                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3025: new y max position: " ), (int)g_nScanYMaxPositionSteps );
                            Com::printFLN( PSTR( " [steps]" ) );
                        }
                    }
                    else
                    {
                        showInvalidSyntax( pCommand->M );
                    }
                }
                break;
            }
#endif // FEATURE_HEAT_BED_Z_COMPENSATION

#if FEATURE_HEAT_BED_Z_COMPENSATION || FEATURE_WORK_PART_Z_COMPENSATION
            case 3030: // M3030 [S] - configure the fast step size for moving of the heat bed up during the heat bed/work part scan
            {
                if( pCommand->hasS() )
                {
                    // test and take over the specified value
                    nTemp = pCommand->S;
                    if( nTemp > 50 )    nTemp = 50;
                    if( nTemp < 1 )     nTemp = 1;

                    g_nScanHeatBedUpFastSteps = -nTemp;
                    if( Printer::debugInfo() )
                    {
                        Com::printF( PSTR( "M3030: new fast step size for moving of the bed up: " ), (int)g_nScanHeatBedUpFastSteps );
                        Com::printFLN( PSTR( " [steps]" ) );
                    }
                }
                else
                {
                    showInvalidSyntax( pCommand->M );
                }
                break;
            }
            case 3031: // M3031 [S] - configure the slow step size for moving of the heat bed up during the heat bed/work part scan
            {
                if( pCommand->hasS() )
                {
                    // test and take over the specified value
                    nTemp = pCommand->S;
                    if( nTemp > 50 )    nTemp = 50;
                    if( nTemp < 1 )     nTemp = 1;

                    g_nScanHeatBedUpSlowSteps = -nTemp;
                    if( Printer::debugInfo() )
                    {
                        Com::printF( PSTR( "M3031: new slow step size for moving of the bed up: " ), (int)g_nScanHeatBedUpSlowSteps );
                        Com::printFLN( PSTR( " [steps]" ) );
                    }
                }
                else
                {
                    showInvalidSyntax( pCommand->M );
                }
                break;
            }
            case 3032: // M3032 [S] - configure the fast step size for moving of the heat bed down during the heat bed/work part scan
            {
                if( pCommand->hasS() )
                {
                    // test and take over the specified value
                    nTemp = pCommand->S;
                    if( nTemp < Printer::axisStepsPerMM[Z_AXIS] /20 )   nTemp = Printer::axisStepsPerMM[Z_AXIS] /20;
                    if( nTemp > Printer::axisStepsPerMM[Z_AXIS] *5 )    nTemp = Printer::axisStepsPerMM[Z_AXIS] *5;

                    g_nScanHeatBedDownFastSteps = nTemp;
                    if( Printer::debugInfo() )
                    {
                        Com::printF( PSTR( "M3032: new fast step size for moving of the bed down: " ), (int)g_nScanHeatBedDownFastSteps );
                        Com::printFLN( PSTR( " [steps]" ) );
                    }
                }
                else
                {
                    showInvalidSyntax( pCommand->M );
                }
                break;
            }
            case 3033: // M3033 [S] - configure the slow step size for moving of the heat bed down during the heat bed/work part scan
            {
                if( pCommand->hasS() )
                {
                    // test and take over the specified value
                    nTemp = pCommand->S;
                    if( nTemp < 1 )     nTemp = 1;
                    if( nTemp > 50 )    nTemp = 50;

                    g_nScanHeatBedDownSlowSteps = nTemp;
                    if( Printer::debugInfo() )
                    {
                        Com::printF( PSTR( "M3033: new slow step size for moving of the bed down: " ), (int)g_nScanHeatBedDownSlowSteps );
                        Com::printFLN( PSTR( " [steps]" ) );
                    }
                }
                else
                {
                    showInvalidSyntax( pCommand->M );
                }
                break;
            }
            case 3040: // M3040 [S] - configure the delay (in ms) between two fast movements during the heat bed/work part scan
            {
                if( pCommand->hasS() )
                {
                    // test and take over the specified value
                    nTemp = pCommand->S;
                    if( nTemp < 1 )     nTemp = 1;
                    if( nTemp > 1000 )  nTemp = 1000;

                    g_nScanFastStepDelay = nTemp;
                    if( Printer::debugInfo() )
                    {
                        Com::printF( PSTR( "M3040: new delay between two fast movements: " ), (int)g_nScanFastStepDelay );
                        Com::printFLN( PSTR( " [ms]" ) );
                    }
                }
                else
                {
                    showInvalidSyntax( pCommand->M );
                }
                break;
            }
            case 3041: // M3041 [S] - configure the delay (in ms) between two slow movements during the heat bed/work part scan
            {
                if( pCommand->hasS() )
                {
                    // test and take over the specified value
                    nTemp = pCommand->S;
                    if( nTemp < 1 )     nTemp = 1;
                    if( nTemp > 1000 )  nTemp = 1000;

                    g_nScanSlowStepDelay = nTemp;
                    if( Printer::debugInfo() )
                    {
                        Com::printF( PSTR( "M3041: new delay between two slow movements: " ), (int)g_nScanSlowStepDelay );
                        Com::printFLN( PSTR( " [ms]" ) );
                    }
                }
                else
                {
                    showInvalidSyntax( pCommand->M );
                }
                break;
            }
            case 3042: // M3042 [S] - configure the delay (in ms) between reaching of a new x/y position and the test of the idle pressure
            {
                if( pCommand->hasS() )
                {
                    // test and take over the specified value
                    nTemp = pCommand->S;
                    if( nTemp < 1 )     nTemp = 1;
                    if( nTemp > 10000 ) nTemp = 10000;

                    g_nScanIdleDelay = nTemp;
                    if( Printer::debugInfo() )
                    {
                        Com::printF( PSTR( "M3042: new idle delay: " ), (int)g_nScanIdleDelay );
                        Com::printFLN( PSTR( " [ms]" ) );
                    }
                }
                else
                {
                    showInvalidSyntax( pCommand->M );
                }
                break;
            }
            case 3050: // M3050 [S] - configure the contact pressure delta (in digits)
            {
                if( pCommand->hasS() )
                {
                    // test and take over the specified value
                    nTemp = pCommand->S;
                    if( nTemp < 1 )     nTemp = 1;
                    if( nTemp > 1000 )  nTemp = 1000;

                    g_nScanContactPressureDelta = nTemp;
                    if( Printer::debugInfo() )
                    {
                        Com::printF( PSTR( "M3050: new contact pressure delta: " ), (int)g_nScanContactPressureDelta );
                        Com::printFLN( PSTR( " [digits]" ) );
                    }
                }
                else
                {
                    showInvalidSyntax( pCommand->M );
                }
                break;
            }
            case 3051: // M3051 [S] - configure the retry pressure delta (in digits)
            {
                if( pCommand->hasS() )
                {
                    // test and take over the specified value
                    nTemp = pCommand->S;
                    if( nTemp < 1 )     nTemp = 1;
                    if( nTemp > 1000 )  nTemp = 1000;

                    g_nScanRetryPressureDelta = nTemp;
                    if( Printer::debugInfo() )
                    {
                        Com::printF( PSTR( "M3051: new retry pressure delta: " ), (int)g_nScanRetryPressureDelta );
                        Com::printFLN( PSTR( " [digits]" ) );
                    }
                }
                else
                {
                    showInvalidSyntax( pCommand->M );
                }
                break;
            }
            case 3052: // M3052 [S] - configure the idle pressure tolerance (in digits)
            {
                if( pCommand->hasS() )
                {
                    // test and take over the specified value
                    nTemp = pCommand->S;
                    if( nTemp < 1 )     nTemp = 1;
                    if( nTemp > 1000 )  nTemp = 1000;

                    g_nScanIdlePressureDelta = nTemp;
                    if( Printer::debugInfo() )
                    {
                        Com::printF( PSTR( "M3052: new idle pressure delta: " ), (int)g_nScanIdlePressureDelta );
                        Com::printFLN( PSTR( " [digits]" ) );
                    }
                }
                else
                {
                    showInvalidSyntax( pCommand->M );
                }
                break;
            }
            case 3053: // M3053 [S] - configure the number of A/D converter reads per pressure measurement
            {
                if( pCommand->hasS() )
                {
                    // test and take over the specified value
                    nTemp = pCommand->S;
                    if( nTemp < 1 )     nTemp = 1;
                    if( nTemp > 100 )   nTemp = 100;

                    g_nScanPressureReads = nTemp;
                    if( Printer::debugInfo() )
                    {
                        Com::printF( PSTR( "M3053: new pressure reads per measurement: " ), (int)g_nScanPressureReads );
                    }
                }
                else
                {
                    showInvalidSyntax( pCommand->M );
                }
                break;
            }
            case 3054: // M3054 [S] - configure the delay (in ms) between two A/D converter reads
            {
                if( pCommand->hasS() )
                {
                    // test and take over the specified value
                    nTemp = pCommand->S;
                    if( nTemp < 1 )     nTemp = 1;
                    if( nTemp > 1000 )  nTemp = 1000;

                    g_nScanPressureReadDelay = nTemp;
                    if( Printer::debugInfo() )
                    {
                        Com::printF( PSTR( "M3054: new delay between two pressure reads: " ), (int)g_nScanPressureReadDelay );
                        Com::printFLN( PSTR( " [ms]" ) );
                    }
                }
                else
                {
                    showInvalidSyntax( pCommand->M );
                }
                break;
            }
            case 3055: // M3055 [S] - configure the pressure tolerance (in digits) per pressure measurement
            {
                if( pCommand->hasS() )
                {
                    // test and take over the specified value
                    nTemp = pCommand->S;
                    if( nTemp < 1 )     nTemp = 1;
                    if( nTemp > 1000 )  nTemp = 1000;

                    g_nScanPressureTolerance = nTemp;
                    if( Printer::debugInfo() )
                    {
                        Com::printF( PSTR( "M3055: new scan pressure tolerance: " ), (int)g_nScanPressureTolerance );
                        Com::printFLN( PSTR( " [digits]" ) );
                    }
                }
                else
                {
                    showInvalidSyntax( pCommand->M );
                }
                break;
            }
#endif // FEATURE_HEAT_BED_Z_COMPENSATION || FEATURE_WORK_PART_Z_COMPENSATION

            case 3060:  // M3060 - output the device type and firmware version
            {
                Com::printFLN( PSTR( "Device Type: " ), UI_PRINTER_NAME );
                Com::printFLN( PSTR( "Firmware Version: " ), UI_VERSION_STRING );
                break;
            }

#if FEATURE_PAUSE_PRINTING
            case 3070: // M3070 [S] - pause the print as if the "Pause" button would have been pressed
            {
                if( pCommand->hasS() )
                {
                    // test and take over the specified value
                    nTemp = pCommand->S;
                    if( nTemp < 1 )     nTemp = 1;
                    if( nTemp > 2 )     nTemp = 2;

                    if( nTemp == 1 )
                    {
                        // we shall pause the printing
                        queueTask( TASK_PAUSE_PRINT );
                    }
                    if( nTemp == 2 )
                    {
                        // we shall pause the printing and we shall move away
                        queueTask( TASK_PAUSE_PRINT_AND_MOVE );
                    }
                }
                //else if( pCommand->hasR() ){
                //    continuePrint(); //doesnt work with this version of gcode processing while paused.
                //}
                else
                {
                    queueTask( TASK_PAUSE_PRINT ); 
                }

                break;
            }
            case 3071: // M3071 - wait until the print has been continued via the "Continue" button
            {
                waitUntilContinue();
                break;
            }
#endif // FEATURE_PAUSE_PRINTING

#if FEATURE_EMERGENCY_PAUSE
            case 3075: // M3075 [S] [P] - configure the emergency pause digits
            {
                long    nMin = g_nEmergencyPauseDigitsMin;
                long    nMax = g_nEmergencyPauseDigitsMax;

                if( pCommand->hasS() )
                {
                    // test and take over the specified value - this is our new min value
                    nMin = pCommand->S;
                }
                if( pCommand->hasP() )
                {
                    // test and take over the specified value - this is our new max value
                    nMax = pCommand->P;
                }

                if(nMin < EMERGENCY_PAUSE_DIGITS_MIN) nMin = EMERGENCY_PAUSE_DIGITS_MIN;
                if(nMax > EMERGENCY_PAUSE_DIGITS_MAX) nMax = EMERGENCY_PAUSE_DIGITS_MAX;

                if( nMin == 0 && nMax == 0 )
                {
                    g_nEmergencyPauseDigitsMin = 0;
                    g_nEmergencyPauseDigitsMax = 0;
#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                    HAL::eprSetInt32( EPR_RF_EMERGENCYPAUSEDIGITSMIN, g_nEmergencyPauseDigitsMin );
                    HAL::eprSetInt32( EPR_RF_EMERGENCYPAUSEDIGITSMAX, g_nEmergencyPauseDigitsMax );
                    EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
                    if( Printer::debugInfo() )
                    {
                        Com::printFLN( PSTR( "M3075: the emergency pause has been disabled" ) );
                    }
                }
                else if( nMin < nMax )
                {
                    g_nEmergencyPauseDigitsMin = nMin;
                    g_nEmergencyPauseDigitsMax = nMax;
#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                    HAL::eprSetInt32( EPR_RF_EMERGENCYPAUSEDIGITSMIN, g_nEmergencyPauseDigitsMin );
                    HAL::eprSetInt32( EPR_RF_EMERGENCYPAUSEDIGITSMAX, g_nEmergencyPauseDigitsMax );
                    EEPROM::updateChecksum(); //deshalb die prüfung
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
                    if( Printer::debugInfo() )
                    {
                        Com::printF( PSTR( "M3075: new min: " ), (int)g_nEmergencyPauseDigitsMin );
                        Com::printF( PSTR( " [digits], new max: " ), (int)g_nEmergencyPauseDigitsMax );
                        Com::printFLN( PSTR( " [digits]" ) );
                    }
                }
                else
                {
                    if( Printer::debugErrors() )
                    {
                        Com::printF( PSTR( "M3075: min is not smaller than max (" ), (int)g_nEmergencyPauseDigitsMin );
                        Com::printF( PSTR( "/" ), (int)g_nEmergencyPauseDigitsMax );
                        Com::printFLN( PSTR( " [digits])" ) );
                    }
                }

                break;
            }
#endif // FEATURE_EMERGENCY_PAUSE

#if FEATURE_OUTPUT_FINISHED_OBJECT
            case 3079: // M3079 - output the printed object
            {
                outputObject();
                break;
            }
#endif // FEATURE_OUTPUT_FINISHED_OBJECT

#if FEATURE_PARK
            case 3080: // M3080 - park the printer
            {
                parkPrinter();
                break;
            }
#endif // FEATURE_PARK

#if FEATURE_WATCHDOG
            case 3090: // M3090 - test the watchdog (this command resets the firmware)
            {
                if( pCommand->hasS() )
                {
                   if(pCommand->S == 666){
                     if(g_bPingWatchdog){ 
                      HAL::stopWatchdog(); 
                      Com::printFLN( PSTR( "M3090: WARNING the watchdog is disabled now" ) );
                     }
                     else {
                      HAL::startWatchdog(); 
                      Com::printFLN( PSTR( "M3090: the watchdog has been activated" ) );
                     }
                   }
                }
                else if(pCommand->hasP()){
                    HAL::delayMilliseconds(10000);
                } 
                else if(pCommand->hasT()){
                    for(int iii = 0; iii < 1000; iii++) HAL::delayMicroseconds(10000);
                } 
                else
                {
                    if( Printer::debugInfo() )
                    {
                      Com::printFLN( PSTR( "M3090: the watchdog is going to reset the firmware" ) );
                    }
                    HAL::delayMilliseconds( 100 );
                    HAL::testWatchdog();
                }
                break;
            }
#endif // FEATURE_WATCHDOG

            case 3091: // M3091 - erase the external EEPROM
            {
                clearExternalEEPROM();
                break;
            }

#if FEATURE_EXTENDED_BUTTONS
            case 3100: // M3100 [S] - configure the number of manual z steps after the "Z up" or "Z down" button has been pressed
            {
                if( pCommand->hasS() )
                {
                    // test and take over the specified value
                    nTemp = pCommand->S;
                    if( nTemp < 1 )                         nTemp = 1;
                    if( nTemp > MAXIMAL_MANUAL_STEPS_Z )    nTemp = MAXIMAL_MANUAL_STEPS_Z;

                    g_nManualSteps[Z_AXIS] = nTemp;
#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                    unsigned long oldval = HAL::eprGetInt32(EPR_RF_MOD_Z_STEP_SIZE);
                    if(oldval != g_nManualSteps[Z_AXIS]){
                        HAL::eprSetInt32( EPR_RF_MOD_Z_STEP_SIZE, g_nManualSteps[Z_AXIS] );
                        EEPROM::updateChecksum(); //deshalb die prüfung
                    }
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
                    if( Printer::debugInfo() )
                    {
                        Com::printF( PSTR( "M3100: new manual z steps: " ), (int)g_nManualSteps[Z_AXIS] );
                        Com::printFLN( PSTR( " [steps]" ) );
                    }
                }
                else
                {
                    showInvalidSyntax( pCommand->M );
                }
                break;
            }
            case 3101: // M3101 [S] - configure the number of manual extruder steps after the "Extruder up" or "Extruder down" button has been pressed
            {
                if( isSupportedMCommand( pCommand->M, OPERATING_MODE_PRINT ) )
                {
                    if( pCommand->hasS() )
                    {
                        // test and take over the specified value
                        nTemp = pCommand->S;
                        if( nTemp < 1 )                         nTemp = 1;
                        if( nTemp > MAXIMAL_MANUAL_STEPS_E )    nTemp = MAXIMAL_MANUAL_STEPS_E *10;

                        g_nManualSteps[E_AXIS] = nTemp;
                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3101: new manual extruder steps: " ), (int)g_nManualSteps[E_AXIS] );
                            Com::printFLN( PSTR( " [steps]" ) );
                        }
                    }
                    else
                    {
                        showInvalidSyntax( pCommand->M );
                    }
                }
                break;
            }
#endif // FEATURE_EXTENDED_BUTTONS

#if FEATURE_PAUSE_PRINTING
            case 3102: // M3102 [X] [Y] [Z] [E] - configure the offset in x, y, z and e direction which shall be applied in case the "Pause" button has been pressed ( units are [steps] )
            {
                if( pCommand->hasNoXYZ() && !pCommand->hasE() )
                {
                    showInvalidSyntax( pCommand->M );
                }
                else
                {
                    if( pCommand->hasX() )
                    {
                        // test and take over the specified value
                        nTemp = pCommand->X;
                        if( nTemp < -(Printer::axisStepsPerMM[X_AXIS] * Printer::lengthMM[X_AXIS]) )    nTemp = -(Printer::axisStepsPerMM[X_AXIS] * Printer::lengthMM[X_AXIS]);
                        if( nTemp > (Printer::axisStepsPerMM[X_AXIS] * Printer::lengthMM[X_AXIS]) )     nTemp = (Printer::axisStepsPerMM[X_AXIS] * Printer::lengthMM[X_AXIS]);

                        g_nPauseSteps[X_AXIS] = nTemp;
                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3102: new x pause offset: " ), g_nPauseSteps[X_AXIS] );
                            Com::printFLN( PSTR( " [steps]" ) );
                        }
                    }
                    if( pCommand->hasY() )
                    {
                        // test and take over the specified value
                        nTemp = pCommand->Y;
                        if( nTemp < -(Printer::axisStepsPerMM[Y_AXIS] * Printer::lengthMM[Y_AXIS]) )    nTemp = -(Printer::axisStepsPerMM[Y_AXIS] * Printer::lengthMM[Y_AXIS]);
                        if( nTemp > (Printer::axisStepsPerMM[Y_AXIS] * Printer::lengthMM[Y_AXIS]) )     nTemp = (Printer::axisStepsPerMM[Y_AXIS] * Printer::lengthMM[Y_AXIS]);

                        g_nPauseSteps[Y_AXIS] = nTemp;
                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3102: new y pause offset: " ), g_nPauseSteps[Y_AXIS] );
                            Com::printFLN( PSTR( " [steps]" ) );
                        }
                    }
                    if( pCommand->hasZ() )
                    {
                        // test and take over the specified value
                        nTemp = pCommand->Z;
                        if( nTemp < 0 )                                                                 nTemp = 0;
                        if( nTemp > (Printer::axisStepsPerMM[Z_AXIS] * Printer::lengthMM[Z_AXIS]) )     nTemp = (Printer::axisStepsPerMM[Z_AXIS] * Printer::lengthMM[Z_AXIS]);

                        g_nPauseSteps[Z_AXIS] = nTemp;
                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3102: new z pause offset: " ), g_nPauseSteps[Z_AXIS] );
                            Com::printFLN( PSTR( " [steps]" ) );
                        }
                    }
                    if( pCommand->hasE() )
                    {
                        // test and take over the specified value
                        nTemp = pCommand->E;
                        if( nTemp < 0 )                             nTemp = 0;
                        if( nTemp > (EXT0_STEPS_PER_MM *5) )        nTemp = EXT0_STEPS_PER_MM *5;

                        g_nPauseSteps[E_AXIS] = nTemp;
                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3102: new extruder pause offset: " ), g_nPauseSteps[E_AXIS] );
                            Com::printFLN( PSTR( " [steps]" ) );
                        }
                    }
                }
                break;
            }
            case 3105: // M3105 [X] [Y] [Z] [E] - configure the offset in x, y, z and e direction which shall be applied in case the "Pause" button has been pressed ( units are [mm] )
            {
                if( pCommand->hasNoXYZ() && !pCommand->hasE() )
                {
                    showInvalidSyntax( pCommand->M );
                }
                else
                {
                    if( pCommand->hasX() )
                    {
                        // test and take over the specified value
                        nTemp = pCommand->X;
                        if( nTemp < -Printer::lengthMM[X_AXIS] )    nTemp = -Printer::lengthMM[X_AXIS];
                        if( nTemp > Printer::lengthMM[X_AXIS] )     nTemp = Printer::lengthMM[X_AXIS];

                        g_nPauseSteps[X_AXIS] = (long)((float)nTemp * Printer::axisStepsPerMM[X_AXIS]);
                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3105: new x pause offset: " ), nTemp );
                            Com::printFLN( PSTR( " [mm]" ) );
                        }
                    }
                    if( pCommand->hasY() )
                    {
                        // test and take over the specified value
                        nTemp = pCommand->Y;
                        if( nTemp < -Y_MAX_LENGTH )     nTemp = -Y_MAX_LENGTH;
                        if( nTemp > Y_MAX_LENGTH )      nTemp = Y_MAX_LENGTH;

                        g_nPauseSteps[Y_AXIS] = (long)((float)nTemp * Printer::axisStepsPerMM[Y_AXIS]);
                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3105: new y pause offset: " ), nTemp );
                            Com::printFLN( PSTR( " [mm]" ) );
                        }
                    }
                    if( pCommand->hasZ() )
                    {
                        // test and take over the specified value
                        nTemp = pCommand->Z;
                        if( nTemp < 0 )                 nTemp = 0;
                        if( nTemp > Z_MAX_LENGTH )      nTemp = Z_MAX_LENGTH;

                        g_nPauseSteps[Z_AXIS] = (long)((float)nTemp * Printer::axisStepsPerMM[Z_AXIS]);
                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3105: new z pause offset: " ), nTemp );
                            Com::printFLN( PSTR( " [mm]" ) );
                        }
                    }
                    if( pCommand->hasE() )
                    {
                        // test and take over the specified value
                        nTemp = pCommand->E;
                        if( nTemp < 0 )     nTemp = 0;
                        if( nTemp > 5 )     nTemp = 5;

                        g_nPauseSteps[E_AXIS] = (long)((float)nTemp * EXT0_STEPS_PER_MM);
                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3105: new extruder pause offset: " ), nTemp );
                            Com::printFLN( PSTR( " [mm]" ) );
                        }
                    }
                }
                break;
            }
#endif // FEATURE_PAUSE_PRINTING

#if FEATURE_PARK
            case 3103: // M3103 [X] [Y] [Z] - configure the x, y and z position which shall set when the printer is parked ( units are [mm] )
            {
                if( pCommand->hasNoXYZ() )
                {
                    showInvalidSyntax( pCommand->M );
                }
                else
                {
                    if( pCommand->hasX() )
                    {
                        // test and take over the specified value
                        nTemp = pCommand->X;
                        if( nTemp < 0 )                         nTemp = 0;
                        if( nTemp > Printer::lengthMM[X_AXIS] ) nTemp = Printer::lengthMM[X_AXIS];

                        g_nParkPosition[X_AXIS] = nTemp;
                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3103: new x park position: " ), g_nParkPosition[X_AXIS] );
                            Com::printFLN( PSTR( " [mm]" ) );
                        }
                    }
                    if( pCommand->hasY() )
                    {
                        // test and take over the specified value
                        nTemp = pCommand->Y;
                        if( nTemp < 0 )             nTemp = 0;
                        if( nTemp > Y_MAX_LENGTH )  nTemp = Y_MAX_LENGTH;

                        g_nParkPosition[Y_AXIS] = nTemp;
                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3103: new y park position: " ), g_nParkPosition[Y_AXIS] );
                            Com::printFLN( PSTR( " [mm]" ) );
                        }
                    }
                    if( pCommand->hasZ() )
                    {
                        // test and take over the specified value
                        nTemp = pCommand->Z;
                        if( nTemp < 0 )             nTemp = 0;
                        if( nTemp > Z_MAX_LENGTH )  nTemp = Z_MAX_LENGTH;

                        g_nParkPosition[Z_AXIS] = nTemp;
                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3103: new z park position: " ), g_nParkPosition[Z_AXIS] );
                            Com::printFLN( PSTR( " [mm]" ) );
                        }
                    }
                }
                break;
            }
#endif // FEATURE_PARK

            case 3115:  // M3115 - set the x/y origin to the current x/y position
            {
                Printer::setOrigin(-Printer::queuePositionLastMM[X_AXIS],-Printer::queuePositionLastMM[Y_AXIS],Printer::originOffsetMM[Z_AXIS]);
                break;
            }
            case 3117:  // M3117 - set a status text which is not overwritten by M117
            {
                if( pCommand->hasString() )
                {
                    if( !uid.locked )
                    {
                        // ensure that the current text won't be overwritten
                        uid.lock();

                        if( Printer::debugInfo() )
                        {
                            Com::printFLN( PSTR( "M3117: lock" ) );
                        }
                    }

                    uid.setStatus( pCommand->text, false, true );
                    uid.refreshPage();
                }
                else
                {
                    if( uid.locked )
                    {
                        // allow to overwrite the current string again
                        uid.unlock();
                        if( Printer::debugInfo() )
                        {
                            Com::printFLN( PSTR( "M3117: unlock" ) );
                        }

                        g_uStartOfIdle = HAL::timeInMilliseconds();
                    }
                }
                break;
            }

#if FEATURE_CASE_FAN && !CASE_FAN_ALWAYS_ON
            case 3120:  // M3120 - turn on the case fan
            {
                //disable fan-temp-ignore to original state // Nibbels
                Printer::ignoreFanOn = false;
                
                // enable the case fan
                Printer::prepareFanOff = 0;
                WRITE( CASE_FAN_PIN, 1 );

                if( Printer::debugInfo() )
                {
                    Com::printFLN( PSTR( "M3120: fan on" ) );
                }
                break;
            }

            case 3121:  // M3121 - turn off the case fan
            {
                // disable the case fan
                if( pCommand->hasS() )
                {
                    // we shall set a new case fan off delay
                    Printer::fanOffDelay =  pCommand->S;
                    Printer::fanOffDelay *= 1000;   // convert from [s] to [ms]
                }

                if( pCommand->hasP() )
                {
                    // the fan should be disabled even if the print starts // temp > fan on temp // Nibbels
                    Printer::ignoreFanOn = true;
                }
                
                if( Printer::fanOffDelay )
                {
                    // we are going to disable the case fan after the delay
                    Printer::prepareFanOff = HAL::timeInMilliseconds();

                    if( Printer::debugInfo() )
                    {
                        Com::printF( PSTR( "M3121: fan off in " ), pCommand->S );
                        Com::printFLN( PSTR( " [s]" ) );
                    }
                }
                else
                {
                    // we are going to disable the case fan now
                    Printer::prepareFanOff = 0;
                    WRITE(CASE_FAN_PIN, 0);

                    if( Printer::debugInfo() )
                    {
                        Com::printFLN( PSTR( "M3121: fan off" ) );
                    }
                }
                break;
            }
#endif // FEATURE_CASE_FAN && !CASE_FAN_ALWAYS_ON

#if FEATURE_FIND_Z_ORIGIN
            case 3130: // M3130 - start/stop the search of the z-origin
            {
                startFindZOrigin();
                break;
            }
#endif // FEATURE_FIND_Z_ORIGIN

#if FEATURE_WORK_PART_Z_COMPENSATION
            case 3140: // M3140 - turn the z-compensation off
            {
                if( isSupportedMCommand( pCommand->M, OPERATING_MODE_MILL ) )
                {
                    if( Printer::debugInfo() )
                    {
                        Com::printFLN( PSTR( "M3140: disabling z compensation" ) );
                    }
                    queueTask( TASK_DISABLE_Z_COMPENSATION );
                }
                break;
            }
            case 3141: // M3141 - turn the z-compensation on
            {
                if( isSupportedMCommand( pCommand->M, OPERATING_MODE_MILL ) )
                {
                    if( Printer::doWorkPartZCompensation )
                    {
                        if( Printer::debugInfo() )
                        {
                            Com::printFLN( PSTR( "M3141: the z compensation is enabled already" ) );
                        }
                        break;
                    }

                    if( Printer::areAxisHomed() )
                    {
                        if( g_ZCompensationMatrix[0][0] != EEPROM_FORMAT )
                        {
                            // we load the z compensation matrix before its first usage because this can take some time
                            prepareZCompensation();
                        }

                        if( g_ZCompensationMatrix[0][0] == EEPROM_FORMAT )
                        {
                            // enable the z compensation only in case we have valid compensation values
                            if( Printer::debugInfo() )
                            {
                                Com::printFLN( PSTR( "M3141: enabling z compensation" ) );
                            }

/*                          if( g_nZOriginPosition[X_AXIS] && g_nZOriginPosition[Y_AXIS] )
                            {
                                Com::printF( PSTR( "g_nZOriginPosition[X_AXIS] = " ), g_nZOriginPosition[X_AXIS] );
                                Com::printFLN( PSTR( ", g_nZOriginPosition[Y_AXIS] = " ), g_nZOriginPosition[Y_AXIS] );
                            }
*/                          queueTask( TASK_ENABLE_Z_COMPENSATION );
                        }
                        else
                        {
                            if( Printer::debugErrors() )
                            {
                                Com::printF( PSTR( "M3141: the z compensation can not be enabled because the work part compensation matrix is not valid ( " ), g_ZCompensationMatrix[0][0] );
                                Com::printF( PSTR( " / " ), EEPROM_FORMAT );
                                Com::printFLN( PSTR( " )" ) );
                            }

                            showError( (void*)ui_text_z_compensation, (void*)ui_text_invalid_matrix );
                        }
                    }
                    else
                    {
                        if( Printer::debugErrors() )
                        {
                            Com::printFLN( PSTR( "M3141: the z compensation can not be enabled because the home position is unknown" ) );
                        }

                        showError( (void*)ui_text_z_compensation, (void*)ui_text_home_unknown );
                    }
                }
                break;
            }

            case 3146: // M3146 [S] [Z] - configure the static z-offset ( S - units are [um], Z - units are [mm] )
            {
                if( isSupportedMCommand( pCommand->M, OPERATING_MODE_MILL ) )
                {
                    if( pCommand->hasZ() )
                    {
                        // test and take over the specified value
                        nTemp = pCommand->Z;

                        if( nTemp < -WORK_PART_MAX_STATIC_Z_OFFSET_MM )     nTemp = -WORK_PART_MAX_STATIC_Z_OFFSET_MM;
                        if( nTemp > WORK_PART_MAX_STATIC_Z_OFFSET_MM )      nTemp = WORK_PART_MAX_STATIC_Z_OFFSET_MM;

                        g_staticZSteps = (nTemp * Printer::axisStepsPerMM[Z_AXIS]);
                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3146: new static z-offset: " ), nTemp );
                            Com::printF( PSTR( " [mm]" ) );
                            Com::printF( PSTR( " / " ), g_staticZSteps );
                            Com::printFLN( PSTR( " [steps]" ) );
                        }
                    }
                    else if( pCommand->hasS() )
                    {
                        // test and take over the specified value
                        nTemp = pCommand->S;

                        if( nTemp < -(WORK_PART_MAX_STATIC_Z_OFFSET_MM * 1000) )    nTemp = -(WORK_PART_MAX_STATIC_Z_OFFSET_MM * 1000);
                        if( nTemp > (WORK_PART_MAX_STATIC_Z_OFFSET_MM * 1000) )     nTemp = (WORK_PART_MAX_STATIC_Z_OFFSET_MM * 1000);

                        g_staticZSteps = (nTemp * Printer::axisStepsPerMM[Z_AXIS]) / 1000;
                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3146: new static z-offset: " ), nTemp );
                            Com::printF( PSTR( " [um]" ) );
                            Com::printF( PSTR( " / " ), g_staticZSteps );
                            Com::printFLN( PSTR( " [steps]" ) );
                        }
                    }
                    else
                    {
                        showInvalidSyntax( pCommand->M );
                    }
                }
                break;
            }

            case 3149: // M3149 [S] - get/choose the active work part z-compensation matrix
            {
                if( isSupportedMCommand( pCommand->M, OPERATING_MODE_MILL ) )
                {
                    if( Printer::doWorkPartZCompensation )
                    {
                        // do not allow to change the current work part z-compensation matrix while the z-compensation is active
                        if( Printer::debugErrors() )
                        {
                            Com::printFLN( PSTR( "M3149: the work part z matrix can not be changed while the z-compensation is active" ) );
                        }

                        showError( (void*)ui_text_z_compensation, (void*)ui_text_operation_denied );
                        break;
                    }

                    if( pCommand->hasS() )
                    {
                        nTemp = pCommand->S;

                        if( nTemp < 1 || nTemp > EEPROM_MAX_WORK_PART_SECTORS )
                        {
                            if( Printer::debugErrors() )
                            {
                                Com::printF( PSTR( "M3149: invalid work part (" ), nTemp );
                                Com::printFLN( PSTR( ")" ) );
                            }
                            break;
                        }

                        if( g_nActiveWorkPart != nTemp )
                        {
                            // we have to switch to another work part
                            switchActiveWorkPart( (char)nTemp );
                        }
                    }

                    if( Printer::debugInfo() )
                    {
                        Com::printFLN( PSTR( "M3149: currently active work part: " ), g_nActiveWorkPart );
                    }
                }
                break;
            }
            case 3150: // M3150 - start/abort the work part scan
            {
                if( isSupportedMCommand( pCommand->M, OPERATING_MODE_MILL ) )
                {
                    if( pCommand->hasS() )
                    {
                        nTemp = pCommand->S;
                    }
                    else
                    {
                        nTemp = 0;
                    }

                    startWorkPartScan( (char)nTemp );
                }
                break;
            }
            case 3151: // M3151 [S] - clear the specified z-compensation matrix from the EEPROM
            {
                if( isSupportedMCommand( pCommand->M, OPERATING_MODE_MILL ) )
                {
                    if( pCommand->hasS() )
                    {
                        nTemp = pCommand->S;
                    }
                    else
                    {
                        // we clear the current z-compensation matrix in case no other z-compensation matrix is specified
                        nTemp = g_nActiveWorkPart;
                    }

                    if( nTemp < 1 || nTemp > EEPROM_MAX_WORK_PART_SECTORS )
                    {
                        if( Printer::debugErrors() )
                        {
                            Com::printF( PSTR( "M3151: invalid work part (" ), nTemp );
                            Com::printFLN( PSTR( ")" ) );
                        }
                        break;
                    }

                    // switch to the specified work part
                    g_nActiveWorkPart = (char)nTemp;
                    writeWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, EEPROM_OFFSET_ACTIVE_WORK_PART_Z_MATRIX, g_nActiveWorkPart );
                    clearCompensationMatrix( (EEPROM_SECTOR_SIZE *9) + (unsigned int)(EEPROM_SECTOR_SIZE * g_nActiveWorkPart) );

                    if( Printer::debugInfo() )
                    {
                        Com::printFLN( PSTR( "M3151: cleared z-compensation matrix: " ), nTemp );
                    }

                    // TODO: in case the z-compensation is active at the moment, this command should not work
                }
                break;
            }
            case 3152: // M3152 - restore the default scan parameters
            {
                if( isSupportedMCommand( pCommand->M, OPERATING_MODE_MILL ) )
                {
                    restoreDefaultScanParameters();
                }
                break;
            }
            case 3153: // M3153 [S] [P] - output the specified work part z-compensation matrix
            {
                if( isSupportedMCommand( pCommand->M, OPERATING_MODE_MILL ) )
                {
                    char    format;


                    if( pCommand->hasS() )
                    {
                        nTemp = pCommand->S;
                    }
                    else
                    {
                        // we output the current z-compensation matrix in case no other z-compensation matrix is specified
                        nTemp = g_nActiveWorkPart;
                    }

                    if( pCommand->hasP() )
                    {
                        format = pCommand->P ? 1 : 0;
                    }
                    else
                    {
                        format = 0;
                    }

                    if( nTemp < 0 || nTemp > EEPROM_MAX_WORK_PART_SECTORS )
                    {
                        if( Printer::debugErrors() )
                        {
                            Com::printF( PSTR( "M3153: invalid work part (" ), nTemp );
                            Com::printFLN( PSTR( ")" ) );
                        }
                        break;
                    }

                    if( g_nActiveWorkPart != nTemp )
                    {
                        // we have to switch to another work part
                        switchActiveWorkPart( (char)nTemp );
                    }

                    if( g_ZCompensationMatrix[0][0] != EEPROM_FORMAT )
                    {
                        // we load the z compensation matrix before its first usage because this can take some time
                        prepareZCompensation();
                    }

                    if( g_ZCompensationMatrix[0][0] == EEPROM_FORMAT )
                    {
                        if( Printer::debugInfo() )
                        {
                            Com::printFLN( PSTR( "M3153: current work part compensation matrix: " ) );
                        }
                        outputCompensationMatrix( format );
                    }
                    else
                    {
                        if( Printer::debugErrors() )
                        {
                            Com::printF( PSTR( "M3153: the work part compensation matrix is not valid ( " ), g_ZCompensationMatrix[0][0] );
                            Com::printF( PSTR( " / " ), EEPROM_FORMAT );
                            Com::printFLN( PSTR( " )" ) );
                        }

                        showError( (void*)ui_text_z_compensation, (void*)ui_text_invalid_matrix );
                    }
                }
                break;
            }
            case 3160: // M3160 [S] - configure the x start position for the work part scan
            {
                if( isSupportedMCommand( pCommand->M, OPERATING_MODE_MILL ) )
                {
                    if( pCommand->hasS() )
                    {
                        // test and take over the specified value
                        nTemp = pCommand->S;
                        if( nTemp < 5 )                                 nTemp = 5;
                        if( nTemp > (Printer::lengthMM[X_AXIS] -5) )    nTemp = Printer::lengthMM[X_AXIS] -5;

                        g_nScanXStartSteps = (long)((float)nTemp * Printer::axisStepsPerMM[X_AXIS]);
                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3160: new x start position: " ), nTemp );
                            Com::printF( PSTR( " [mm], " ), (int)g_nScanXStartSteps );
                            Com::printFLN( PSTR( " [steps]" ) );
                        }               
                    }
                    else
                    {
                        showInvalidSyntax( pCommand->M );
                    }
                }
                break;
            }
            case 3161: // M3161 [S] - configure the y start position for the work part scan
            {
                if( isSupportedMCommand( pCommand->M, OPERATING_MODE_MILL ) )
                {
                    if( pCommand->hasS() )
                    {
                        // test and take over the specified value
                        nTemp = pCommand->S;
                        if( nTemp < 5 )                     nTemp = 5;
                        if( nTemp > (Y_MAX_LENGTH -5 ) )    nTemp = Y_MAX_LENGTH -5;

                        g_nScanYStartSteps = (long)((float)nTemp * Printer::axisStepsPerMM[Y_AXIS]);
                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3161: new y start position: " ), nTemp );
                            Com::printF( PSTR( " [mm], " ), (int)g_nScanYStartSteps );
                            Com::printFLN( PSTR( " [steps]" ) );
                        }
                    }
                    else
                    {
                        showInvalidSyntax( pCommand->M );
                    }
                }
                break;
            }
            case 3162: // M3162 [S] - configure the x step size for the work part scan
            {
                if( isSupportedMCommand( pCommand->M, OPERATING_MODE_MILL ) )
                {
                    if( pCommand->hasS() )
                    {
                        // test and take over the specified value
                        nTemp = pCommand->S;
                        if( nTemp < WORK_PART_SCAN_X_STEP_SIZE_MIN_MM ) nTemp = WORK_PART_SCAN_X_STEP_SIZE_MIN_MM;
                        if( nTemp > 100 )                               nTemp = 100;

                        g_nScanXStepSizeMm    = nTemp;
                        g_nScanXStepSizeSteps = (long)((float)nTemp * Printer::axisStepsPerMM[X_AXIS]);
                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3162: new x step size: " ), (int)g_nScanXStepSizeMm );
                            Com::printF( PSTR( " [mm], " ), (int)g_nScanXStepSizeSteps );
                            Com::printFLN( PSTR( " [steps]" ) );
                        }
                    }
                    else
                    {
                        showInvalidSyntax( pCommand->M );
                    }
                }
                break;
            }
            case 3163: // M3163 [S] - configure the y step size for the work part scan
            {
                if( isSupportedMCommand( pCommand->M, OPERATING_MODE_MILL ) )
                {
                    if( pCommand->hasS() )
                    {
                        // test and take over the specified value
                        nTemp = pCommand->S;
                        if( nTemp < WORK_PART_SCAN_Y_STEP_SIZE_MIN_MM ) nTemp = WORK_PART_SCAN_Y_STEP_SIZE_MIN_MM;
                        if( nTemp > 100 )                               nTemp = 100;

                        g_nScanYStepSizeMm    = nTemp;
                        g_nScanYStepSizeSteps = (long)((float)nTemp * Printer::axisStepsPerMM[Y_AXIS]);
                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3163: new y step size: " ), (int)g_nScanYStepSizeMm );
                            Com::printF( PSTR( " [mm], " ), (int)g_nScanYStepSizeSteps );
                            Com::printFLN( PSTR( " [steps]" ) );
                        }
                    }
                    else
                    {
                        showInvalidSyntax( pCommand->M );
                    }
                }
                break;
            }
            case 3164: // M3164 [S] - configure the x end position for the work part scan
            {
                if( isSupportedMCommand( pCommand->M, OPERATING_MODE_MILL ) )
                {
                    if( pCommand->hasS() )
                    {
                        // test and take over the specified value
                        nTemp = pCommand->S;
                        if( nTemp < 5 )                                 nTemp = 5;
                        if( nTemp > (Printer::lengthMM[X_AXIS] -5) )    nTemp = Printer::lengthMM[X_AXIS] -5;

                        g_nScanXEndSteps = (long)((float)nTemp * Printer::axisStepsPerMM[X_AXIS]);
                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3164: new x end position: " ), nTemp );
                            Com::printF( PSTR( " [mm], " ), (int)g_nScanXEndSteps );
                            Com::printFLN( PSTR( " [steps]" ) );
                        }

                        g_nScanXMaxPositionSteps = long(Printer::lengthMM[X_AXIS] * Printer::axisStepsPerMM[X_AXIS] - g_nScanXEndSteps);
                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3164: new x max position: " ), (int)g_nScanXMaxPositionSteps );
                            Com::printFLN( PSTR( " [steps]" ) );
                        }
                    }
                    else
                    {
                        showInvalidSyntax( pCommand->M );
                    }
                }
                break;
            }
            case 3165: // M3165 [S] - configure the y end position for the work part scan
            {
                if( isSupportedMCommand( pCommand->M, OPERATING_MODE_MILL ) )
                {
                    if( pCommand->hasS() )
                    {
                        // test and take over the specified value
                        nTemp = pCommand->S;
                        if( nTemp < 5 )                     nTemp = 5;
                        if( nTemp > (Y_MAX_LENGTH -5 ) )    nTemp = Y_MAX_LENGTH -5;

                        g_nScanYEndSteps = (long)((float)nTemp * Printer::axisStepsPerMM[Y_AXIS]);
                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3165: new y end position: " ), nTemp );
                            Com::printF( PSTR( " [mm], " ), (int)g_nScanYEndSteps );
                            Com::printFLN( PSTR( " [steps]" ) );
                        }

                        g_nScanYMaxPositionSteps = long(Y_MAX_LENGTH * Printer::axisStepsPerMM[Y_AXIS] - g_nScanYEndSteps);
                        if( Printer::debugInfo() )
                        {
                            Com::printF( PSTR( "M3165: new y max position: " ), (int)g_nScanYMaxPositionSteps );
                            Com::printFLN( PSTR( " [steps]" ) );
                        }
                    }
                    else
                    {
                        showInvalidSyntax( pCommand->M );
                    }
                }
                break;
            }
#endif // FEATURE_WORK_PART_Z_COMPENSATION

            case 3200: // M3200 - reserved for test and debug
            {
                if( pCommand->hasP() )
                {
                    switch( pCommand->P )
                    {
                        case 1:
                        {
                            Commands::checkFreeMemory();
                            Commands::writeLowestFreeRAM();
                            Com::printFLN( PSTR( "lowest free RAM: " ), Commands::lowestRAMValue );

#if FEATURE_HEAT_BED_Z_COMPENSATION || FEATURE_WORK_PART_Z_COMPENSATION
                            Com::printFLN( PSTR( "z-compensation matrix x: " ), COMPENSATION_MATRIX_MAX_X );
                            Com::printFLN( PSTR( "z-compensation matrix y: " ), COMPENSATION_MATRIX_MAX_Y );
#endif // FEATURE_HEAT_BED_Z_COMPENSATION || FEATURE_WORK_PART_Z_COMPENSATION
                            break;
                        }
                        case 2:
                        {
                            Com::printFLN( PSTR( "Homing feedrates:" ) );
                            Com::printF( PSTR( "x = " ), Printer::homingFeedrate[X_AXIS] );
                            Com::printF( PSTR( ", y = " ), Printer::homingFeedrate[Y_AXIS] );
                            Com::printFLN( PSTR( ", z = " ), Printer::homingFeedrate[Z_AXIS] );
                            break;
                        }
                        case 3:
                        {
                            if( pCommand->hasS() )
                            {
                                switch( pCommand->S )
                                {
                                    case  1:    BEEP_SHORT                  break;
                                    case  2:    BEEP_LONG                   break;
                                    case  3:    BEEP_START_PRINTING         break;
                                    case  4:    BEEP_ABORT_PRINTING         break;
                                    case  5:    BEEP_STOP_PRINTING          break;
                                    case  6:    BEEP_PAUSE                  break;
                                    case  7:    BEEP_CONTINUE               break;
                                    case  8:    BEEP_START_HEAT_BED_SCAN    break;
                                    case  9:    BEEP_ABORT_HEAT_BED_SCAN    break;
                                    case 10:    BEEP_STOP_HEAT_BED_SCAN     break;
                                    case 11:    BEEP_SERVICE_INTERVALL      break;
                                    case 12:    BEEP_ALIGN_EXTRUDERS        break;
                                    case 13:    BEEP_WRONG_FIRMWARE         break;
                                }
                            }
                            break;
                        }
                        case 4:
                        {
                            // simulate blocking of all axes
                            Com::printFLN( PSTR( "M3200: block all" ) );

                            doEmergencyStop( STOP_BECAUSE_OF_Z_BLOCK );
                            break;
                        }
                        case 5:
                        {
                            // simulate a temp sensor error
                            Com::printFLN( PSTR( "M3200: simulating a defect temperature sensor" ) );
                            Printer::flag0 |= PRINTER_FLAG0_TEMPSENSOR_DEFECT;
                            reportTempsensorError();
                            break;
                        }
                        case 6:
                        {
                            Com::printF( PSTR( "nCPS X;" ),   Printer::queuePositionCurrentSteps[X_AXIS] );
                            Com::printF( Com::tSemiColon,         Printer::queuePositionCurrentSteps[X_AXIS] / Printer::axisStepsPerMM[X_AXIS] );
                            Com::printF( PSTR( "; nCPS Y;" ), Printer::queuePositionCurrentSteps[Y_AXIS] );
                            Com::printFLN( Com::tSemiColon,       Printer::queuePositionCurrentSteps[Y_AXIS] / Printer::axisStepsPerMM[Y_AXIS] );

                            Com::printF( PSTR( "; nCPS Z;" ), Printer::queuePositionCurrentSteps[Z_AXIS] );
                            Com::printF( Com::tSemiColon,         Printer::queuePositionCurrentSteps[Z_AXIS] / Printer::axisStepsPerMM[Z_AXIS] );
                            Com::printF( PSTR( "; qTS;" ),    Printer::queuePositionTargetSteps[Z_AXIS] );
                            Com::printFLN( PSTR( "; qLS;" ),  Printer::queuePositionLastSteps[Z_AXIS] );
							
#if FEATURE_HEAT_BED_Z_COMPENSATION || FEATURE_WORK_PART_Z_COMPENSATION
                            Com::printF( PSTR( "; zTZ;" ), Printer::compensatedPositionTargetStepsZ );
                            Com::printFLN( PSTR( "; zCZ;" ), Printer::compensatedPositionCurrentStepsZ );
#endif // FEATURE_HEAT_BED_Z_COMPENSATION || FEATURE_WORK_PART_Z_COMPENSATION

#if FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING
                            Com::printF( PSTR( "; direct TZ;" ), Printer::directPositionTargetSteps[Z_AXIS] );
                            Com::printFLN( PSTR( "; CZ;" ), Printer::directPositionCurrentSteps[Z_AXIS] );
#endif // FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING

//                          Com::printFLN( PSTR( "; Int32;" ), g_debugInt32 );

                            Com::printF( PSTR( "; SDX;" ), Printer::stepperDirection[X_AXIS] );
                            Com::printF( PSTR( "; SDY;" ), Printer::stepperDirection[Y_AXIS] );
                            Com::printFLN( PSTR( "; SDZ;" ), Printer::stepperDirection[Z_AXIS] );


                            #if FEATURE_HEAT_BED_Z_COMPENSATION && FEATURE_WORK_PART_Z_COMPENSATION
								if( !Printer::doHeatBedZCompensation && !Printer::doWorkPartZCompensation )
								{
									Com::printFLN( PSTR( "; return 1;" ) );
								}
                            #elif FEATURE_HEAT_BED_Z_COMPENSATION
								if( !Printer::doHeatBedZCompensation )
								{
									Com::printFLN( PSTR( "; return heatbedz;" ) );
								}
                            #elif FEATURE_WORK_PART_Z_COMPENSATION
								if( !Printer::doWorkPartZCompensation )
								{
									Com::printFLN( PSTR( "; return workpart;" ) );
								}
                            #endif // FEATURE_HEAT_BED_Z_COMPENSATION && FEATURE_WORK_PART_Z_COMPENSATION

								if( Printer::blockAll )
								{
									// do not perform any compensation in case the moving is blocked
									Com::printFLN( PSTR( "; return block;" ) );
								}
								if( PrintLine::direct.isZMove() )
								{
									// do not perform any compensation in case the moving is blocked
									Com::printFLN( PSTR( "; return directZ;" ), PrintLine::direct.stepsRemaining );
								}
								if( PrintLine::cur->isZMove() )
								{
									// do not perform any compensation in case the moving is blocked
									Com::printFLN( PSTR( "; return curZ;" ) );
								}

                            break;
                        }

#if FEATURE_FIND_Z_ORIGIN
                        case 7:
                        {
                            Com::printF( PSTR( "Z-Origin;X;" ), g_nZOriginPosition[X_AXIS] );
                            Com::printF( Com::tSemiColon, (float)g_nZOriginPosition[X_AXIS] / Printer::axisStepsPerMM[X_AXIS] );
                            Com::printF( PSTR( ";Y;" ), g_nZOriginPosition[Y_AXIS] );
                            Com::printF( Com::tSemiColon, (float)g_nZOriginPosition[Y_AXIS] / Printer::axisStepsPerMM[Y_AXIS] );
                            Com::printF( PSTR( ";Z;" ), Printer::staticCompensationZ );
                            Com::printFLN( Com::tSemiColon, (float)Printer::staticCompensationZ / Printer::axisStepsPerMM[Z_AXIS] );
                            break;
                        }
#endif // FEATURE_FIND_Z_ORIGIN

                        case 9:
                        {
                            Com::printFLN( PSTR( "debug level: "), Printer::debugLevel );
                            break;
                        }
                        case 10:
                        {
//                          Com::printF( PSTR( "g_debugLevel= "), g_debugLevel );
//                          Com::printF( PSTR( ", g_debugLog= "), g_debugLog );
//                          Com::printF( PSTR( ", g_debugInt16= "), g_debugInt16 );
//                          Com::printF( PSTR( ", g_debugUInt16= "), (unsigned long)g_debugUInt16 );
/*                          Com::printF( PSTR( ", g_debugCounter[0]= "), g_debugCounter[0] );
                            Com::printF( PSTR( ", g_debugCounter[1]= "), g_debugCounter[1] );
                            Com::printF( PSTR( ", g_debugCounter[2]= "), g_debugCounter[2] );
                            Com::printF( PSTR( ", g_debugCounter[3]= "), g_debugCounter[3] );
                            Com::printF( PSTR( ", g_debugCounter[4]= "), g_debugCounter[4] );
                            Com::printFLN( PSTR( ", g_debugInt32= "), g_debugInt32 );
*/                          break;
                        }
                        case 11:
                        {
#if FEATURE_MILLING_MODE
                            Com::printF( PSTR( "operating mode= "), Printer::operatingMode );
                            Com::printFLN( PSTR( "") );
#endif // FEATURE_MILLING_MODE

#if FEATURE_CONFIGURABLE_Z_ENDSTOPS
                            Com::printF( PSTR( "Z endstop type= "), Printer::ZEndstopType );
                            Com::printF( PSTR( ", Z-Min= "), Printer::isZMinEndstopHit() );
                            Com::printF( PSTR( ", Z-Max= "), Printer::isZMaxEndstopHit() );
                            Com::printF( PSTR( ", lastZDirection= "), Printer::lastZDirection );
                            Com::printF( PSTR( ", endstopZMinHit= "), Printer::endstopZMinHit );
                            Com::printF( PSTR( ", endstopZMaxHit= "), Printer::endstopZMaxHit );
                            Com::printF( PSTR( ", ZEndstopUnknown= "), Printer::ZEndstopUnknown );
                            Com::printFLN( PSTR( "") );
#endif // FEATURE_CONFIGURABLE_Z_ENDSTOPS
                            break;
                        }
                        case 12:
                        {
                            Com::printF( PSTR( "extruder= "), Extruder::current->id );
                            Com::printF( PSTR( ", g_nHeatBedScanStatus= "), g_nHeatBedScanStatus );
                            break;
                        }
                        case 13:
                        {
                            Com::printFLN( PSTR( "LCD re-initialization") );
                            initializeLCD();
                        }

#if FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING
                        case 14:
                        {
                            Com::printF( PSTR( "target = " ), Printer::directPositionTargetSteps[X_AXIS] );
                            Com::printF( PSTR( "; " ), Printer::directPositionTargetSteps[Y_AXIS] );
                            Com::printF( PSTR( "; " ), Printer::directPositionTargetSteps[Z_AXIS] );
                            Com::printF( PSTR( "; " ), Printer::directPositionTargetSteps[E_AXIS] );
                            Com::printF( PSTR( "; current = " ), Printer::directPositionCurrentSteps[X_AXIS] );
                            Com::printF( PSTR( "; " ), Printer::directPositionCurrentSteps[Y_AXIS] );
                            Com::printF( PSTR( "; " ), Printer::directPositionCurrentSteps[Z_AXIS] );
                            Com::printF( PSTR( "; " ), Printer::directPositionCurrentSteps[E_AXIS] );
                            Com::printF( PSTR( "; remaining = " ), PrintLine::direct.stepsRemaining );
                            Com::printFLN( PSTR( "" ) );
                            break;
                        }
#endif // FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING

                        case 16:
                        {
                            Com::printF( PSTR( "stepperDirection=" ), Printer::stepperDirection[X_AXIS] );
                            Com::printF( PSTR( "/" ), Printer::stepperDirection[Y_AXIS] );
                            Com::printF( PSTR( "/" ), Printer::stepperDirection[Z_AXIS] );
                            Com::printFLN( PSTR( "" ) );
                            break;
                        }
                        case 17:
                        {
                            if( pCommand->hasS() )
                            {
                                if( pCommand->S == 2 )
                                {
                                    // output in [steps]
                                    Com::printF( PSTR( "nCPS Z;" ), Printer::queuePositionCurrentSteps[Z_AXIS] );

#if FEATURE_HEAT_BED_Z_COMPENSATION || FEATURE_WORK_PART_Z_COMPENSATION
                                    Com::printF( PSTR( "; cCZ;" ), Printer::compensatedPositionCurrentStepsZ );
                                    Com::printF( PSTR( "; sZP;" ), g_nZScanZPosition );
#endif // FEATURE_HEAT_BED_Z_COMPENSATION || FEATURE_WORK_PART_Z_COMPENSATION

#if FEATURE_FIND_Z_ORIGIN
                                    Com::printF( PSTR( "; oZ;" ), g_nZOriginPosition[Z_AXIS] );
#endif // FEATURE_FIND_Z_ORIGIN

#if FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING
                                    Com::printF( PSTR( "; cPSZ;" ), Printer::directPositionCurrentSteps[Z_AXIS] );
#endif // FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING

#if FEATURE_HEAT_BED_Z_COMPENSATION
                                    Com::printF( PSTR( "; hbO;" ), getHeatBedOffset() );
#endif // FEATURE_HEAT_BED_Z_COMPENSATION

#if FEATURE_WORK_PART_Z_COMPENSATION
                                    Com::printF( PSTR( "; wpO;" ), getWorkPartOffset() );
#endif // FEATURE_WORK_PART_Z_COMPENSATION

                                    Com::printFLN( PSTR( "" ) );

                                    // output in [mm]
                                    Com::printF( PSTR( "nCPS Z;" ), Printer::queuePositionCurrentSteps[Z_AXIS] / Printer::axisStepsPerMM[Z_AXIS] );

#if FEATURE_HEAT_BED_Z_COMPENSATION || FEATURE_WORK_PART_Z_COMPENSATION
                                    Com::printF( PSTR( "; cCZ;" ), Printer::compensatedPositionCurrentStepsZ / Printer::axisStepsPerMM[Z_AXIS] );
                                    Com::printF( PSTR( "; sZP;" ), g_nZScanZPosition / Printer::axisStepsPerMM[Z_AXIS] );
#endif // FEATURE_HEAT_BED_Z_COMPENSATION || FEATURE_WORK_PART_Z_COMPENSATION

#if FEATURE_FIND_Z_ORIGIN
                                    Com::printF( PSTR( "; oZ;" ), g_nZOriginPosition[Z_AXIS] / Printer::axisStepsPerMM[Z_AXIS] );
#endif // FEATURE_FIND_Z_ORIGIN

#if FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING
                                    Com::printF( PSTR( "; cPSZ;" ), Printer::directPositionCurrentSteps[Z_AXIS] / Printer::axisStepsPerMM[Z_AXIS] );
#endif // FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING

#if FEATURE_HEAT_BED_Z_COMPENSATION
                                    Com::printF( PSTR( "; hbO;" ), getHeatBedOffset() / Printer::axisStepsPerMM[Z_AXIS] );
#endif // FEATURE_HEAT_BED_Z_COMPENSATION

#if FEATURE_WORK_PART_Z_COMPENSATION
                                    Com::printF( PSTR( "; wpO;" ), getWorkPartOffset() / Printer::axisStepsPerMM[Z_AXIS] );
#endif // FEATURE_WORK_PART_Z_COMPENSATION

                                    Com::printFLN( PSTR( "" ) );
                                }
                            }
                            break;
                        }
                        case 18:
                        {
#if FEATURE_Z_MIN_OVERRIDE_VIA_GCODE
                            Com::printF( PSTR( "Z;" ), Printer::currentZSteps );
#endif // FEATURE_Z_MIN_OVERRIDE_VIA_GCODE

                            Com::printFLN( Com::tSemiColon, Printer::currentZPositionSteps() );
                            break;
                        }
                    }
                }

                break;
            }

#if FEATURE_24V_FET_OUTPUTS
            case 3300: // M3300 [P] [S] - configure the 24V FET outputs ( on/off )
            {
                if( pCommand->hasP() )
                {
                    switch( pCommand->P )
                    {
                        case 1:
                        {
                            if( pCommand->hasS() )
                            {
                                if ( pCommand->S == 0 )
                                {
                                    Printer::enableFET1 = 0;
                                    WRITE(FET1, Printer::enableFET1);
                                    Com::printFLN( PSTR( " 24V FET1-output = off ") );
                                }
                                else
                                {
                                    Printer::enableFET1 = 1;
                                    WRITE(FET1, Printer::enableFET1);
                                    Com::printFLN( PSTR( " 24V FET1-output = on ") );
                                }

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                                HAL::eprSetByte( EPR_RF_FET1_MODE, Printer::enableFET1 );
                                EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
                            }
                            break;
                        }
                        case 2:
                        {
                            if( pCommand->hasS() )
                            {
                                if ( pCommand->S == 0 )
                                {
                                    Printer::enableFET2 = 0;
                                    WRITE(FET2, Printer::enableFET2);
                                    Com::printFLN( PSTR( " 24V FET2-output = off ") );
                                }
                                else
                                {
                                    Printer::enableFET2 = 1;
                                    WRITE(FET2, Printer::enableFET2);
                                    Com::printFLN( PSTR( " 24V FET2-output = on ") );
                                }

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                                HAL::eprSetByte( EPR_RF_FET2_MODE, Printer::enableFET2 );
                                EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
                            }
                            break;
                        }
                        case 3:
                        {
                            if( pCommand->hasS() )
                            {
                                if ( pCommand->S == 0 )
                                {
                                    Printer::enableFET3 = 0;
                                    WRITE(FET3, Printer::enableFET3);
                                    Com::printFLN( PSTR( " 24V FET3-output = off ") );
                                }
                                else
                                {
                                    Printer::enableFET3 = 1;
                                    WRITE(FET3, Printer::enableFET3);
                                    Com::printFLN( PSTR( " 24V FET3-output = on ") );
                                }

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                                HAL::eprSetByte( EPR_RF_FET3_MODE, Printer::enableFET3 );
                                EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
                            }
                            break;
                        }
                    }
                }
                else
                {
                    showInvalidSyntax( pCommand->M );
                }

                break;
            }
#endif // FEATURE_24V_FET_OUTPUTS

#if FEATURE_230V_OUTPUT
            case 3301: // M3301 [S] - configure the 230V output ( on/off )
            {
                if( pCommand->hasS() )
                {
                    if( pCommand->S == 0 )
                    {
                        Printer::enable230VOutput = 0;
                        WRITE(OUTPUT_230V_PIN, Printer::enable230VOutput);
                        Com::printFLN( PSTR( "230V output = off") );
                    }
                    else
                    {
                        Printer::enable230VOutput = 1;
                        WRITE(OUTPUT_230V_PIN, Printer::enable230VOutput);
                        Com::printFLN( PSTR( "230V output = on") );
                    }

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                    // after a power-on, the 230 V plug always shall be turned off - thus, we do not store this setting to the EEPROM
                    // HAL::eprSetByte( EPR_RF_230V_OUTPUT_MODE, Printer::enable230VOutput );
                    // EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
                }
                else
                {
                    showInvalidSyntax( pCommand->M );
                }
                break;
            }
#endif // FEATURE_230V_OUTPUT

#if FEATURE_RGB_LIGHT_EFFECTS
            case 3303: // M3303 [P] [S] - configure the RGB light effects for heating
            {
                if( pCommand->hasP() )
                {
                    switch( pCommand->P )
                    {
                        case 1: // red
                        {
                            if( pCommand->hasS() )
                            {
                                if( pCommand->S >= 0 && pCommand->S <= 255 )
                                {
                                    g_uRGBHeatingR = pCommand->S;

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                                    HAL::eprSetByte( EPR_RF_RGB_HEATING_R, g_uRGBHeatingR );
                                    EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

                                    if( Printer::RGBLightMode == RGB_MODE_AUTOMATIC && Printer::RGBLightStatus == RGB_STATUS_HEATING )
                                    {
                                        setRGBTargetColors( g_uRGBHeatingR, g_uRGBHeatingG, g_uRGBHeatingB );
                                    }
                                    Com::printFLN( PSTR( "RGB_HEATING_R = "), g_uRGBHeatingR );
                                }
                                else
                                {
                                    Com::printFLN( PSTR( "RGB_HEATING_R out of range ") );
                                }
                            }
                            break;
                        }
                        case 2: // green
                        {
                            if( pCommand->hasS() )
                            {
                                if ( pCommand->S >= 0 && pCommand->S <= 255 )
                                {
                                    g_uRGBHeatingG = pCommand->S;

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                                    HAL::eprSetByte( EPR_RF_RGB_HEATING_G, g_uRGBHeatingG );
                                    EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

                                    if( Printer::RGBLightMode == RGB_MODE_AUTOMATIC && Printer::RGBLightStatus == RGB_STATUS_HEATING )
                                    {
                                        setRGBTargetColors( g_uRGBHeatingR, g_uRGBHeatingG, g_uRGBHeatingB );
                                    }
                                    Com::printFLN( PSTR( "RGB_HEATING_G = "), g_uRGBHeatingG );
                                }
                                else
                                {
                                    Com::printFLN( PSTR( "RGB_HEATING_G out of range ") );
                                }
                            }
                            break;
                        }
                        case 3: // blue
                        {
                            if( pCommand->hasS() )
                            {
                                if ( pCommand->S >= 0 && pCommand->S <= 255 )
                                {
                                    g_uRGBHeatingB = pCommand->S;

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                                    HAL::eprSetByte( EPR_RF_RGB_HEATING_B, g_uRGBHeatingB );
                                    EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

                                    if( Printer::RGBLightMode == RGB_MODE_AUTOMATIC && Printer::RGBLightStatus == RGB_STATUS_HEATING )
                                    {
                                        setRGBTargetColors( g_uRGBHeatingR, g_uRGBHeatingG, g_uRGBHeatingB );
                                    }
                                    Com::printFLN( PSTR( "RGB_HEATING_B = "), g_uRGBHeatingB );
                                }
                                else
                                {
                                    Com::printFLN( PSTR( "RGB_HEATING_B out of range ") );
                                }
                            }
                            break;
                        }
                    }
                }

                break;
            }
            case 3304: // M3304 [P] [S] - configure the RGB light effects for printing
            {
                if( pCommand->hasP() )
                {
                    switch( pCommand->P )
                    {
                        case 1: // red
                        {
                            if( pCommand->hasS() )
                            {
                                if ( pCommand->S >= 0 && pCommand->S <= 255 )
                                {
                                    g_uRGBPrintingR = pCommand->S;

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                                    HAL::eprSetByte( EPR_RF_RGB_PRINTING_R, g_uRGBPrintingR );
                                    EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

                                    if( Printer::RGBLightMode == RGB_MODE_AUTOMATIC && Printer::RGBLightStatus == RGB_STATUS_PRINTING )
                                    {
                                        setRGBTargetColors( g_uRGBPrintingR, g_uRGBPrintingG, g_uRGBPrintingB );
                                    }
                                    Com::printFLN( PSTR( "RGB_PRINTING_R = "), g_uRGBPrintingR );
                                }
                                else
                                {
                                    Com::printFLN( PSTR( "RGB_PRINTING_R out of range ") );
                                }
                            }
                            break;
                        }
                        case 2: // green
                        {
                            if( pCommand->hasS() )
                            {
                                if ( pCommand->S >= 0 && pCommand->S <= 255 )
                                {
                                    g_uRGBPrintingG = pCommand->S;

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                                    HAL::eprSetByte( EPR_RF_RGB_PRINTING_G, g_uRGBPrintingG );
                                    EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

                                    if( Printer::RGBLightMode == RGB_MODE_AUTOMATIC && Printer::RGBLightStatus == RGB_STATUS_PRINTING )
                                    {
                                        setRGBTargetColors( g_uRGBPrintingR, g_uRGBPrintingG, g_uRGBPrintingB );
                                    }
                                    Com::printFLN( PSTR( "RGB_PRINTING_G = "), g_uRGBPrintingG );
                                }
                                else
                                {
                                    Com::printFLN( PSTR( "RGB_PRINTING_G out of range ") );
                                }
                            }
                            break;
                        }
                        case 3: // blue
                        {
                            if( pCommand->hasS() )
                            {
                                if ( pCommand->S >= 0 && pCommand->S <= 255 )
                                {
                                    g_uRGBPrintingB = pCommand->S;

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                                    HAL::eprSetByte( EPR_RF_RGB_PRINTING_B, g_uRGBPrintingB );
                                    EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

                                    if( Printer::RGBLightMode == RGB_MODE_AUTOMATIC && Printer::RGBLightStatus == RGB_STATUS_PRINTING )
                                    {
                                        setRGBTargetColors( g_uRGBPrintingR, g_uRGBPrintingG, g_uRGBPrintingB );
                                    }
                                    Com::printFLN( PSTR( "RGB_PRINTING_B = "), g_uRGBPrintingB );
                                }
                                else
                                {
                                    Com::printFLN( PSTR( "RGB_PRINTING_B out of range ") );
                                }
                            }
                            break;
                        }
                    }
                }
                break;
            }

            case 3305: // M3305 [P] [S] - configure the RGB light effects for cooling
            {
                if( pCommand->hasP() )
                {
                    switch( pCommand->P )
                    {
                        case 1: // red
                        {
                            if( pCommand->hasS() )
                            {
                                if ( pCommand->S >= 0 && pCommand->S <= 255 )
                                {
                                    g_uRGBCoolingR = pCommand->S;

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                                    HAL::eprSetByte( EPR_RF_RGB_COOLING_R, g_uRGBCoolingR );
                                    EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

                                    if( Printer::RGBLightMode == RGB_MODE_AUTOMATIC && Printer::RGBLightStatus == RGB_STATUS_COOLING )
                                    {
                                        setRGBTargetColors( g_uRGBCoolingR, g_uRGBCoolingG, g_uRGBCoolingB );
                                    }
                                    Com::printFLN( PSTR( "RGB_COOLING_R = "), g_uRGBCoolingR );
                                }
                                else
                                {
                                    Com::printFLN( PSTR( "RGB_COOLING_R out of range ") );
                                }
                            }
                            break;
                        }
                        case 2: // green
                        {
                            if( pCommand->hasS() )
                            {
                                if ( pCommand->S >= 0 && pCommand->S <= 255 )
                                {
                                    g_uRGBCoolingG = pCommand->S;

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                                    HAL::eprSetByte( EPR_RF_RGB_COOLING_G, g_uRGBCoolingG );
                                    EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

                                    if( Printer::RGBLightMode == RGB_MODE_AUTOMATIC && Printer::RGBLightStatus == RGB_STATUS_COOLING )
                                    {
                                        setRGBTargetColors( g_uRGBCoolingR, g_uRGBCoolingG, g_uRGBCoolingB );
                                    }
                                    Com::printFLN( PSTR( "RGB_COOLING_G = "), g_uRGBCoolingG );
                                }
                                else
                                {
                                    Com::printFLN( PSTR( "RGB_COOLING_G out of range ") );
                                }
                            }
                            break;
                        }
                        case 3: // blue
                        {
                            if( pCommand->hasS() )
                            {
                                if ( pCommand->S >= 0 && pCommand->S <= 255 )
                                {
                                    g_uRGBCoolingB = pCommand->S;

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                                    HAL::eprSetByte( EPR_RF_RGB_COOLING_B, g_uRGBCoolingB );
                                    EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

                                    if( Printer::RGBLightMode == RGB_MODE_AUTOMATIC && Printer::RGBLightStatus == RGB_STATUS_COOLING )
                                    {
                                        setRGBTargetColors( g_uRGBCoolingR, g_uRGBCoolingG, g_uRGBCoolingB );
                                    }
                                    Com::printFLN( PSTR( "RGB_COOLING_B = "), g_uRGBCoolingB );
                                }
                                else
                                {
                                    Com::printFLN( PSTR( "RGB_COOLING_B out of range ") );
                                }
                            }
                            break;
                        }
                    }
                }
                break;
            }

            case 3306: // M3306 [P] [S] - configure the RGB light effects for idle
            {
                if( pCommand->hasP() )
                {
                    switch( pCommand->P )
                    {
                        case 1: // red
                        {
                            if( pCommand->hasS() )
                            {
                                if ( pCommand->S >= 0 && pCommand->S <= 255 )
                                {
                                    g_uRGBIdleR = pCommand->S;

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                                    HAL::eprSetByte( EPR_RF_RGB_IDLE_R, g_uRGBIdleR );
                                    EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

                                    if( Printer::RGBLightMode == RGB_MODE_AUTOMATIC && Printer::RGBLightStatus == RGB_STATUS_IDLE )
                                    {
                                        setRGBTargetColors( g_uRGBIdleR, g_uRGBIdleG, g_uRGBIdleB );
                                    }
                                    Com::printFLN( PSTR( "RGB_IDLE_R = "), g_uRGBIdleR );
                                }
                                else
                                {
                                    Com::printFLN( PSTR( "RGB_IDLE_R out of range ") );
                                }
                            }
                            break;
                        }
                        case 2: // green
                        {
                            if( pCommand->hasS() )
                            {
                                if ( pCommand->S >= 0 && pCommand->S <= 255 )
                                {
                                    g_uRGBIdleG = pCommand->S;

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                                    HAL::eprSetByte( EPR_RF_RGB_IDLE_G, g_uRGBIdleG );
                                    EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

                                    if( Printer::RGBLightMode == RGB_MODE_AUTOMATIC && Printer::RGBLightStatus == RGB_STATUS_IDLE )
                                    {
                                        setRGBTargetColors( g_uRGBIdleR, g_uRGBIdleG, g_uRGBIdleB );
                                    }
                                    Com::printFLN( PSTR( "RGB_IDLE_G = "), g_uRGBIdleG );
                                }
                                else
                                {
                                    Com::printFLN( PSTR( "RGB_IDLE_G out of range ") );
                                }
                            }
                            break;
                        }
                        case 3: // blue
                        {
                            if( pCommand->hasS() )
                            {
                                if ( pCommand->S >= 0 && pCommand->S <= 255 )
                                {
                                    g_uRGBIdleB = pCommand->S;

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                                    HAL::eprSetByte( EPR_RF_RGB_IDLE_B, g_uRGBIdleB );
                                    EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

                                    if( Printer::RGBLightMode == RGB_MODE_AUTOMATIC && Printer::RGBLightStatus == RGB_STATUS_IDLE )
                                    {
                                        setRGBTargetColors( g_uRGBIdleR, g_uRGBIdleG, g_uRGBIdleB );
                                    }
                                    Com::printFLN( PSTR( "RGB_IDLE_B = "), g_uRGBIdleB );
                                }
                                else
                                {
                                    Com::printFLN( PSTR( "RGB_IDLE_B out of range ") );
                                }
                            }
                            break;
                        }
                    }
                }

                break;
            }
            case 3307: // M3307 [P] [S] - configure the manual RGB light colors
            {
                if( pCommand->hasP() )
                {
                    switch( pCommand->P )
                    {
                        case 1: // red
                        {
                            if( pCommand->hasS() )
                            {
                                if ( pCommand->S >= 0 && pCommand->S <= 255 )
                                {
                                    g_uRGBManualR = pCommand->S;

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                                    HAL::eprSetByte( EPR_RF_RGB_MANUAL_R, g_uRGBManualR );
                                    EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

                                    if( Printer::RGBLightMode == RGB_MODE_MANUAL )
                                    {
                                        setRGBTargetColors( g_uRGBManualR, g_uRGBManualG, g_uRGBManualB );
                                    }
                                    Com::printFLN( PSTR( "RGB_MANUAL_R = "), g_uRGBManualR );
                                }
                                else
                                {
                                    Com::printFLN( PSTR( "RGB_MANUAL_R out of range ") );
                                }
                            }
                            break;
                        }
                        case 2: // green
                        {
                            if( pCommand->hasS() )
                            {
                                if ( pCommand->S >= 0 && pCommand->S <= 255 )
                                {
                                    g_uRGBManualG = pCommand->S;

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                                    HAL::eprSetByte( EPR_RF_RGB_MANUAL_G, g_uRGBManualG );
                                    EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

                                    if( Printer::RGBLightMode == RGB_MODE_MANUAL )
                                    {
                                        setRGBTargetColors( g_uRGBManualR, g_uRGBManualG, g_uRGBManualB );
                                    }
                                    Com::printFLN( PSTR( "RGB_MANUAL_G = "), g_uRGBManualG );
                                }
                                else
                                {
                                    Com::printFLN( PSTR( "RGB_MANUAL_G out of range ") );
                                }
                            }
                            break;
                        }
                        case 3: // blue
                        {
                            if( pCommand->hasS() )
                            {
                                if ( pCommand->S >= 0 && pCommand->S <= 255 )
                                {
                                    g_uRGBManualB = pCommand->S;

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                                    HAL::eprSetByte( EPR_RF_RGB_MANUAL_B, g_uRGBManualB );
                                    EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE

                                    if( Printer::RGBLightMode == RGB_MODE_MANUAL )
                                    {
                                        setRGBTargetColors( g_uRGBManualR, g_uRGBManualG, g_uRGBManualB );
                                    }
                                    Com::printFLN( PSTR( "RGB_MANUAL_B = "), g_uRGBManualB );
                                }
                                else
                                {
                                    Com::printFLN( PSTR( "RGB_MANUAL_B out of range ") );
                                }
                            }
                            break;
                        }
                    }
                }

                break;
            }
            case 3308: // M3308 [P] - configure the RGB light mode
            {
                if( pCommand->hasP() )
                {
                    switch( pCommand->P )
                    {
                        case RGB_MODE_OFF:
                        {
                            Printer::RGBLightStatus         = RGB_STATUS_NOT_AUTOMATIC;
                            Printer::RGBLightMode           = RGB_MODE_OFF;
                            Printer::RGBLightModeForceWhite = 0;

                            setRGBTargetColors( 0, 0, 0 );

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                            HAL::eprSetByte( EPR_RF_RGB_LIGHT_MODE, Printer::RGBLightMode );
                            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
                            break;
                        }
                        case RGB_MODE_WHITE:
                        {
                            Printer::RGBLightStatus         = RGB_STATUS_NOT_AUTOMATIC;
                            Printer::RGBLightMode           = RGB_MODE_WHITE;
                            Printer::RGBLightModeForceWhite = 0;

                            setRGBTargetColors( 255, 255, 255 );

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                            HAL::eprSetByte( EPR_RF_RGB_LIGHT_MODE, Printer::RGBLightMode );
                            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
                            break;
                        }
                        case RGB_MODE_AUTOMATIC:
                        {
                            Printer::RGBLightStatus         = RGB_STATUS_AUTOMATIC;
                            Printer::RGBLightMode           = RGB_MODE_AUTOMATIC;
                            Printer::RGBLightModeForceWhite = 0;

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                            HAL::eprSetByte( EPR_RF_RGB_LIGHT_MODE, Printer::RGBLightMode );
                            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
                            break;
                        }
                        case RGB_MODE_MANUAL:
                        {
                            Printer::RGBLightStatus         = RGB_STATUS_NOT_AUTOMATIC;
                            Printer::RGBLightMode           = RGB_MODE_MANUAL;
                            Printer::RGBLightModeForceWhite = 0;

                            setRGBTargetColors( g_uRGBManualR, g_uRGBManualG, g_uRGBManualB );

#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                            HAL::eprSetByte( EPR_RF_RGB_LIGHT_MODE, Printer::RGBLightMode );
                            EEPROM::updateChecksum();
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
                            break;
                        }
                    }
                }

                break;
            }
#endif // FEATURE_RGB_LIGHT_EFFECTS

#if FEATURE_HEAT_BED_Z_COMPENSATION
            case 3901: // 3901 [X] [Y] - configure the Matrix-Position to Scan, [S] confugure learningrate, [P] configure dist weight || by Nibbels
            case 3900: // 3900 direct preconfig, no break; -> next is M3900.
            {
                if( isSupportedMCommand( pCommand->M, OPERATING_MODE_PRINT ) )
                {
                    bool err3900r = false;
                    if( g_ZCompensationMatrix[0][0] != EEPROM_FORMAT )
                    {
                        // we load the z compensation matrix before its first usage because this can take some time
                        Com::printFLN( PSTR( "M3900/M3901: INFO Die Z-Matrix wurde aus dem EEPROM gelesen." ) );
                        prepareZCompensation();
                    }

                    if( g_ZCompensationMatrix[0][0] == EEPROM_FORMAT )
                    {
                        if( !pCommand->hasR() ){ //Normaler ZOS, kein Auto-Matrix-Leveling.
                            if( pCommand->hasX() )
                            {
                                // test and take over the specified value
                                unsigned char nTempUC = (unsigned char)pCommand->X;

                                //Wessix idee: zufalls-Scanpunkt wegen DDP-Platten-Schonung.
                                if( nTempUC == 0 ) nTempUC = (unsigned char)random( (HEAT_BED_SCAN_X_START_MM == 0) ? 1 : 2 , g_uZMatrixMax[X_AXIS] ); // ergibt min bis max-1

                                //wenn die Startposition nicht 0 ist, wird eine Dummy-Matrix-Linie ergänzt. Mit der sollten wir nicht arbeiten.
                                if( nTempUC < 1 + ((HEAT_BED_SCAN_X_START_MM == 0) ? 0 : 1) ) nTempUC = 1 + ((HEAT_BED_SCAN_X_START_MM == 0) ? 0 : 1);
                                if( nTempUC > g_uZMatrixMax[X_AXIS] - 1 ) nTempUC = g_uZMatrixMax[X_AXIS] - 1; //2..n-1

                                g_ZOSTestPoint[X_AXIS] = nTempUC;
#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                                unsigned char oldval = HAL::eprGetByte(EPR_RF_MOD_ZOS_SCAN_POINT_X);
                                if(oldval != g_ZOSTestPoint[X_AXIS]){
                                   HAL::eprSetByte( EPR_RF_MOD_ZOS_SCAN_POINT_X, g_ZOSTestPoint[X_AXIS] );
                                   EEPROM::updateChecksum(); //deshalb die prüfung
                                }
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
                                if( Printer::debugInfo() )
                                {
                                    Com::printF( PSTR( "M3900/M3901: CHANGED X ZOS Testposition: " ), nTempUC );
                                    if(HEAT_BED_SCAN_X_START_MM == 0){
                                        Com::printF( PSTR( " [Z-Matrix index] X={1.." ), g_uZMatrixMax[X_AXIS]-1 );
                                    }
                                    else
                                    {
                                        Com::printF( PSTR( " [Z-Matrix index] X={2.." ), g_uZMatrixMax[X_AXIS]-1 );
                                    }
                                    Com::printFLN( PSTR( "}" ) );
                                }
                            }

                            if( pCommand->hasY() )
                            {
                                // test and take over the specified value
                                unsigned char nTempUC = (unsigned char)pCommand->Y;

                                //Wessix idee: zufalls-Scanpunkt wegen DDP-Platten-Schonung.
                                if( nTempUC == 0 ) nTempUC = (unsigned char)random( (HEAT_BED_SCAN_Y_START_MM == 0) ? 1 : 2 , g_uZMatrixMax[Y_AXIS] ); // ergibt min bis max-1

                                if( nTempUC < 1 + ((HEAT_BED_SCAN_Y_START_MM == 0) ? 0 : 1) ) nTempUC = 1 + ((HEAT_BED_SCAN_Y_START_MM == 0) ? 0 : 1);
                                if( nTempUC > g_uZMatrixMax[Y_AXIS] - 1 ) nTempUC = g_uZMatrixMax[Y_AXIS] - 1; //2..n-1

                                g_ZOSTestPoint[Y_AXIS] = nTempUC;
#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                                unsigned char oldval = HAL::eprGetByte(EPR_RF_MOD_ZOS_SCAN_POINT_Y);
                                if(oldval != g_ZOSTestPoint[Y_AXIS]){
                                   HAL::eprSetByte( EPR_RF_MOD_ZOS_SCAN_POINT_Y, g_ZOSTestPoint[Y_AXIS] );
                                   EEPROM::updateChecksum(); //deshalb die prüfung
                                }
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
                                if( Printer::debugInfo() )
                               {
                                    Com::printF( PSTR( "M3900/M3901: CHANGED Y ZOS Testposition: " ), nTempUC );
                                    if(HEAT_BED_SCAN_Y_START_MM == 0){
                                        Com::printF( PSTR( " [Z-Matrix index] Y={1.." ), g_uZMatrixMax[Y_AXIS]-1 );
                                    }
                                    else
                                    {
                                        Com::printF( PSTR( " [Z-Matrix index] Y={2.." ), g_uZMatrixMax[Y_AXIS]-1 );
                                    }
                                    Com::printFLN( PSTR( "}" ) );
                                }
                            }

                            if( pCommand->hasS() )
                            {
                                // M3900 S set learning rate to limit changes caused of z-Offset Scan. This might proof handy for multiple positions scans.
                                if ( pCommand->S >= 0 && pCommand->S <= 100 )
                                {
                                    g_ZOSlearningRate = (float)pCommand->S *0.01f;
                                    Com::printFLN( PSTR( "M3900/M3901: CHANGED [S] ZOS learning rate : "), g_ZOSlearningRate);              
                                }
                                else
                                {
                                    Com::printFLN( PSTR( "M3900/M3901: ERROR [S] ZOS learning rate ignored, out of range {0...100}") );
                                    err3900r = true;
                                }
                            }

                            if( pCommand->hasP() )
                            {
                                // M3900 P set distance weight: This can be used as something like the auto-bed-leveling (if used in all corners) but technically affects the Z_Matrix
                                if ( pCommand->P >= 0 && pCommand->P <= 100 )
                                {
                                    g_ZOSlearningGradient = (float)pCommand->P *0.01f;
                                    Com::printFLN( PSTR( "M3900/M3901: CHANGED [P] ZOS learning linear distance weight : "), g_ZOSlearningGradient);
                                }
                                else
                                {
                                    Com::printFLN( PSTR( "M3900/M3901: ERROR [P] ZOS learning DistanceWeight, out of range {0...100}") );
                                    err3900r = true;
                                }
                            }

                            //Anzeige der aktuellen Settings
                            Com::printFLN( PSTR( "M3900/M3901: ### AKTIVE ZOS SETTINGS ###" ) );
                            Com::printF( PSTR( "M3900/M3901: Testposition [X]: " ), g_ZOSTestPoint[X_AXIS] );                   
                            Com::printF( PSTR( " Testposition [Y]: " ), g_ZOSTestPoint[Y_AXIS] );
                            Com::printFLN( PSTR( " [Z-Matrix index]" ) );
                            Com::printFLN( PSTR( "M3900/M3901: [S] ZOS learning rate is : "), g_ZOSlearningRate);
                            if ( g_ZOSlearningRate == 1.0f )
                            {
                                Com::printFLN( PSTR( "M3900/M3901: INFO [S] ZOS::overwrite mode (1.00)") ); 
                            }
                            else
                            {
                                Com::printFLN( PSTR( "M3900/M3901: INFO [S] ZOS::additiv / learning mode (0.00 - 0.99)") );
                            }
                            Com::printFLN( PSTR( "M3900/M3901: [P] ZOS learning linear distance weight is : "), g_ZOSlearningGradient);
                            if ( g_ZOSlearningGradient == 0.0f )
                            {
                                Com::printFLN( PSTR( "M3900/M3901: INFO [P] ZOS::reiner Offset-Scan") );    
                            }
                            else if ( g_ZOSlearningGradient == 1.0f )
                            {
                                Com::printFLN( PSTR( "M3900/M3901: INFO [P] ZOS::rein linear abstandsgewichteter Scan") );  
                            }
                            else
                            {
                                Com::printFLN( PSTR( "M3900/M3901: INFO [P] ZOS::Das Ergebnis-Offset setzt sich zusammen aus Abstandsgewichtung und Offset") );                 
                            }

                            if ( g_ZOSlearningGradient > 0.0f )
                            {
                                Com::printFLN( PSTR( "M3900/M3901: INFO [P] Set 0 => 0.0 for Offset only, set 100 => 1.0 for distance weight only") );
                                Com::printFLN( PSTR( "M3900/M3901: INFO [P] Combine linear distance weight with low learning rate and multiple checks at corners (for example) against bed warping!") );                        
                            }
                            if ( g_ZOSlearningRate == 1.0f )
                            {
                                Com::printFLN( PSTR( "M3900/M3901: FORMEL Z-Matrix = EEPROM-Matrix + g_ZOSlearningRate*(g_ZOSlearningGradient*weight(x,y)*Offset + (1.0-g_ZOSlearningGradient)*Offset)" ) );
                            }
                            else
                            {
                                Com::printFLN( PSTR( "M3900/M3901: FORMEL Z-Matrix = Z-Matrix + g_ZOSlearningRate*(g_ZOSlearningGradient*weight(x,y)*Offset + (1.0-g_ZOSlearningGradient)*Offset)" ) );
                            }
                        }else{ // pCommand->hasR() //Auto-Matrix-Leveling.
                            //Auto-Matrix-Leveling aktiv.
                        }
                        if(pCommand->M == 3901)
                        {
                            //M3901
                            //M3900 wird nicht gestartet - nur preconfig, kein mhier-Scan!
                            if( pCommand->hasR() ) Com::printFLN( PSTR( "M3901: Use M3900 for Auto-Matrix-Leveling!" ) );
                        }
                        else
                        {
                            if(err3900r){
                                Com::printFLN( PSTR( "M3900/M3901: ERROR in Config ### ZOS SKIPPED ###" ) );
                            }
                            else{
                                //M3900:
                                startZOScan(pCommand->hasR()); //mit R im GCode macht der Scan ein Auto-Matrix-Leveling, anstatt die anderen Schalter zu bedienen.
                                Commands::waitUntilEndOfZOS();
                            }
                        }
                    }
                    else
                    {
                        Com::printFLN( PSTR( "M3900/M3901: ERROR Matrix Initialisation Error!" ) );
                        Com::printFLN( PSTR( "M3900/M3901: INFO Die Z-Matrix konnte nicht aus dem EEPROM gelesen werden." ) );
                        Com::printFLN( PSTR( "M3900/M3901: INFO Vermutlich noch nie einen Heat-Bed-Scan gemacht." ) );
                    }
                }
                break;
            }
#else
            case 3900: // M3900 search for the heat bed and set the Z offset appropriately
            case 3901: // 3901 [X] [Y] - configure the Matrix-Position to Scan, [S] confugure learningrate, [P] configure dist weight || by Nibbels
            {
                Com::printFLN( PSTR( "M3900/M3901 inactive Feature FEATURE_HEAT_BED_Z_COMPENSATION" ) );
                break;
            }
#endif // FEATURE_HEAT_BED_Z_COMPENSATION   


#if FEATURE_HEAT_BED_Z_COMPENSATION
            case 3902: // M3902 Nibbels Matrix Manipulations "NMM"
            {
                if( isSupportedMCommand( pCommand->M, OPERATING_MODE_PRINT ) )
                {
                    //NMM Funktion 1 - R=Repair
                    if ( pCommand->hasR() ) 
                    {
                        //search for a hole within the heat beds z-Matrix
                        fixKeramikLochInMatrix();
                    }

                    if ( pCommand->hasE() ) 
                    {
                        //completly wipe the matrix-data to zero -> flatten the matrix to nothing.
                        setMatrixNull();
                    }
                    //NMM Funktion 2 - Z=Offset manuell nachstellen
                    if ( pCommand->hasZ() ) 
                    {
                        if( g_ZCompensationMatrix[0][0] != EEPROM_FORMAT )
                        {
                            // we load the z compensation matrix before its first usage because this can take some time
                            Com::printFLN( PSTR( "M3901: INFO Die Z-Matrix wurde aus dem EEPROM gelesen." ) );
                            prepareZCompensation();
                        }
                        if( g_ZCompensationMatrix[0][0] == EEPROM_FORMAT )
                        {
                            //search for a hole within the heat beds z-Matrix
                            float hochrunter = (float)pCommand->Z;
                            if(hochrunter > 0.2f) hochrunter = 0.2f;
                            if(hochrunter < -0.2f) hochrunter = -0.2f;

                            Com::printFLN( PSTR( "#############################################################################") );
                            if(hochrunter < 0.0f){
                                Com::printFLN( PSTR( "M3902: Duese-Bett-Abstand wird kleiner gemacht. "), hochrunter );
                            }else if(hochrunter > 0.0f){
                                Com::printFLN( PSTR( "M3902: Duese-Bett-Abstand wird groesser gemacht. "), hochrunter );
                            }else{
                                Com::printFLN( PSTR( "M3902: Duese-Bett-Abstand bleibt gleich. Das Offset wird in die Matrix verrechnet und genullt."), hochrunter );
                            }
                            Com::printFLN( PSTR( "M3902: -Z heisst Bett hoch/weniger Abstand, +Z heisst Bett runter/mehr Abstand. ") );
                            Com::printFLN( PSTR( "M3902: Bei Z=0 ändert sich nichts, doch es wird das aktuelle Offset in die Matrix verrechnet. ") );
                            Com::printFLN( PSTR( "#############################################################################") );

                            // determine the minimal distance between extruder and heat bed
                            determineCompensationOffsetZ(); //-> schreibt kleinsten abstand in g_offsetZCompensationSteps, sollte schon drin sein, aber man weiß nie.
                            Com::printFLN( PSTR( "M3902: Alt::Min. Bett-Hotend Abstand [Steps] = " ), -1*(int)g_offsetZCompensationSteps );

                            long hochrunterSteps = long(hochrunter * Printer::axisStepsPerMM[Z_AXIS]); //axissteps ist auch float

                            Com::printFLN( PSTR( "M3902: Veraenderung Z [mm] = "), hochrunter );
                            Com::printFLN( PSTR( "M3902: Veraenderung Z [Steps] = " ), hochrunterSteps );

                            if(hochrunter == 0.00f){
                                //wenn Z=0.0 soll alles so bleibenk aber das Offset ins Matrix-Offset reingerechnet und genullt werden.
                                //das Offset muss negativ eingehen, denn wenn die Matrix "weniger tief" ist, bleibt die Düse weiter weg vom Bett.
                                hochrunterSteps = long((Printer::ZOffset * Printer::axisStepsPerMM[Z_AXIS]) / 1000); //offset-stepps neu berechnen!                         
                                Com::printFLN( PSTR( "M3902: Veraenderung = zOffset --> zMatrix; Offset = 0;" ) );
                            }

                            //das ist ein negativer wert! Je mehr Abstand, desto negativer. Positiv verboten.
                            if(g_offsetZCompensationSteps + hochrunterSteps > (HEAT_BED_SCAN_Z_START_STEPS - -1*g_nScanHeatBedUpFastSteps)){
                                Com::printF( PSTR( "M3902: Fehler::Z-Matrix wuerde positiv werden. Das waere eine Kollision um " ), (int)(g_offsetZCompensationSteps + hochrunterSteps) );
                                Com::printFLN( PSTR( " [Steps] bzw. " ), Printer::invAxisStepsPerMM[Z_AXIS]*(float)(g_offsetZCompensationSteps + hochrunterSteps),2 );
                                Com::printFLN( PSTR( " [mm]" ) );
                            }else{
                                Com::printFLN( PSTR( "M3902: Neu::Min. Bett-Extruder Restabstand = " ), -1*(int)(g_offsetZCompensationSteps) );

                                short   x;
                                short   y;
                                short   deltaZ  = (short)hochrunterSteps;
                                bool overflow = false;
                                bool overnull = false;
                                bool overH = false;

                                for( x=1; x<=g_uZMatrixMax[X_AXIS]; x++ )
                                {
                                    for( y=1; y<=g_uZMatrixMax[Y_AXIS]; y++ )
                                    {
                                        if((long)g_ZCompensationMatrix[x][y] + (long)deltaZ >= 32767){
                                            overflow = true;
                                        }else{
                                            g_ZCompensationMatrix[x][y] += deltaZ;  
                                            if(g_ZCompensationMatrix[x][y] > 0) overnull = true; //overflow oder kollision, kann einfach nicht sein und abfrage ist kurz.
                                            if(g_ZCompensationMatrix[x][y] > (HEAT_BED_SCAN_Z_START_STEPS - -1*g_nScanHeatBedUpFastSteps) ) overH = true;
                                        }
                                    }
                                }
                                if(overnull){
                                    Com::printFLN( PSTR( "M3901: WARNING::positive Matrix::Please fix Z-Screw" ) );
                                }
                                if(overH){
                                    Com::printFLN( PSTR( "M3901: ERROR::sehr positive Matrix::ReLoading zMatrix from EEPROM to RAM" ) );
                                    loadCompensationMatrix( (unsigned int)(EEPROM_SECTOR_SIZE * g_nActiveHeatBed) );
                                }else if(overflow){
                                    Com::printFLN( PSTR( "M3901: ERROR::Overflow in Matrix::ReLoading zMatrix from EEPROM to RAM" ) );
                                    loadCompensationMatrix( (unsigned int)(EEPROM_SECTOR_SIZE * g_nActiveHeatBed) );
                                }else{
                                    if(hochrunter == 0.00f){
                                        Printer::ZOffset = 0; //offset um nullen
#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                                        if( HAL::eprGetInt32( EPR_RF_Z_OFFSET ) != Printer::ZOffset )
                                        {
                                            HAL::eprSetInt32( EPR_RF_Z_OFFSET, Printer::ZOffset );
                                            EEPROM::updateChecksum();
                                        }
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
                                        g_staticZSteps = ((Printer::ZOffset+g_nSensiblePressureOffset) * Printer::axisStepsPerMM[Z_AXIS]) / 1000; //offset-stepps neu berechnen
                                    }   
                                    g_ZMatrixChangedInRam = 1;
                                }
                            }

                            // determine the minimal distance between extruder and heat bed
                            determineCompensationOffsetZ();
                            Com::printFLN( PSTR( "M3902: Neu::Min. Bett-Extruder Restabstand [Steps] = " ), -1*(int)g_offsetZCompensationSteps );
                            Com::printFLN( PSTR( "M3902: Neu::Min. Bett-Extruder Restabstand [mm] = " ), -1*Printer::invAxisStepsPerMM[Z_AXIS]*(float)g_offsetZCompensationSteps,2 );
                        }else{
                            Com::printFLN( PSTR( "M3902: ERROR::Matrix Initialisation Error!" ) );
                            Com::printFLN( PSTR( "M3902: INFO Die Z-Matrix konnte nicht aus dem EEPROM gelesen werden." ) );
                            Com::printFLN( PSTR( "M3902: INFO Vermutlich nie einen Heat-Bed-Scan gemacht." ) );
                        }   
                    }
                    
                    //NMM Funktion 3 - S=Save,Sichern der Matrix an spezielle EEPROM-Position
                    if( pCommand->hasS() )
                    {
                        if(pCommand->S >= 1 && pCommand->S <= EEPROM_MAX_HEAT_BED_SECTORS){
                            // save the determined values to the EEPROM @ savepoint "pCommand->S" Standard: 1..9
                            unsigned int savepoint = (unsigned int)pCommand->S;
                            if( saveCompensationMatrix( (unsigned int)(EEPROM_SECTOR_SIZE * savepoint) ) ) //g_nActiveHeatBed --> pCommand->S
                            {
                                //Ähm.... diesen Fall gibts garnicht laut saveCompensationMatrix()
                                //retcode != 0
                                //Com::printFLN( PSTR( "M3902: Save the Matrix::ERROR::The heat bed compensation matrix could not be saved" ) );
                            }
                            else
                            {
                                //retcode 0
                                Com::printFLN( PSTR( "M3902: Save the Matrix::OK" ) );
                            }
                        }else{
                            Com::printFLN( PSTR( "M3902: Save the Matrix::ERROR::invalid savepoint, invalid active heatbedmatrix" ) );
                        }
                    }
                    
                }
                break;
            }
#else
            case 3902: // M3902 Nibbels Matrix Manipulations "NMM"
            {
                Com::printFLN( PSTR( "M3902 inactive Feature" ) );
                break;
            }
#endif // FEATURE_HEAT_BED_Z_COMPENSATION

#if FEATURE_SENSIBLE_PRESSURE
            case 3909: // M3909 [P]PressureDigits - configure the sensible pressure value threshold || by Wessix and Nibbels
            {
                if( isSupportedMCommand( pCommand->M, OPERATING_MODE_PRINT ) )
                {
                    //Statusänderung per M3909 P10000 (for 10000 [digits])
                    Commands::waitUntilEndOfAllMoves();
                    if (pCommand->hasP() ){
                        if ( pCommand->P >= 0 && pCommand->P < EMERGENCY_PAUSE_DIGITS_MAX )
                        {
                            if(pCommand->P > 32767) pCommand->P = 32767;
                            g_nSensiblePressureDigits = pCommand->P;
                            if(g_nSensiblePressureDigits){
                                Com::printFLN( PSTR( "M3909: SensiblePressure Pmax="),g_nSensiblePressureDigits );
#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                                short oldval = HAL::eprGetInt16(EPR_RF_MOD_SENSEOFFSET_DIGITS);
                                if(oldval != g_nSensiblePressureDigits){
                                   HAL::eprSetInt16( EPR_RF_MOD_SENSEOFFSET_DIGITS, g_nSensiblePressureDigits );
                                   EEPROM::updateChecksum(); //deshalb die prüfung
                                }
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
                            }
                            else{
                                Com::printFLN( PSTR( "M3909: SensiblePressure disabled") );
                            }
                        }else{
                            Com::printFLN( PSTR( "M3909: ERROR Parameter [P]") );
                        }
                    }else{
                        //if possible read from EEPROM
                        short oldval = HAL::eprGetInt16(EPR_RF_MOD_SENSEOFFSET_DIGITS);
                        if ( oldval > 0 && oldval < EMERGENCY_PAUSE_DIGITS_MAX ){
                            Com::printFLN( PSTR( "M3909: SensiblePressure EEPROM: Pmax="), oldval);
                            g_nSensiblePressureDigits = oldval;
                        }
                    }

                    if (pCommand->hasS() ){
                        if ( pCommand->S > 0 && pCommand->S <= 300 )
                        {
                            //max darf nie 0 werden!! div/0 bei zeile ~5600
                            g_nSensiblePressureOffsetMax = (short)pCommand->S;
                            Com::printFLN( PSTR( "M3909: SensiblePressure [S]max. Offset changed to "),g_nSensiblePressureOffsetMax );
#if FEATURE_AUTOMATIC_EEPROM_UPDATE
                            short oldval = HAL::eprGetInt16(EPR_RF_MOD_SENSEOFFSET_OFFSET_MAX);
                            if(oldval != g_nSensiblePressureOffsetMax){
                               HAL::eprSetInt16( EPR_RF_MOD_SENSEOFFSET_OFFSET_MAX, g_nSensiblePressureOffsetMax );
                               EEPROM::updateChecksum(); //deshalb die prüfung
                            }
#endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
                        }else{
                            Com::printFLN( PSTR( "M3909: ERROR::>0.3mm=S300 - This function is ment to compensate minimal amounts of too close distance.") );
                            Com::printFLN( PSTR( "M3909: ERROR::>0.3mm=S300 - Diese Funktion soll nur minimal justieren.") );
                            Com::printFLN( PSTR( "M3909: INFO::If you have to auto-compensate your offset to high, clean your nozzle, rise your temp, lower your speed.") );
                            Com::printFLN( PSTR( "M3909: INFO::This function should lower the chance of an accidential emergency block on the first layer. It cannot help you to avoid calibration!") );
                        }
                    }

                    //Statusausgabe per M3909
                    if(g_nSensiblePressureDigits){
                        Com::printF( PSTR( "M3909: INFO SensiblePressure threshold is active. [P]PressureDigit = +-"), g_nSensiblePressureDigits );
                        Com::printFLN( PSTR( " [digits] (Standard: use positive `Normal Digits`+20%)") );           
                        Com::printF( PSTR( "M3909: INFO SensiblePressures [S]max. offset is "), g_nSensiblePressureOffsetMax );   
                        Com::printFLN( PSTR( " [um] (Standard: 180um, Min: 1um, Max: 300um)") );            
                    }else{
                        Com::printFLN( PSTR( "M3909: INFO SensiblePressure is currently disabled. [P]PressureDigit = OFF (Standard: use positive `Normal Digits`+20%" ) );
                        Com::printF( PSTR( "M3909: INFO SensiblePressures [S]max. offset is "), g_nSensiblePressureOffsetMax );   
                        Com::printFLN( PSTR( " [um] (Standard: 180um)") );      
                    }
                    if (!pCommand->hasS() && !pCommand->hasP()) Com::printFLN( PSTR( "M3909: INFO Example: M3909 P8000 S180 (8000digits and offset limited to 0,18mm)") );
                }
                break;
            }
#else
            case 3909: // 3909 [P]PressureDigits - configure the sensible pressure value threshold || by Wessix and Nibbels
            {
                Com::printFLN( PSTR( "M3909 is disabled : inactive Feature FEATURE_EMERGENCY_PAUSE || FEATURE_HEAT_BED_Z_COMPENSATION" ) );
                break;
            }
#endif // FEATURE_SENSIBLE_PRESSURE

#if FEATURE_DIGIT_FLOW_COMPENSATION
            case 3911: // 3911 [S]Inc/Dec - Testfunction for AtlonXP's DigitFlowCompensation
            {
                if ( pCommand->hasS() && pCommand->hasP() ){
                    short min = abs( static_cast<short>(pCommand->S) );
                    short max = abs( static_cast<short>(pCommand->P) );
                    if(min > max) {
                        min = abs( static_cast<short>(pCommand->P) );
                        max = abs( static_cast<short>(pCommand->S) );
                    }
                    if(min < 500) min = 500; //weniger ist in jedem fall sinnfrei, selbst mit ninjaflex und digit homing
                    if(max < min) max = min; //geht: flowsprung.
                    g_nDigitFlowCompensation_Fmin = min;
                    g_nDigitFlowCompensation_Fmax = max;
                }

                if ( pCommand->hasE() ){
                    int8_t e = static_cast<int8_t>(pCommand->E);
                    if(e > 99) e = 99;
                    if(e < -99) e = -99;
                    g_nDigitFlowCompensation_intense = e;
                }

                Com::printFLN( PSTR( "[S|P] Flow CMP min: " ), g_nDigitFlowCompensation_Fmin);
                Com::printFLN( PSTR( "[S|P] Flow CMP max: " ), g_nDigitFlowCompensation_Fmax);
                Com::printFLN( PSTR( "[E]   Flow CMP max Prozente: " ), g_nDigitFlowCompensation_intense);
                break;
            }
#endif // FEATURE_DIGIT_FLOW_COMPENSATION

            case 3919: // 3919 [S]mikrometer - Testfunction for Dip-Down-Hotend beim T1: Einstellen des extruderspezifischen Z-Offsets
            {
                /* Eigentlich kann man das mit jedem Extruder machen, aber ich lasse das nur für T1 zu, weil ich nur das testen kann. Der Rechte Extruder kann damit absinken, wenn das Filament ihn runterdrückt. Der Linke bleibt in jedem Fall gleich hoch auf der Höhe des Homings! */
                if ( pCommand->hasZ() && (pCommand->Z <= 0 && pCommand->Z >= -2.0f) ){
                    if(Printer::debugDryrun()) break;
                    Commands::waitUntilEndOfAllMoves();
                    
                    Extruder *actExtruder = Extruder::current;
                    if(pCommand->hasT() && pCommand->T < NUM_EXTRUDER) actExtruder = &extruder[pCommand->T]; //unter umständen ist actExtruder was anderes wie Extruder::current!
                    if(extruder[1].id == actExtruder->id){ //wenn zielextruder aktuell oder Tn = Extrudernummer 1
                        if(pCommand->Z <= 0 && pCommand->Z >= -2.0f) actExtruder->zOffset = int32_t((float)pCommand->Z * Printer::axisStepsPerMM[Z_AXIS]);
                        //Nur wenn aktuell der extruder mit ID1 aktiv ist, dann sofort nachstellen, ohne T0/T1/... :
                        if(Extruder::current->id == extruder[1].id){
                            //Printer::extruderOffset[Z_AXIS] = -Extruder::current->zOffset*Printer::invAxisStepsPerMM[Z_AXIS];
                            //Printer::updateCurrentPosition();        ... besser ist es, den zweiten extruder neu auszuwählen:
                            Extruder::selectExtruderById(Extruder::current->id);     
                        }
                        Com::printFLN( PSTR( "M3919 T1 Spring displace: " ), actExtruder->zOffset * Printer::invAxisStepsPerMM[Z_AXIS] );
                    }else{
                        Com::printFLN( PSTR( "M3919 Error: !=Extr." ), extruder[1].id );
                    }
                }else{
                    Com::printFLN( PSTR( "M3919 Help: Write M3919 T1 Z-0.500 when T1 goes down 500um/0.5mm" ) );
                }
                break;
            }

#if FEATURE_VISCOSITY_TEST
            case 3939: // 3939 startViscosityTest - Testfunction to determine the digits over extrusion speed || by Nibbels
            {
                Com::printFLN( PSTR( "M3939 ViscosityTest starting ..." ) );
                
                int maxD = 10000;
                float maxE = 5.0f;
                float Inc = 0.1f;
                int maxRFill = 800;
                short StartTemp = 0;    //0=aus, der user muss das dann einstellen.
                short EndTemp = 0;
                
                if (pCommand->hasF() ){
                    if(pCommand->F < 32767 && pCommand->F > 0 && pCommand->F < (g_nEmergencyPauseDigitsMax*0.8)){
                        maxD = (int)pCommand->F;
                    }else{
                        Com::printFLN( PSTR( "M3939 [F] Digits ERROR" ) );
                    }
                }
                if (pCommand->hasE() ){
                    maxE = (float)pCommand->E;
                }
                if (pCommand->hasI() ){
                    Inc = (float)pCommand->I;
                }
                if (pCommand->hasR() ){
                    maxRFill = (float)pCommand->R;
                }               
                if (pCommand->hasS() ){
                    if(pCommand->S > UI_SET_MAX_EXTRUDER_TEMP) pCommand->S = UI_SET_MAX_EXTRUDER_TEMP;
                    if(pCommand->S < UI_SET_MIN_EXTRUDER_TEMP) pCommand->S = UI_SET_MIN_EXTRUDER_TEMP;
                    StartTemp = (short)pCommand->S;
                }
                if (pCommand->hasP() ){     
                    if(pCommand->P > UI_SET_MAX_EXTRUDER_TEMP) pCommand->P = UI_SET_MAX_EXTRUDER_TEMP;
                    if(pCommand->P < UI_SET_MIN_EXTRUDER_TEMP) pCommand->P = UI_SET_MIN_EXTRUDER_TEMP;
                    
                    if(pCommand->P < StartTemp) pCommand->P = StartTemp;
                    EndTemp = (short)pCommand->P;
                }               
                /*
                Com::printFLN( PSTR( "M3939 [F] Digits_max_in = " ) , maxD );
                Com::printFLN( PSTR( "M3939 [E] Extrusionspeed_max_in = " ) , maxE , 1 );
                Com::printFLN( PSTR( "M3939 [I] Extr. Increment_in = " ) , Inc , 2 );
                Com::printFLN( PSTR( "M3939 [R] RefillLimit Digits_in = " ) , maxRFill );
                Com::printFLN( PSTR( "M3939 [S] StartTemp = " ) , StartTemp );
                Com::printFLN( PSTR( "M3939 [P] EndTemp = " ) , EndTemp );
                Com::printFLN( PSTR( "M3939 If [S] and [P] = 0, then no temperature is set." ) );
                */
                startViscosityTest( maxD, maxE, Inc, StartTemp, EndTemp, maxRFill ); //E ist float, constraint in funktion!

                Com::printFLN( PSTR( "M3939 Ended!" ) );
                break;
            }
#endif // FEATURE_VISCOSITY_TEST

#if RESERVE_ANALOG_INPUTS
            case 3941: // 3941 reading optional temperature port X35 - Testfunction || by Nibbels
            {
                //Com::printFLN( PSTR( "M3941 TempReader starting ..." ) );
                TemperatureController* act = &optTempController;
                act->updateCurrentTemperature();
                Com::printFLN( PSTR( "Opt Temp: " ) , act->currentTemperatureC , 2 );
                //Com::printFLN( PSTR( "M3941 Ended!" ) );
                break;
            }
#endif // RESERVE_ANALOG_INPUTS

#if FEATURE_READ_STEPPER_STATUS
            case 3987: // M3987 reading motor driver and stall pins - Testfunction || by Nibbels
            {
                Com::printFLN( PSTR( "M3987 MotorStatus X-Y-Z-E0-E1 ..." ) );
                for(uint8_t driver = 1; driver <= 5; driver++){
                  readMotorStatus( driver );
                }
                break;
            }
#endif //FEATURE_READ_STEPPER_STATUS

#if FEATURE_USER_INT3
            case 3989: // M3989 : proof that dummy function/additional hardware button works! || by Nibbels
            {
                Com::printFLN( PSTR( "Request INT3:" ), g_uCOUNT_INT3 ); //evtl. je nach Anwendung entprellen nötig: wenn entprellen nötig, hilft evtl. interrupt nach ausführung sperren und mit dem watchdog-timer erneut entsperren.
                break;
            }
#endif //FEATURE_USER_INT3

            case 3998: // M3999 : proof that dummy function/additional hardware button works! || by Nibbels
            {
                Com::printF( PSTR( "File Write: " ) );
                char filename[] = "SVDat___.csv";
                
                sd.startWrite(filename);
                if(sd.savetosd){
                    sd.file.writeln_P(PSTR( "[um]" ));
                    sd.file.write_P(Com::tNewline);
                    sd.file.write_P(Com::tNewline);
                    sd.file.writeln_P(PSTR( "AM" ));
                    sd.file.writeFloat(1.98765f, 3, true);
                    sd.file.write_P(Com::tNewline);
                    sd.file.writeFloat((float)234000, 0, true);
                }else{
                    Com::printFLN( PSTR( "Write Error" ) );
                }
                sd.finishWrite();
                Com::printFLN( PSTR( "END" ) );
                break;
            }

#if FEATURE_READ_CALIPER
            case 3999: // M3999 : proof that dummy function/additional hardware button works! || by Nibbels
            {
                Com::printF( PSTR( "Caliper: " ), caliper_um );
                Com::printFLN( PSTR( "[um]" ) );
                break;
            }
#endif //FEATURE_READ_CALIPER
        }
    }

    return;

} // processCommand


void runStandardTasks( void )
{
    GCode*  pCode;
    GCode::readFromSerial();
    pCode = GCode::peekCurrentCommand();
    if( pCode )
    {
        Commands::executeGCode( pCode );
        pCode->popCurrentCommand();
    }
    Commands::checkForPeriodicalActions(); 
    return;

} // runStandardTasks


void queueTask( char task )
{
    while( PrintLine::linesCount >= MOVE_CACHE_SIZE )
    {
        // wait for a free entry in movement cache
        GCode::readFromSerial();
        Commands::checkForPeriodicalActions();
    }
  
    PrintLine::queueTask( task );
    return;

} // queueTask


extern void processButton( int nAction )
{

    switch( nAction )
    {
#if FEATURE_EXTENDED_BUTTONS
        case UI_ACTION_RF_HEAT_BED_UP:
        {
            //DO NOT MOVE Z: ALTER Z-OFFSET
            if( uid.menuLevel == 0 && uid.menuPos[0] == 1 ){ //wenn im Mod-Menü für Z-Offset/Matrix Sense-Offset/Limiter, dann anders!
                beep(1,4);
                // show that we are active
                previousMillisCmd = HAL::timeInMilliseconds();            
                long nTemp = Printer::ZOffset; //um --> mm*1000 
                nTemp -= Z_OFFSET_BUTTON_STEPS;
                //beim Unterschreiten von 0, soll 0 erreicht werden, sodass man nicht mit krummen Zahlen rumhantieren muss.
                if(nTemp < 0 && nTemp > -Z_OFFSET_BUTTON_STEPS) nTemp = 0;
        #if FEATURE_SENSIBLE_PRESSURE
                /* IDEE: Wenn automatisches Offset und wir korrigieren dagegen, soll erst dieses abgebaut werden */
                if(g_nSensiblePressureOffset > 0){ //aus: dann 0, an: dann > 0
                    //automatik hat das bett runtergefahren, wir fahren es mit negativem offset hoch.
                    //blöd: damit ist die automatik evtl. weiter am limit und kann nachfolgend nichts mehr tun.
                    //also erst ausgleichen! dann verändern.
                    if(g_nSensiblePressureOffset > Z_OFFSET_BUTTON_STEPS){
                        nTemp += Z_OFFSET_BUTTON_STEPS;
                        g_nSensiblePressureOffset -= Z_OFFSET_BUTTON_STEPS;
                    }else{
                        nTemp += g_nSensiblePressureOffset;
                        g_nSensiblePressureOffset = 0;
                    }     
                }
        #endif //FEATURE_SENSIBLE_PRESSURE                            
                if( nTemp < -(HEAT_BED_Z_COMPENSATION_MAX_MM * 1000) ) nTemp = -(HEAT_BED_Z_COMPENSATION_MAX_MM * 1000);
                if( nTemp > (HEAT_BED_Z_COMPENSATION_MAX_MM * 1000) ) nTemp = (HEAT_BED_Z_COMPENSATION_MAX_MM * 1000);                
                Printer::ZOffset = nTemp;
        #if FEATURE_SENSIBLE_PRESSURE
                g_staticZSteps = long(( (Printer::ZOffset+g_nSensiblePressureOffset) * Printer::axisStepsPerMM[Z_AXIS] ) / 1000);
        #else
                g_staticZSteps = long(( Printer::ZOffset * Printer::axisStepsPerMM[Z_AXIS] ) / 1000);
        #endif //FEATURE_SENSIBLE_PRESSURE
            #if FEATURE_AUTOMATIC_EEPROM_UPDATE
                if( HAL::eprGetInt32( EPR_RF_Z_OFFSET ) != Printer::ZOffset )
                {
                    HAL::eprSetInt32( EPR_RF_Z_OFFSET, Printer::ZOffset );
                    EEPROM::updateChecksum();
                }
            #endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
            } //ELSE DO MOVE Z: 
            else
            {
                nextPreviousZAction( -1 );
            }
            break;
        }
        case UI_ACTION_RF_HEAT_BED_DOWN:
        {            
            //DO NOT MOVE Z: ALTER Z-OFFSET
            if( uid.menuLevel == 0 && uid.menuPos[0] == 1 ){ //wenn im Mod-Menü für Z-Offset/Matrix Z-Offset/Matrix Sense-Offset/Limiter, dann anders!
                beep(1,4);
                // show that we are active
                previousMillisCmd = HAL::timeInMilliseconds();
            
                long nTemp = Printer::ZOffset; //um --> mm*1000 
                nTemp += Z_OFFSET_BUTTON_STEPS;
                //beim Überschreiten von 0, soll 0 erreicht werden, sodass man nicht mit krummen Zahlen rumhantieren muss.
                if(nTemp < Z_OFFSET_BUTTON_STEPS && nTemp > 0) nTemp = 0;
                if( nTemp < -(HEAT_BED_Z_COMPENSATION_MAX_MM * 1000) ) nTemp = -(HEAT_BED_Z_COMPENSATION_MAX_MM * 1000);
                if( nTemp > (HEAT_BED_Z_COMPENSATION_MAX_MM * 1000) ) nTemp = (HEAT_BED_Z_COMPENSATION_MAX_MM * 1000);
                Printer::ZOffset = nTemp;
        #if FEATURE_SENSIBLE_PRESSURE
                g_staticZSteps = long(( (Printer::ZOffset+g_nSensiblePressureOffset) * Printer::axisStepsPerMM[Z_AXIS] ) / 1000);
        #else
                g_staticZSteps = long(( Printer::ZOffset * Printer::axisStepsPerMM[Z_AXIS] ) / 1000);
        #endif //FEATURE_SENSIBLE_PRESSURE
            #if FEATURE_AUTOMATIC_EEPROM_UPDATE
                if( HAL::eprGetInt32( EPR_RF_Z_OFFSET ) != Printer::ZOffset )
                {
                    HAL::eprSetInt32( EPR_RF_Z_OFFSET, Printer::ZOffset );
                    EEPROM::updateChecksum();
                }
            #endif // FEATURE_AUTOMATIC_EEPROM_UPDATE
            } //ELSE DO MOVE Z: 
            else
            {
                nextPreviousZAction( 1 );
            }
            break;
        }
        case UI_ACTION_RF_EXTRUDER_OUTPUT:
        {
            if( uid.menuLevel == 0 && uid.menuPos[0] == 1 ){ //wenn im Mod-Menü für Z-Offset/Matrix Sense-Offset/Limiter, dann anders!
                //we are in the Mod menu
                //so dont retract, change the speed of the print to a lower speed instead of retracting:
                //limits handled by change-function!
                Commands::changeFeedrateMultiply(Printer::feedrateMultiply + 1);
                beep(1,4);
            }else{
    #if !EXTRUDER_ALLOW_COLD_MOVE
                if( Extruder::current->tempControl.currentTemperatureC < UI_SET_MIN_EXTRUDER_TEMP )
                {
                    // we do not allow to move the extruder in case it is not heated up enough
                    if( Printer::debugErrors() )
                    {
                        Com::printFLN( PSTR( "processButton(): extruder output: aborted" ) );
                    }

                    showError( (void*)ui_text_extruder, (void*)ui_text_operation_denied );
                    break;
                }
    #endif // !EXTRUDER_ALLOW_COLD_MOVE

                // show that we are active
                previousMillisCmd = HAL::timeInMilliseconds();

                if( (unsigned long)abs(Printer::directPositionTargetSteps[E_AXIS] - Printer::directPositionCurrentSteps[E_AXIS]) <= g_nManualSteps[E_AXIS] )
                {
                    // we are printing at the moment - use direct steps
                    if( Printer::debugInfo() )
                    {
                        Com::printF( PSTR( "processButton(): extruder output: " ), (int)g_nManualSteps[E_AXIS] );
                        Com::printFLN( PSTR( " [steps]" ) );
                    }
                    InterruptProtectedBlock noInts; //HAL::forbidInterrupts();
                    Extruder::enable();
                    Printer::directPositionTargetSteps[E_AXIS] += g_nManualSteps[E_AXIS];
                    noInts.unprotect(); //HAL::allowInterrupts();

                    if( Printer::debugInfo() )
                    {
                        Com::printF( PSTR( "Button: E-steps: " ), (int)Printer::directPositionTargetSteps[E_AXIS] );
                        Com::printFLN( PSTR( " [steps]" ) );
                    }
                    
                    //In case of double pause and in case we tempered with the retract, we dont want to drive the E-Axis back to some old location - that much likely causes emergency block.
                    g_nContinueSteps[E_AXIS] = 0;
                }
            }
            break;
        }
        case UI_ACTION_RF_EXTRUDER_RETRACT:
        {           
            if( uid.menuLevel == 0 && uid.menuPos[0] == 1 ){ //wenn im Mod-Menü für Z-Offset/Matrix Sense-Offset/Limiter, dann anders!
                //we are in the Mod menu
                //so dont retract, change the speed of the print to a lower speed instead of retracting:
                //limits handled by change-function!
                Commands::changeFeedrateMultiply(Printer::feedrateMultiply - 1);
                beep(1,4);
            }else{
                //we are sonewhere "normal"
    #if !EXTRUDER_ALLOW_COLD_MOVE
                if( Extruder::current->tempControl.currentTemperatureC < UI_SET_MIN_EXTRUDER_TEMP )
                {
                    // we do not allow to move the extruder in case it is not heated up enough
                    if( Printer::debugErrors() )
                    {
                        Com::printFLN( PSTR( "processButton(): extruder retract: aborted" ) );
                    }
                    showError( (void*)ui_text_extruder, (void*)ui_text_operation_denied );
                    break;
                }
    #endif // !EXTRUDER_ALLOW_COLD_MOVE

                // show that we are active
                previousMillisCmd = HAL::timeInMilliseconds();

                if( (unsigned long)abs(Printer::directPositionTargetSteps[E_AXIS] - Printer::directPositionCurrentSteps[E_AXIS]) <= g_nManualSteps[E_AXIS] )
                {
                    // we are printing at the moment - use direct steps
                    if( Printer::debugInfo() )
                    {
                        Com::printF( PSTR( "processButton(): extruder retract: " ), (int)g_nManualSteps[E_AXIS] );
                        Com::printFLN( PSTR( " [steps]" ) );
                    }

                    InterruptProtectedBlock noInts; //HAL::forbidInterrupts();
                    Extruder::enable();
                    Printer::directPositionTargetSteps[E_AXIS] -= g_nManualSteps[E_AXIS];
                    noInts.unprotect(); //HAL::allowInterrupts();

                    if( Printer::debugInfo() )
                    {
                        Com::printF( PSTR( "processButton(): current manual E steps: " ), (int)Printer::directPositionTargetSteps[E_AXIS] );
                        Com::printFLN( PSTR( " [steps]" ) );
                    }
                    
                    //In case of double pause and in case we tempered with the retract, we dont want to drive the E-Axis back to some old location - that much likely causes emergency block.
                    g_nContinueSteps[E_AXIS] = 0;
                }
            }
            break;
        }
#if FEATURE_PAUSE_PRINTING
        case UI_ACTION_RF_PAUSE:
        {
            pausePrint();
            break;
        }
        case UI_ACTION_RF_CONTINUE:
        {
            continuePrint();
            break;
        }
#endif // FEATURE_PAUSE_PRINTING

#endif // FEATURE_EXTENDED_BUTTONS

#if FEATURE_HEAT_BED_Z_COMPENSATION
        case UI_ACTION_RF_SCAN_HEAT_BED:
        {
#if FEATURE_PRECISE_HEAT_BED_SCAN
            g_nHeatBedScanMode = 0;
#endif // FEATURE_PRECISE_HEAT_BED_SCAN

            startHeatBedScan();
            //gehe zurück und zeige dem User was passiert.
            uid.executeAction(UI_ACTION_TOP_MENU);
            uid.menuPos[0] = 0;
            break;
        }
#if FEATURE_PRECISE_HEAT_BED_SCAN
        case UI_ACTION_RF_SCAN_HEAT_BED_PLA:
        {
            g_nHeatBedScanMode = HEAT_BED_SCAN_MODE_PLA;
            startHeatBedScan();
            //gehe zurück und zeige dem User was passiert.
            uid.executeAction(UI_ACTION_TOP_MENU);
            uid.menuPos[0] = 0;
            break;
        }
        case UI_ACTION_RF_SCAN_HEAT_BED_ABS:
        {
            g_nHeatBedScanMode = HEAT_BED_SCAN_MODE_ABS;
            startHeatBedScan();
            //gehe zurück und zeige dem User was passiert.
            uid.executeAction(UI_ACTION_TOP_MENU);
            uid.menuPos[0] = 0;
            break;
        }        
#endif // FEATURE_PRECISE_HEAT_BED_SCAN

#if FEATURE_WORK_PART_Z_COMPENSATION
        case UI_ACTION_RF_SCAN_WORK_PART:
        {
            startWorkPartScan( 0 );
            break;
        }
        case UI_ACTION_RF_SET_SCAN_XY_START:
        {
            setScanXYStart();
            break;
        }
        case UI_ACTION_RF_SET_SCAN_XY_END:
        {
            setScanXYEnd();
            break;
        }
#endif // FEATURE_WORK_PART_Z_COMPENSATION
#endif // FEATURE_HEAT_BED_Z_COMPENSATION

#if FEATURE_OUTPUT_FINISHED_OBJECT
        case UI_ACTION_RF_OUTPUT_OBJECT:
        {
            outputObject();
            break;
        }
#endif // FEATURE_OUTPUT_FINISHED_OBJECT

#if FEATURE_FIND_Z_ORIGIN
        case UI_ACTION_RF_FIND_Z_ORIGIN:
        {
            startFindZOrigin();
            break;
        }
#endif // FEATURE_FIND_Z_ORIGIN

#if FEATURE_PARK
        case UI_ACTION_RF_PARK:
        {
            parkPrinter();
            break;
        }
#endif // FEATURE_PARK
    }
    return;

} // processButton


void nextPreviousXAction( int8_t increment )
{
    long    steps;

    if( PrintLine::direct.stepsRemaining )
    {
        return;
    }

    if( Printer::processAsDirectSteps() )
    {
        // this operation is not allowed while a printing/milling is in progress
        //wenn man schnell den knopf klickt, soll man nicht im showerror landen, das nervt. Man sieht das ja, was der verfährt.
        //showError( (void*)ui_text_x_axis, (void*)ui_text_operation_denied );
        return;
    }

#if !FEATURE_ALLOW_UNKNOWN_POSITIONS
    if(!Printer::isAxisHomed(X_AXIS))
    {
        // we do not allow unknown positions and the printer is not homed, thus we do not move
        showError( (void*)ui_text_x_axis, (void*)ui_text_home_unknown );
        return;
    }
#endif // !FEATURE_ALLOW_UNKNOWN_POSITIONS

    if(increment<0 && Printer::isXMinEndstopHit())
    {
        // we shall move to the left but the x-min-endstop is hit already, so we do nothing
        showError( (void*)ui_text_x_axis, (void*)ui_text_min_reached );
        return;
    }

    if(increment>0 && (Printer::lengthMM[X_AXIS] - Printer::targetXPosition()) < 0.1)
    {
        // we shall move to the right but the end of the x-axis has been reached already, so we do nothing
        showError( (void*)ui_text_x_axis, (void*)ui_text_max_reached );
        return;
    }

    // show that we are active
    previousMillisCmd = HAL::timeInMilliseconds();

    switch( Printer::moveMode[X_AXIS] )
    {
        case MOVE_MODE_SINGLE_STEPS:
        {
            long    Temp;


            steps = g_nManualSteps[X_AXIS] * increment;

            InterruptProtectedBlock noInts; //HAL::forbidInterrupts();
            Temp =  Printer::directPositionTargetSteps[X_AXIS] + steps;
            Temp += Printer::queuePositionCurrentSteps[X_AXIS];

            noInts.unprotect(); //HAL::allowInterrupts();
            if( increment < 0 && Temp < 0 )
            {
                // do not allow to drive the head against the left border
                if( Printer::debugErrors() )
                {
                    Com::printFLN( PSTR( "nextPreviousXAction(): moving x aborted (safety stop)") );
                }

                showError( (void*)ui_text_x_axis, (void*)ui_text_operation_denied );
                break;
            }
            else
            {
                Printer::unsetAllSteppersDisabled();
                Printer::enableXStepper();

                noInts.protect(); //HAL::forbidInterrupts();
                Printer::directPositionTargetSteps[X_AXIS] += steps;
                /*if( Printer::directPositionTargetSteps[Z_AXIS] < EXTENDED_BUTTONS_Z_MIN )
                {
                    Printer::directPositionTargetSteps[Z_AXIS] = EXTENDED_BUTTONS_Z_MIN;
                }*/
                noInts.unprotect(); //HAL::allowInterrupts();

                if( Printer::debugInfo() )
                {
                    Com::printF( PSTR( "nextPreviousXAction(): current manual x steps: " ), Printer::directPositionTargetSteps[X_AXIS] );
                    Com::printFLN( PSTR( " [steps]" ) );
                }
            }
            break;
        }
        case MOVE_MODE_SINGLE_MOVE:
        {
            if( PrintLine::direct.stepsRemaining )
            {
                // we are moving already, there is nothing more to do
                return;
            }

            if( increment < 0 ) steps = -(long)((Printer::lengthMM[X_AXIS] + 5) * Printer::axisStepsPerMM[X_AXIS]);
            else                steps =  (long)((Printer::lengthMM[X_AXIS] + 5) * Printer::axisStepsPerMM[X_AXIS]);

            InterruptProtectedBlock noInts; //HAL::forbidInterrupts();
            Printer::directPositionTargetSteps[X_AXIS] = Printer::directPositionCurrentSteps[X_AXIS] + steps;
            Printer::directPositionTargetSteps[Y_AXIS] = Printer::directPositionCurrentSteps[Y_AXIS];
            Printer::directPositionTargetSteps[Z_AXIS] = Printer::directPositionCurrentSteps[Z_AXIS];

            PrintLine::prepareDirectMove();
            PrintLine::direct.task = TASK_MOVE_FROM_BUTTON;
            noInts.unprotect(); //HAL::allowInterrupts();
            break;
        }
        case MOVE_MODE_1_MM:
        {
            long Temp = Printer::directPositionTargetSteps[X_AXIS];
            Temp += Printer::queuePositionCurrentSteps[X_AXIS]; //homed oder nicht homed, das ist hier egal, nicht gehomed kann ich das sowieso nicht prüfen.
			Temp += (long)(increment * 1 * Printer::axisStepsPerMM[X_AXIS]);
			if(Temp <= long(Printer::lengthMM[X_AXIS] * Printer::axisStepsPerMM[X_AXIS])) Printer::setDestinationStepsFromMenu( 1 * increment, 0, 0 );
			else showError( (void*)ui_text_x_axis, (void*)ui_text_max_reached );
            break;
        }
        case MOVE_MODE_10_MM:
        {
            long Temp = Printer::directPositionTargetSteps[X_AXIS];
            Temp += Printer::queuePositionCurrentSteps[X_AXIS]; //homed oder nicht homed, das ist hier egal, nicht gehomed kann ich das sowieso nicht prüfen.
			Temp += (long)(increment * 10 * Printer::axisStepsPerMM[X_AXIS]);
			if(Temp <= long(Printer::lengthMM[X_AXIS] * Printer::axisStepsPerMM[X_AXIS])) Printer::setDestinationStepsFromMenu( 10 * increment, 0, 0 );
			else showError( (void*)ui_text_x_axis, (void*)ui_text_max_reached );
            break;
        }
        case MOVE_MODE_50_MM:
        {
            long Temp = Printer::directPositionTargetSteps[X_AXIS];
            Temp += Printer::queuePositionCurrentSteps[X_AXIS]; //homed oder nicht homed, das ist hier egal, nicht gehomed kann ich das sowieso nicht prüfen.
			Temp += (long)(increment * 50 * Printer::axisStepsPerMM[X_AXIS]);
			if(Temp <= long(Printer::lengthMM[X_AXIS] * Printer::axisStepsPerMM[X_AXIS])) Printer::setDestinationStepsFromMenu( 50 * increment, 0, 0 );
			else showError( (void*)ui_text_x_axis, (void*)ui_text_max_reached );
            break;
        }
    }

    return;

} // nextPreviousXAction


void nextPreviousYAction( int8_t increment )
{
    long    steps;

    if( PrintLine::direct.stepsRemaining )
    {
        // we are moving already, there is nothing more to do
        return;
    }

    if( Printer::processAsDirectSteps() )
    {
        // this operation is not allowed while a printing/milling is in progress
        if( Printer::debugErrors() )
        {
            Com::printFLN( PSTR( "nextPreviousYAction(): moving y aborted (not allowed)" ) );
        }
        //wenn man schnell den knopf klickt, soll man nicht im showerror landen, das nervt. Man sieht das ja, was der verfährt.
        //showError( (void*)ui_text_y_axis, (void*)ui_text_operation_denied );
        return;
    }

#if !FEATURE_ALLOW_UNKNOWN_POSITIONS
    if(!Printer::isAxisHomed(Y_AXIS))
    {
        // we do not allow unknown positions and the printer is not homed, thus we do not move
        if( Printer::debugErrors() )
        {
            Com::printFLN( PSTR( "nextPreviousYAction(): moving y aborted (not homed)" ) );
        }

        showError( (void*)ui_text_y_axis, (void*)ui_text_home_unknown );
        return;
    }
#endif // !FEATURE_ALLOW_UNKNOWN_POSITIONS

    if(increment<0 && Printer::isYMinEndstopHit())
    {
        // we shall move to the back but the y-min-endstop is hit already, so we do nothing
        showError( (void*)ui_text_y_axis, (void*)ui_text_min_reached );
        return;
    }

    if(increment>0 && (Printer::lengthMM[Y_AXIS] - Printer::targetYPosition()) < 0.1)
    {
        // we shall move to the front but the end of the y-axis has been reached already, so we do nothing
        showError( (void*)ui_text_y_axis, (void*)ui_text_max_reached );
        return;
    }

    // show that we are active
    previousMillisCmd = HAL::timeInMilliseconds();

    switch( Printer::moveMode[Y_AXIS] )
    {
        case MOVE_MODE_SINGLE_STEPS:
        {
            long    Temp;


            steps = g_nManualSteps[Y_AXIS] * increment;

            InterruptProtectedBlock noInts; //HAL::forbidInterrupts();
            Temp =  Printer::directPositionTargetSteps[Y_AXIS] + steps;
            Temp += Printer::queuePositionCurrentSteps[Y_AXIS];

            noInts.unprotect(); //HAL::allowInterrupts();
            if( increment < 0 && Temp < 0 )
            {
                // do not allow to drive the bed against the back border
                if( Printer::debugErrors() )
                {
                    Com::printFLN( PSTR( "nextPreviousYAction(): moving y aborted (safety stop)") );
                }

                showError( (void*)ui_text_y_axis, (void*)ui_text_operation_denied );
                break;
            }
            else
            {
                Printer::unsetAllSteppersDisabled();
                Printer::enableYStepper();

                noInts.protect(); //HAL::forbidInterrupts();
                Printer::directPositionTargetSteps[Y_AXIS] += steps;
                /*if( Printer::directPositionTargetSteps[Z_AXIS] < EXTENDED_BUTTONS_Z_MIN )
                {
                    Printer::directPositionTargetSteps[Z_AXIS] = EXTENDED_BUTTONS_Z_MIN;
                }*/
                noInts.unprotect(); //HAL::allowInterrupts();

                if( Printer::debugInfo() )
                {
                    Com::printF( PSTR( "nextPreviousYAction(): current manual y steps: " ), Printer::directPositionTargetSteps[Y_AXIS] );
                    Com::printFLN( PSTR( " [steps]" ) );
                }
            }
            break;
        }
        case MOVE_MODE_SINGLE_MOVE:
        {
            if( PrintLine::direct.stepsRemaining )
            {
                // we are moving already, there is nothing more to do
                return;
            }

            if( increment < 0 ) steps = -(long)((Printer::lengthMM[Y_AXIS] + 5) * Printer::axisStepsPerMM[Y_AXIS]);
            else                steps =  (long)((Printer::lengthMM[Y_AXIS] + 5) * Printer::axisStepsPerMM[Y_AXIS]);

            InterruptProtectedBlock noInts; //HAL::forbidInterrupts();
            Printer::directPositionTargetSteps[X_AXIS] = Printer::directPositionCurrentSteps[X_AXIS];
            Printer::directPositionTargetSteps[Y_AXIS] = Printer::directPositionCurrentSteps[Y_AXIS] + steps;
            Printer::directPositionTargetSteps[Z_AXIS] = Printer::directPositionCurrentSteps[Z_AXIS];

            PrintLine::prepareDirectMove();
            PrintLine::direct.task = TASK_MOVE_FROM_BUTTON;
            noInts.unprotect(); //HAL::allowInterrupts();
            break;
        }
		case MOVE_MODE_1_MM:
        {
            long Temp = Printer::directPositionTargetSteps[Y_AXIS];
            Temp += Printer::queuePositionCurrentSteps[Y_AXIS]; //homed oder nicht homed, das ist hier egal, nicht gehomed kann ich das sowieso nicht prüfen.
			Temp += (long)(increment * 1 * Printer::axisStepsPerMM[Y_AXIS]);
			if(Temp <= long(Printer::lengthMM[Y_AXIS] * Printer::axisStepsPerMM[Y_AXIS])) Printer::setDestinationStepsFromMenu( 0, 1 * increment, 0 );
			else showError( (void*)ui_text_y_axis, (void*)ui_text_max_reached );
            break;
        }
        case MOVE_MODE_10_MM:
        {
            long Temp = Printer::directPositionTargetSteps[Y_AXIS];
            Temp += Printer::queuePositionCurrentSteps[Y_AXIS]; //homed oder nicht homed, das ist hier egal, nicht gehomed kann ich das sowieso nicht prüfen.
			Temp += (long)(increment * 10 * Printer::axisStepsPerMM[Y_AXIS]);
			if(Temp <= long(Printer::lengthMM[Y_AXIS] * Printer::axisStepsPerMM[Y_AXIS])) Printer::setDestinationStepsFromMenu( 0, 10 * increment, 0 );
			else showError( (void*)ui_text_y_axis, (void*)ui_text_max_reached );
            break;
        }
        case MOVE_MODE_50_MM:
        {
            long Temp = Printer::directPositionTargetSteps[Y_AXIS];
            Temp += Printer::queuePositionCurrentSteps[Y_AXIS]; //homed oder nicht homed, das ist hier egal, nicht gehomed kann ich das sowieso nicht prüfen.
			Temp += (long)(increment * 50 * Printer::axisStepsPerMM[Y_AXIS]);
			if(Temp <= long(Printer::lengthMM[Y_AXIS] * Printer::axisStepsPerMM[Y_AXIS])) Printer::setDestinationStepsFromMenu( 0, 50 * increment, 0 );
			else showError( (void*)ui_text_y_axis, (void*)ui_text_max_reached );
            break;
        }
    }

    return;

} // nextPreviousYAction


void nextPreviousZAction( int8_t increment )
{
    long    steps;
    char    moveMode;
    
    if( PrintLine::direct.stepsRemaining )
    {
        // we are moving already, there is nothing more to do
        return;
    }

#if !FEATURE_ALLOW_UNKNOWN_POSITIONS
    if(!Printer::isAxisHomed(Z_AXIS))
    {
        // we do not allow unknown positions and the printer is not homed, thus we do not move
        if( Printer::debugErrors() )
        {
            Com::printFLN( PSTR( "nextPreviousZAction(): moving z aborted (not homed)" ) );
        }

        showError( (void*)ui_text_z_axis, (void*)ui_text_home_unknown );
        return;
    }
#endif // !FEATURE_ALLOW_UNKNOWN_POSITIONS

#if FEATURE_CONFIGURABLE_Z_ENDSTOPS
    if( Printer::ZEndstopUnknown )
    {
        if( Printer::debugErrors() )
        {
            Com::printFLN( PSTR( "nextPreviousZAction(): moving z aborted (perform a z-homing first)" ) );
        }

        showError( (void*)ui_text_z_axis, (void*)ui_text_home_unknown );
        return;
    }
#endif // FEATURE_CONFIGURABLE_Z_ENDSTOPS

    if(increment>0 && Printer::isZMaxEndstopHit())
    {
        // we shall move downwards but the z-max-endstop is hit already, so we do nothing
        if( Printer::debugErrors() )
        {
            Com::printFLN( PSTR( "nextPreviousZAction(): moving z aborted (max reached)" ) );
        }

        showError( (void*)ui_text_z_axis, (void*)ui_text_max_reached );
        return;
    }

    if(increment>0 && (Printer::lengthMM[Z_AXIS] - Printer::targetZPosition()) < 0.1)
    {
        // we shall move downwards but the end of the z-axis has been reached already, so we do nothing
        if( Printer::debugErrors() )
        {
            Com::printFLN( PSTR( "nextPreviousZAction(): moving z aborted (z-max reached)" ) );
        }

        showError( (void*)ui_text_z_axis, (void*)ui_text_max_reached );
        return;
    }

    // show that we are active
    previousMillisCmd = HAL::timeInMilliseconds();
    
    moveMode = Printer::moveMode[Z_AXIS];
    if( Printer::processAsDirectSteps() )
    {
        // in case we are printing/milling at the moment, only the single step movements are allowed
        moveMode = MOVE_MODE_SINGLE_STEPS;
    }
    
    /*if(!Printer::isHomed() && moveMode > MOVE_MODE_SINGLE_MOVE)
    {
        moveMode = MOVE_MODE_SINGLE_MOVE;
    }*/
    if(uid.lastButtonAction == UI_ACTION_RF_HEAT_BED_DOWN || uid.lastButtonAction == UI_ACTION_RF_HEAT_BED_UP){ 
        //es wäre evtl. cooler die variable durchzuschleifen, aber so gehts auch!
        //VORSICHT: ES könnte evtl. sein dass sich lastbuttonaction während der ausführung ändert. (??)
    
        //sollte diese Aktion durch die Knöpfe ausgeführt worden sein, dann in der Sprungweite limitieren:
        if( PrintLine::linesCount )
        {
            // there is some printing in progress at the moment - do not allow single move in this case
            moveMode = MOVE_MODE_SINGLE_STEPS;    
        }else{
            moveMode = MOVE_MODE_SINGLE_MOVE;
        }
    }
    //Limits für die Bewegung in Z by Nibbels
    if(increment>0 && Printer::isZMaxEndstopHit())
    {
        //fall down to Single Steps @Endstop
        moveMode = MOVE_MODE_SINGLE_STEPS;
    }
    if(increment<0 && Printer::isZMinEndstopHit()){
        //fall down to Single Steps @Endstop
        moveMode = MOVE_MODE_SINGLE_STEPS;        
    }

    switch( moveMode )
    {
        case MOVE_MODE_SINGLE_STEPS:
        {
            long    Temp; //bringt nur was wenn homed
            steps = g_nManualSteps[Z_AXIS] * increment;

            InterruptProtectedBlock noInts; //HAL::forbidInterrupts();
            Temp =  Printer::directPositionTargetSteps[Z_AXIS] + steps;
            Temp += Printer::queuePositionCurrentSteps[Z_AXIS];

#if FEATURE_HEAT_BED_Z_COMPENSATION || FEATURE_WORK_PART_Z_COMPENSATION
            Temp += Printer::compensatedPositionCurrentStepsZ;
#endif // FEATURE_HEAT_BED_Z_COMPENSATION || FEATURE_WORK_PART_Z_COMPENSATION

            noInts.unprotect(); //HAL::allowInterrupts();
            if( increment < 0 && Temp < -Z_OVERRIDE_MAX && Printer::isZMinEndstopHit() )
            {
                // do not allow to drive the bed into the extruder
                showError( (void*)ui_text_z_axis, (Printer::isAxisHomed(Z_AXIS) ? (void*)ui_text_min_reached : (void*)ui_text_min_reached_unhomed) );
                break;
            }
            else
            {
                previousMillisCmd = HAL::timeInMilliseconds();
                Printer::unsetAllSteppersDisabled();
                Printer::enableZStepper();

                noInts.protect(); //HAL::forbidInterrupts();
                Printer::directPositionTargetSteps[Z_AXIS] += steps;
                
                //Nibbels: Diese Eingrenzung scheint mir völlig sinnlos und fehl am Platz:
                //Wir sind hier bei Single-Steps und mit Homing gibts Z_OVERRIDE_MAX
                //Ohne Homing gibts den Z-Endstop Min. Diese Zahl Z=.... stimmt ohne Homing nicht!
                /*if( increment < 0 && Printer::directPositionTargetSteps[Z_AXIS] < EXTENDED_BUTTONS_Z_MIN )
                {
                    Printer::directPositionTargetSteps[Z_AXIS] = Printer::directPositionCurrentSteps[Z_AXIS];

                    if( Printer::debugErrors() )
                    {
                        Com::printFLN( PSTR( "nextPreviousZAction(): moving z aborted (min reached)" ) );
                    }
                    showError( (void*)ui_text_z_axis, (void*)ui_text_min_reached );
                }
                if( increment > 0 && Printer::directPositionTargetSteps[Z_AXIS] > EXTENDED_BUTTONS_Z_MAX )
                {
                    Printer::directPositionTargetSteps[Z_AXIS] = Printer::directPositionCurrentSteps[Z_AXIS];

                    if( Printer::debugErrors() )
                    {
                        Com::printFLN( PSTR( "nextPreviousZAction(): moving z aborted (max reached)" ) );
                    }
                    showError( (void*)ui_text_z_axis, (void*)ui_text_max_reached );
                }*/
                noInts.unprotect(); //HAL::allowInterrupts();

                if( Printer::debugInfo() )
                {
                    Com::printF( PSTR( "nextPreviousZAction(): current manual z steps: " ), Printer::directPositionTargetSteps[Z_AXIS] );
                    Com::printFLN( PSTR( " [steps]" ) );
                }
            }
            break;
        }
        case MOVE_MODE_SINGLE_MOVE:
        {
            if( PrintLine::direct.stepsRemaining )
            {
                // we are moving already, there is nothing more to do
                return;
            }

            if(increment<0 && Printer::isZMinEndstopHit())
            {
                // we shall move upwards but the z-min-endstop is hit already, so we do nothing
                if( Printer::debugErrors() )
                {
                    Com::printFLN( PSTR( "nextPreviousZAction(): moving z aborted (min reached)" ) );
                }

                showError( (void*)ui_text_z_axis, (Printer::isAxisHomed(Z_AXIS) ? (void*)ui_text_min_reached : (void*)ui_text_min_reached_unhomed) );
                return;
            }

            if( increment < 0 ) steps = -(long)((Printer::lengthMM[Z_AXIS] + 5) * Printer::axisStepsPerMM[Z_AXIS]);
            else                steps =  (long)((Printer::lengthMM[Z_AXIS] + 5) * Printer::axisStepsPerMM[Z_AXIS]);

            InterruptProtectedBlock noInts; //HAL::forbidInterrupts();
            Printer::directPositionTargetSteps[X_AXIS] = Printer::directPositionCurrentSteps[X_AXIS];
            Printer::directPositionTargetSteps[Y_AXIS] = Printer::directPositionCurrentSteps[Y_AXIS];
            Printer::directPositionTargetSteps[Z_AXIS] = Printer::directPositionCurrentSteps[Z_AXIS] + steps;

            PrintLine::prepareDirectMove();
            PrintLine::direct.task = TASK_MOVE_FROM_BUTTON;
            noInts.unprotect(); //HAL::allowInterrupts();
            break;
        }
        case MOVE_MODE_1_MM:
        {
            if(increment<0 && Printer::isZMinEndstopHit())
            {
                // we shall move upwards but the z-min-endstop is hit already, so we do nothing
                if( Printer::debugErrors() )
                {
                    Com::printFLN( PSTR( "nextPreviousZAction(): moving z aborted (min reached)" ) );
                }

                showError( (void*)ui_text_z_axis, (Printer::isAxisHomed(Z_AXIS) ? (void*)ui_text_min_reached : (void*)ui_text_min_reached_unhomed) );
                return;
            }        
            float currentZmm = (float)Printer::currentZPositionSteps() * Printer::invAxisStepsPerMM[Z_AXIS] ; //z achse bezogen auf Z-Schalter. Scale Z-Min
            Com::printFLN( PSTR( "1mm: " ) , currentZmm , 3 );
            Com::printFLN( PSTR( "increment: " ) , (1.0f-currentZmm) * increment , 3 );
            
            if(Printer::isAxisHomed(Z_AXIS) && increment < 0 && currentZmm < 1.0f){
#if FEATURE_MILLING_MODE
                if( Printer::operatingMode == OPERATING_MODE_PRINT )
                {
#endif // FEATURE_MILLING_MODE
                    //Nur so weit runterfahren, wie man über 0 ist. Denn da ist beim Printermode homed der Endschalter.
                    if(currentZmm > 0) Printer::setDestinationStepsFromMenu( 0, 0, currentZmm * increment );
                    else Printer::setDestinationStepsFromMenu( 0, 0, g_nManualSteps[Z_AXIS]*Printer::invAxisStepsPerMM[Z_AXIS] * increment );
#if FEATURE_MILLING_MODE
                }
                else
                {
                    //Beim Milling ist Z=0 das obere des Bauteils. Dann geht Z - ins Bauteil rein. Daher ist überfahren ok.
                    Printer::setDestinationStepsFromMenu( 0, 0, 1 * increment );    
                }
#endif // FEATURE_MILLING_MODE
            }else{
                Printer::setDestinationStepsFromMenu( 0, 0, 1 * increment );                
            }
            
            break;
        }
        case MOVE_MODE_10_MM:
        {
            if(increment<0 && Printer::isZMinEndstopHit())
            {
                // we shall move upwards but the z-min-endstop is hit already, so we do nothing
                if( Printer::debugErrors() )
                {
                    Com::printFLN( PSTR( "nextPreviousZAction(): moving z aborted (min reached)" ) );
                }

                showError( (void*)ui_text_z_axis, (Printer::isAxisHomed(Z_AXIS) ? (void*)ui_text_min_reached : (void*)ui_text_min_reached_unhomed) );
                return;
            }
            float currentZmm = (float)Printer::currentZPositionSteps() * Printer::invAxisStepsPerMM[Z_AXIS] ; //z achse bezogen auf Z-Schalter. Scale Z-Min
            if(Printer::isAxisHomed(Z_AXIS) && increment < 0 && currentZmm < 10.0f){
#if FEATURE_MILLING_MODE
                if( Printer::operatingMode == OPERATING_MODE_PRINT )
                {
#endif // FEATURE_MILLING_MODE
                    //Nur so weit runterfahren, wie man über 0 ist. Denn da ist beim Printermode homed der Endschalter.
                    if(currentZmm > 0) Printer::setDestinationStepsFromMenu( 0, 0, currentZmm * increment );
                    else Printer::setDestinationStepsFromMenu( 0, 0, g_nManualSteps[Z_AXIS]*Printer::invAxisStepsPerMM[Z_AXIS] * increment );
#if FEATURE_MILLING_MODE
                }
                else
                {
                    //Beim Milling ist Z=0 das obere des Bauteils. Dann geht Z - ins Bauteil rein. Daher ist überfahren ok.
                    Printer::setDestinationStepsFromMenu( 0, 0, 10 * increment );    
                }
#endif // FEATURE_MILLING_MODE
            }else{
                Printer::setDestinationStepsFromMenu( 0, 0, 10 * increment );                
            }
            break;
        }
        case MOVE_MODE_50_MM:
        {
            if(increment<0 && Printer::isZMinEndstopHit())
            {
                // we shall move upwards but the z-min-endstop is hit already, so we do nothing
                if( Printer::debugErrors() )
                {
                    Com::printFLN( PSTR( "nextPreviousZAction(): moving z aborted (min reached)" ) );
                }

                showError( (void*)ui_text_z_axis, (Printer::isAxisHomed(Z_AXIS) ? (void*)ui_text_min_reached : (void*)ui_text_min_reached_unhomed) );
                return;
            }
            float currentZmm = (float)Printer::currentZPositionSteps() * Printer::invAxisStepsPerMM[Z_AXIS] ; //z achse bezogen auf Z-Schalter. Scale Z-Min
            if(Printer::isAxisHomed(Z_AXIS) && increment < 0 && currentZmm < 50.0f){
#if FEATURE_MILLING_MODE
                if( Printer::operatingMode == OPERATING_MODE_PRINT )
                {
#endif // FEATURE_MILLING_MODE
                    //Nur so weit runterfahren, wie man über 0 ist. Denn da ist beim Printermode homed der Endschalter.
                    if(currentZmm > 0) Printer::setDestinationStepsFromMenu( 0, 0, currentZmm * increment );
                    else Printer::setDestinationStepsFromMenu( 0, 0, g_nManualSteps[Z_AXIS]*Printer::invAxisStepsPerMM[Z_AXIS] * increment );
#if FEATURE_MILLING_MODE
                }
                else
                {
                    //Beim Milling ist Z=0 das obere des Bauteils. Dann geht Z - ins Bauteil rein. Daher ist überfahren ok.
                    Printer::setDestinationStepsFromMenu( 0, 0, 50 * increment );    
                }
#endif // FEATURE_MILLING_MODE
            }else{
                Printer::setDestinationStepsFromMenu( 0, 0, 50 * increment );                
            }
            break;
        }
    }

} // nextPreviousZAction


#if STEPPER_CURRENT_CONTROL==CURRENT_CONTROL_DRV8711

#define DRV8711_NUM_CHANNELS                5

void drv8711Transmit( unsigned short command )
{
    char    i;


    // transfer the command (= direction, address and data)
    InterruptProtectedBlock noInts; //HAL::forbidInterrupts();
    for( i=15; i>=0; i-- )
    {
        WRITE( DRV_SDATI, command & (0x01 << i));
        HAL::delayMicroseconds( 1 );
        WRITE( DRV_SCLK, 1 );
        HAL::delayMicroseconds( 5 );
        WRITE( DRV_SCLK, 0 );
    }
    noInts.unprotect(); //HAL::allowInterrupts();

} // drv8711Transmit


unsigned short drv8711Receive( unsigned char address )
{
    unsigned short  acknowledge = 0;
    unsigned short  temp;
    char                i;


    if( address > 7 )   return 0;

    acknowledge =  address;
    acknowledge =  acknowledge << 12;
    acknowledge |= 0x8000;

    // transfer the read request plus the register address (4 bits)
    InterruptProtectedBlock noInts; //HAL::forbidInterrupts();
    for( i=15; i>=12; i-- )
    {
        WRITE( DRV_SDATI, acknowledge & (0x01 << i));
        HAL::delayMicroseconds( 1 );
        WRITE( DRV_SCLK, 1 );
        HAL::delayMicroseconds( 5 );
        WRITE( DRV_SCLK, 0 );
    }

    HAL::delayMicroseconds( 20 );
  
    // read the acknowledge (12 bits)
    for( i=11; i>=0; i-- )
    {
        temp = READ( DRV_SDATO );
        acknowledge = acknowledge | (temp << i);
        WRITE( DRV_SCLK, 1 );
        HAL::delayMicroseconds( 25 );
        WRITE( DRV_SCLK, 0 );
        HAL::delayMicroseconds( 25 );
    }
    noInts.unprotect(); //HAL::allowInterrupts();

    return acknowledge;

} // drv8711Receive


void drv8711EnableAll( void )
{
    // enable the chip select of all present DRV8711
    switch( DRV8711_NUM_CHANNELS )
    {
        case 5:  {  WRITE( O1_SCS_PIN, LOW ); SET_OUTPUT( O1_SCS_PIN ); WRITE( O1_SCS_PIN, HIGH ); }  // fall through
        case 4:  {  WRITE( O0_SCS_PIN, LOW ); SET_OUTPUT( O0_SCS_PIN ); WRITE( O0_SCS_PIN, HIGH ); }  // fall through
        case 3:  {  WRITE( Z_SCS_PIN, LOW );  SET_OUTPUT( Z_SCS_PIN );  WRITE( Z_SCS_PIN, HIGH );  }  // fall through
        case 2:  {  WRITE( Y_SCS_PIN, LOW );  SET_OUTPUT( Y_SCS_PIN );  WRITE( Y_SCS_PIN, HIGH );  }  // fall through
        case 1:  {  WRITE( X_SCS_PIN, LOW );  SET_OUTPUT( X_SCS_PIN );  WRITE( X_SCS_PIN, HIGH );  }
    }

} // drv8711EnableAll


void drv8711DisableAll( void )
{
    // disable the chip select of all present DRV8711
    switch( DRV8711_NUM_CHANNELS )
    {
        case 5:  {  WRITE( O1_SCS_PIN, LOW ); }  // fall through
        case 4:  {  WRITE( O0_SCS_PIN, LOW ); }  // fall through
        case 3:  {  WRITE( Z_SCS_PIN, LOW );  }  // fall through
        case 2:  {  WRITE( Y_SCS_PIN, LOW );  }  // fall through
        case 1:  {  WRITE( X_SCS_PIN, LOW );  }
    }

} // drv8711DisableAll


void drv8711Enable( unsigned char driver )
{
    // enable the chip select of the DRV8711
    switch( driver )
    {
        case 5:  {  WRITE( O1_SCS_PIN, LOW ); SET_OUTPUT( O1_SCS_PIN ); WRITE( O1_SCS_PIN, HIGH ); break;  }
        case 4:  {  WRITE( O0_SCS_PIN, LOW ); SET_OUTPUT( O0_SCS_PIN ); WRITE( O0_SCS_PIN, HIGH ); break;  }
        case 3:  {  WRITE( Z_SCS_PIN, LOW );  SET_OUTPUT( Z_SCS_PIN );  WRITE( Z_SCS_PIN, HIGH );  break;  }
        case 2:  {  WRITE( Y_SCS_PIN, LOW );  SET_OUTPUT( Y_SCS_PIN );  WRITE( Y_SCS_PIN, HIGH );  break;  }
        case 1:  {  WRITE( X_SCS_PIN, LOW );  SET_OUTPUT( X_SCS_PIN );  WRITE( X_SCS_PIN, HIGH );  break;  }
    }

} // drv8711Enable


void drv8711Disable( unsigned char driver )
{
    // disable the chip select of the DRV8711
    switch( driver )
    {
        case 5:  {  WRITE( O1_SCS_PIN, LOW ); break;  }
        case 4:  {  WRITE( O0_SCS_PIN, LOW ); break;  }
        case 3:  {  WRITE( Z_SCS_PIN, LOW );  break;  }
        case 2:  {  WRITE( Y_SCS_PIN, LOW );  break;  }
        case 1:  {  WRITE( X_SCS_PIN, LOW );  break;  }
    }

} // drv8711Disable


void drv8711Init( void )
{
    // configure the pins
    WRITE( DRV_RESET1, LOW );
    SET_OUTPUT( DRV_RESET1 );

#if DRV_RESET2
    WRITE( DRV_RESET2, LOW );
    SET_OUTPUT( DRV_RESET2 );
#endif // DRV_RESET2

    WRITE( DRV_SCLK, LOW );
    SET_OUTPUT( DRV_SCLK );
    WRITE( DRV_SDATI, LOW );
    SET_OUTPUT( DRV_SDATI );

    // configure the following inputs as pullup
    WRITE( DRV_SDATO, HIGH );
    WRITE( DRV_FAULT, HIGH );
    WRITE( X_STALL_PIN, HIGH );
    WRITE( Y_STALL_PIN, HIGH );
    WRITE( Z_STALL_PIN, HIGH );
    WRITE( O0_STALL_PIN, HIGH );
    WRITE( O1_STALL_PIN, HIGH );

    // reset all DRV8711 (active high)
    WRITE( DRV_RESET1, HIGH );

#if DRV_RESET2
    WRITE( DRV_RESET2, HIGH );
#endif // DRV_RESET2

    HAL::delayMicroseconds( 5000 );
    WRITE( DRV_RESET1, LOW );

#if DRV_RESET2
    WRITE( DRV_RESET2, LOW );
#endif // DRV_RESET2

    HAL::delayMicroseconds( 5000 );
  
    // configure all registers except the motor current (= register 01)
    drv8711EnableAll();

#if RF_MICRO_STEPS == 4
    #define DRV8711_REGISTER_00             0x0E11                                              // 0000 1110 0001 0001: ENBL = 1, RDIR = 0, RSTEP = 0, MODE = 0010, EXSTALL = 0, ISGAIN = 10, DTIME = 11
#elif RF_MICRO_STEPS == 8
    #define DRV8711_REGISTER_00             0x0E19                                              // 0000 1110 0001 1001: ENBL = 1, RDIR = 0, RSTEP = 0, MODE = 0011, EXSTALL = 0, ISGAIN = 10, DTIME = 11
#elif RF_MICRO_STEPS == 16
    #define DRV8711_REGISTER_00             0x0E21                                              // 0000 1110 0010 0001: ENBL = 1, RDIR = 0, RSTEP = 0, MODE = 0100, EXSTALL = 0, ISGAIN = 10, DTIME = 11
#elif RF_MICRO_STEPS == 32
    #define DRV8711_REGISTER_00             0x0E29                                              // 0000 1110 0010 1001: ENBL = 1, RDIR = 0, RSTEP = 0, MODE = 0101, EXSTALL = 0, ISGAIN = 10, DTIME = 11
#elif RF_MICRO_STEPS == 64
    #define DRV8711_REGISTER_00             0x0E31                                              // 0000 1110 0011 0001: ENBL = 1, RDIR = 0, RSTEP = 0, MODE = 0110, EXSTALL = 0, ISGAIN = 10, DTIME = 11
#else
    #error this number of micro steps is not supported
#endif // RF_MICRO_STEPS

    drv8711Transmit( DRV8711_REGISTER_00 );
    drv8711DisableAll();
    HAL::delayMicroseconds( 1 );

                                                                                                // ADRESS 11..8 7..4 3..0
#define DRV8711_REGISTER_02                 0x2097                                              // 0010   0000  1001 0111: TOFF = 10010111, PWMMODE = 0
#define DRV8711_REGISTER_03                 0x31D7                                              // 0011   0001  1101 0111: TBLANK = 11010111, ABT = 1
#define DRV8711_REGISTER_04                 0x4430                                              // 0100   0100  0011 0000: TDECAY = 00110000, DECMOD = 100
#define DRV8711_REGISTER_05                 0x583C                                              // 0101   1000  0011 1100: SDTHR = 00111100, SDCNT = 00, VDIV = 10
#define DRV8711_REGISTER_06                 0x60F0                                              // 0110   0000  1111 0000: OCPTH = 00, OCPDEG = 00, TDRIVEN = 11, TDRIVEP = 11, IDRIVEN = 00, IDRIVEP = 00
#define DRV8711_REGISTER_07                 0x7000                                              // 0111   0000  0000 0000: OTS = 0, AOCP = 0, BOCP = 0, UVLO = 0, APDF = 0, BPDF = 0, STD = 0, STDLAT = 0

    drv8711EnableAll();
    drv8711Transmit( DRV8711_REGISTER_02 );
    drv8711DisableAll();
    HAL::delayMicroseconds( 1 );

    drv8711EnableAll();
    drv8711Transmit( DRV8711_REGISTER_03 );
    drv8711DisableAll();
    HAL::delayMicroseconds( 1 );

    drv8711EnableAll();
    drv8711Transmit( DRV8711_REGISTER_04 );
    drv8711DisableAll();
    HAL::delayMicroseconds( 1 );

    drv8711EnableAll();
    drv8711Transmit( DRV8711_REGISTER_05 );
    drv8711DisableAll();
    HAL::delayMicroseconds( 1 );

    drv8711EnableAll();
    drv8711Transmit( DRV8711_REGISTER_06 );
    drv8711DisableAll();
    HAL::delayMicroseconds( 1 );

    drv8711EnableAll();
    drv8711Transmit( DRV8711_REGISTER_07 );
    drv8711DisableAll();

} // drv8711Init


void setMotorCurrent( unsigned char driver, uint8_t level )
{
    // NOTE: Do not increase the current endlessly. In case the engine reaches its current saturation, the engine and the driver can heat up and loss power.
    // When the saturation is reached, more current causes more heating and more power loss.
    // In case of engines with lower quality, the saturation current may be reached before the nominal current.

    //X = 1
    //Y = 2
    //Z = 3
    //E0 = 4
    //E1 = 5

    // configure the pins
    WRITE( DRV_SCLK, LOW );
    SET_OUTPUT( DRV_SCLK );
    WRITE( DRV_SDATI, LOW );
    SET_OUTPUT( DRV_SDATI );

    drv8711Enable( driver );

    // we have to write to register 01
    unsigned short command = 0x1100 + (short)level;
    drv8711Transmit( command );

    drv8711Disable( driver );

} // setMotorCurrent


void motorCurrentControlInit( void )
{
    const unsigned short  uMotorCurrentUse[] = MOTOR_CURRENT_NORMAL; //--> {x,y,z,e1,e2} siehe RFx000.h
    Printer::motorCurrent[X_AXIS]   = uMotorCurrentUse[X_AXIS];
    Printer::motorCurrent[Y_AXIS]   = uMotorCurrentUse[Y_AXIS];
    Printer::motorCurrent[Z_AXIS]   = uMotorCurrentUse[Z_AXIS];
    Printer::motorCurrent[E_AXIS+0] = uMotorCurrentUse[E_AXIS+0];
    Printer::motorCurrent[E_AXIS+1] = uMotorCurrentUse[E_AXIS+1]; //egal ob NUM_EXTRUDER == 1 oder 2
    // configure all DRV8711
    drv8711Init();
    // set all motor currents
    for( uint8_t i=0 ; i<DRV8711_NUM_CHANNELS ; i++ )
    {
        if(i < 3+NUM_EXTRUDER) setMotorCurrent( i+1, Printer::motorCurrent[i] );
        else setMotorCurrent( i+1, MOTOR_CURRENT_MIN );
    }
} // motorCurrentControlInit

#if FEATURE_READ_STEPPER_STATUS
unsigned short readMotorStatus( unsigned char driver )
{
    //driver:
    //X = 1
    //Y = 2
    //Z = 3
    //E0 = 4
    //E1 = 5

/*
0	OTS	1	R/W	0	0: Normal operation 
1: Device has entered overtemperature shutdown 
Write a 0 to this bit to clear the fault. 
Operation automatically resumes when the temperature has fallen to safe levels.
1	AOCP	1	R/W	0	0: Normal operation 
1: Channel A overcurrent shutdown 
Write a 0 to this bit to clear the fault and resume operation
2	BOCP	1	R/W	0	0: Normal operation 
1: Channel B overcurrent shutdown 
Write a 0 to this bit to clear the fault and resume operation
3	APDF	1	R/W	0	0: Normal operation 
1: Channel A predriver fault 
Write a 0 to this bit to clear the fault and resume operation.
4	BPDF	1	R/W	0	0: Normal operation 
1: Channel B predriver fault 
Write a 0 to this bit to clear the fault and resume operation
5	UVLO	1	R/W	0	0: Normal operation 
1: Undervoltage lockout 
Write a 0 to this bit to clear the fault. The UVLO bit cannot be cleared in sleep mode. Operation automatically resumes when VM has increased above VUVLO
6	STD	1	R	0	0: Normal operation 
1: Stall detected
7	STDLAT	1	R/W	0	0: Normal operation 
1: Latched stall detect 
Write a 0 to this bit to clear the fault and resume operation
11-8	Reserved	4	-	-	Reserved
*/

// configure the pins
 WRITE( DRV_SCLK, LOW );
 SET_OUTPUT( DRV_SCLK );
 WRITE( DRV_SDATI, LOW );
 SET_OUTPUT( DRV_SDATI );

 drv8711Enable( driver );
 unsigned short status = drv8711Receive( 7 ); //7.6.9 STATUS Register (Address = 0x07)
 drv8711Disable( driver );

 Com::printF( PSTR( "Driver: " ), driver );
 switch( driver )
 {
        case 1:{ Com::printFLN( PSTR( "X" ) ); break; }
        case 2:{ Com::printFLN( PSTR( "Y" ) ); break; }
        case 3:{ Com::printFLN( PSTR( "Z" ) ); break; }
        case 4:{ Com::printFLN( PSTR( "E0" ) ); break; }
        case 5:{ Com::printFLN( PSTR( "E1" ) ); break; }
 }
 
 uint8_t bit = 0;
 for( uint8_t n = 0; n <= 7; n++ ){
    bit = status & (1 << n) ? 1:0;
    switch (n){
        case 0:{ Com::printFLN( PSTR( "OTS OverTemp " ), bit ); break; }
        case 1:{ Com::printFLN( PSTR( "AOCP Channel A Overcurrent " ), bit ); break; }
        case 2:{ Com::printFLN( PSTR( "BOCP Channel B Overcurrent " ), bit ); break; }
        case 3:{ Com::printFLN( PSTR( "APDF Channel A predriver fault " ), bit ); break; }
        case 4:{ Com::printFLN( PSTR( "BPDF Channel B predriver fault " ), bit ); break; }
        case 5:{ Com::printFLN( PSTR( "UVLO Undervoltage lockout " ), bit ); break; }
        case 6:{ Com::printFLN( PSTR( "STD Stall detected " ), bit ); break; }
        case 7:{ Com::printFLN( PSTR( "STDLAT Latched stall detect " ), bit ); break; }
    }
 }

 switch( driver )
 {
        case 5: { bit = READ( O1_STALL_PIN ); break; }
        case 4: { bit = READ( O0_STALL_PIN ); break; }
        case 3: { bit = READ( Z_STALL_PIN );  break; }
        case 2: { bit = READ( Y_STALL_PIN );  break; }
        case 1: { bit = READ( X_STALL_PIN );  break; }
 }

 Com::printFLN( PSTR( "Stall Pin: " ), !bit );
 return ( status & (bit << 8) );
} // readMotorStatus
#endif // FEATURE_READ_STEPPER_STATUS

#endif // CURRENT_CONTROL_DRV8711


void cleanupXPositions( void )
{
    InterruptProtectedBlock noInts; //HAL::forbidInterrupts();

    Printer::queuePositionCurrentSteps[X_AXIS] =
    Printer::queuePositionLastSteps[X_AXIS]    =
    Printer::queuePositionTargetSteps[X_AXIS]  =
    Printer::queuePositionLastMM[X_AXIS]       =
    Printer::queuePositionCommandMM[X_AXIS]    = 0;

#if FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING
    Printer::directPositionTargetSteps[X_AXIS]  = 
    Printer::directPositionCurrentSteps[X_AXIS] = 0;
#endif // FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING
    
#if FEATURE_PAUSE_PRINTING
    g_nContinueSteps[X_AXIS] = 0;
    g_pauseStatus            = PAUSE_STATUS_NONE;
    g_pauseMode              = PAUSE_MODE_NONE;
    g_uPauseTime             = 0;
    g_pauseBeepDone          = 0;
#endif // FEATURE_PAUSE_PRINTING

    noInts.unprotect(); //HAL::allowInterrupts();

} // cleanupXPositions


void cleanupYPositions( void )
{
    InterruptProtectedBlock noInts; //HAL::forbidInterrupts();

    Printer::queuePositionCurrentSteps[Y_AXIS] =
    Printer::queuePositionLastSteps[Y_AXIS]    =
    Printer::queuePositionTargetSteps[Y_AXIS]  =
    Printer::queuePositionLastMM[Y_AXIS]       =
    Printer::queuePositionCommandMM[Y_AXIS]    = 0;

#if FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING
    Printer::directPositionTargetSteps[Y_AXIS]  = 
    Printer::directPositionCurrentSteps[Y_AXIS] = 0;
#endif // FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING
    
#if FEATURE_PAUSE_PRINTING
    g_nContinueSteps[Y_AXIS] = 0;
    g_pauseStatus            = PAUSE_STATUS_NONE;
    g_pauseMode              = PAUSE_MODE_NONE;
    g_uPauseTime             = 0;
    g_pauseBeepDone          = 0;
#endif // FEATURE_PAUSE_PRINTING

    noInts.unprotect(); //HAL::allowInterrupts();

} // cleanupYPositions


void cleanupZPositions( void )
{
    //Com::printFLN( PSTR( "cleanupZPositions()" ) );

    InterruptProtectedBlock noInts; //HAL::forbidInterrupts();

#if FEATURE_HEAT_BED_Z_COMPENSATION
    Printer::doHeatBedZCompensation = 0;
#endif // FEATURE_HEAT_BED_Z_COMPENSATION

#if FEATURE_WORK_PART_Z_COMPENSATION
    Printer::doWorkPartZCompensation = 0;
    Printer::staticCompensationZ     = 0;
#endif // FEATURE_WORK_PART_Z_COMPENSATION

    Printer::queuePositionCurrentSteps[Z_AXIS] =
    Printer::queuePositionLastSteps[Z_AXIS]    =
    Printer::queuePositionTargetSteps[Z_AXIS]  =
    Printer::queuePositionLastMM[Z_AXIS]       =
    Printer::queuePositionCommandMM[Z_AXIS]    = 

#if FEATURE_Z_MIN_OVERRIDE_VIA_GCODE
    Printer::currentZSteps                     = 0;
#endif // FEATURE_Z_MIN_OVERRIDE_VIA_GCODE

#if FEATURE_HEAT_BED_Z_COMPENSATION || FEATURE_WORK_PART_Z_COMPENSATION
    Printer::compensatedPositionTargetStepsZ  =
    Printer::compensatedPositionCurrentStepsZ =
    Printer::endZCompensationStep             = 
    g_nZScanZPosition                         = 0;
#endif // FEATURE_HEAT_BED_Z_COMPENSATION || FEATURE_WORK_PART_Z_COMPENSATION

#if FEATURE_FIND_Z_ORIGIN
    g_nZOriginPosition[X_AXIS] = 0;
    g_nZOriginPosition[Y_AXIS] = 0;
    g_nZOriginPosition[Z_AXIS] = 0;
    g_nZOriginSet              = 0;
#endif // FEATURE_FIND_Z_ORIGIN

#if FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING
    Printer::directPositionTargetSteps[Z_AXIS]  =
    Printer::directPositionCurrentSteps[Z_AXIS] = 0;
#endif // FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING
    
#if FEATURE_PAUSE_PRINTING
    g_nContinueSteps[Z_AXIS] = 0;
    g_pauseStatus            = PAUSE_STATUS_NONE;
    g_pauseMode              = PAUSE_MODE_NONE;
    g_uPauseTime             = 0;
    g_pauseBeepDone          = 0;
#endif // FEATURE_PAUSE_PRINTING

    noInts.unprotect(); //HAL::allowInterrupts();

} // cleanupZPositions


void cleanupEPositions( void )
{
    InterruptProtectedBlock noInts; //HAL::forbidInterrupts();

#if FEATURE_EXTENDED_BUTTONS
    Printer::directPositionTargetSteps[E_AXIS]  =
    Printer::directPositionCurrentSteps[E_AXIS] = 0;
#endif // FEATURE_EXTENDED_BUTTONS
    
#if FEATURE_PAUSE_PRINTING
    g_nContinueSteps[E_AXIS] = 0;
    g_pauseStatus            = PAUSE_STATUS_NONE;
    g_pauseMode              = PAUSE_MODE_NONE;
    g_uPauseTime             = 0;
    g_pauseBeepDone          = 0;
#endif // FEATURE_PAUSE_PRINTING

    noInts.unprotect(); //HAL::allowInterrupts();

} // cleanupEPositions


void setZOrigin( void )
{
#if FEATURE_FIND_Z_ORIGIN
    g_nZOriginPosition[X_AXIS] = Printer::queuePositionLastSteps[X_AXIS];
    g_nZOriginPosition[Y_AXIS] = Printer::queuePositionLastSteps[Y_AXIS];

#if FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING
    g_nZOriginPosition[X_AXIS] += Printer::directPositionLastSteps[X_AXIS];
    g_nZOriginPosition[Y_AXIS] += Printer::directPositionLastSteps[Y_AXIS];
#endif // FEATURE_EXTENDED_BUTTONS || FEATURE_PAUSE_PRINTING

    g_nZOriginPosition[Z_AXIS] = 0;
    g_nZOriginSet              = 1;
#endif // FEATURE_FIND_Z_ORIGIN

    Printer::updateCurrentPosition();

    // it does not make sense to change the length here because Printer::queuePositionLastMM[Z_AXIS] can be a more or less random value
    //Printer::lengthMM[Z_AXIS] -= Printer::queuePositionLastMM[Z_AXIS];

    Printer::queuePositionTargetSteps[Z_AXIS]   = 
    Printer::queuePositionCurrentSteps[Z_AXIS]  = 
    Printer::queuePositionLastSteps[Z_AXIS]     = 0;
    Printer::directPositionTargetSteps[Z_AXIS]  = 
    Printer::directPositionCurrentSteps[Z_AXIS] = 
    Printer::directPositionLastSteps[Z_AXIS]    = 0;
    Printer::originOffsetMM[Z_AXIS]             = 0;

#if FEATURE_Z_MIN_OVERRIDE_VIA_GCODE
    Printer::currentZSteps                      = 0;
#endif // FEATURE_Z_MIN_OVERRIDE_VIA_GCODE

    Printer::updateDerivedParameter();
    Printer::updateCurrentPosition(true);
    
#if EEPROM_MODE!=0
    EEPROM::storeDataIntoEEPROM(false);

    if( Printer::debugInfo() )
    {
        Com::printFLN(Com::tEEPROMUpdated);
    }
#endif // EEPROM_MODE!=0
    
    Commands::printCurrentPosition();

    Printer::setZOriginSet(true);

    BEEP_ACCEPT_SET_POSITION

} // setZOrigin


#if FEATURE_FIND_Z_ORIGIN
void startFindZOrigin( void )
{
    if( g_nFindZOriginStatus )
    {
        if( !g_abortSearch )
        {
            // abort the finding of the z-origin
            if( Printer::debugInfo() )
            {
                Com::printFLN( PSTR( "startFindZOrigin(): the search has been cancelled" ) );
            }
            g_abortSearch = 1;
        }
    }
    else
    {
        if( Printer::operatingMode != OPERATING_MODE_MILL )
        {
            if( Printer::debugErrors() )
            {
                Com::printFLN( PSTR( "startFindZOrigin(): finding of the z-origin is not supported in this mode" ) );
            }

            showError( (void*)ui_text_find_z_origin, (void*)ui_text_operation_denied );
            return;
        }

/*      if( PrintLine::linesCount )
        {
            // there is some printing in progress at the moment - do not start the search in this case
            if( Printer::debugErrors() )
            {
                Com::printFLN( PSTR( "startFindZOrigin(): the search can not be started while the milling is in progress" ) );
                return;
            }
        }
*/
        // start the search
        g_nFindZOriginStatus = 1;

#if FEATURE_HEAT_BED_Z_COMPENSATION
        // when the search is running, the z-compensation must be disabled
        if( Printer::doHeatBedZCompensation )
        {
            if( Printer::debugInfo() )
            {
                Com::printFLN( PSTR( "startFindZOrigin(): the z compensation has been disabled" ) );
            }
            resetZCompensation();
        }
#endif // FEATURE_HEAT_BED_Z_COMPENSATION

#if FEATURE_WORK_PART_Z_COMPENSATION
        // when the search is running, the z-compensation must be disabled
        if( Printer::doWorkPartZCompensation )
        {
            if( Printer::debugInfo() )
            {
                Com::printFLN( PSTR( "startFindZOrigin(): the z compensation has been disabled" ) );
            }
            resetZCompensation();
        }
#endif // FEATURE_WORK_PART_Z_COMPENSATION
    }

    return;

} // startFindZOrigin


void findZOrigin( void )
{
    static short    nMaxPressureContact;
    static short    nMinPressureContact;
    short           nCurrentPressure;
    unsigned long   uStartTime;
    unsigned long   uCurrentTime;


    if( g_abortSearch )
    {
        // the search has been aborted
        g_abortSearch       = 0;
        g_nZOriginPosition[Z_AXIS] = 0;
        g_nZOriginSet       = 0;

        // turn off the engines
        Printer::disableZStepper();

        if( Printer::debugInfo() )
        {
            Com::printFLN( PSTR( "findZOrigin(): the search has been aborted" ) );
        }

        UI_STATUS_UPD( UI_TEXT_FIND_Z_ORIGIN_ABORTED );
        g_nFindZOriginStatus = 0;
        return;
    }

    // show that we are active
    previousMillisCmd = HAL::timeInMilliseconds();

    if( g_nFindZOriginStatus )
    {
        UI_STATUS_UPD( UI_TEXT_FIND_Z_ORIGIN );

        //HAL::delayMilliseconds( 2000 );

        switch( g_nFindZOriginStatus )
        {
            case 1:
            {
                g_abortSearch       = 0;
                g_nZOriginPosition[Z_AXIS] = 0;

                if( Printer::debugInfo() )
                {
                    Com::printFLN( PSTR( "findZOrigin(): the search has been started" ) );
                }

                if( readAveragePressure( &nCurrentPressure ) )
                {
                    // some error has occurred
                    if( Printer::debugErrors() )
                    {
                        Com::printFLN( PSTR( "findZOrigin(): the start pressure could not be determined" ) );
                    }
                    g_abortSearch = 1;
                    return;
                }

                nMinPressureContact = nCurrentPressure - SEARCH_Z_ORIGIN_CONTACT_PRESSURE_DELTA;
                nMaxPressureContact = nCurrentPressure + SEARCH_Z_ORIGIN_CONTACT_PRESSURE_DELTA;

                if( Printer::debugInfo() )
                {
                    Com::printF( PSTR( "findZOrigin(): nMinPressureContact = " ), nMinPressureContact );
                    Com::printFLN( PSTR( ", nMaxPressureContact = " ), nMaxPressureContact );
                }

                previousMillisCmd = HAL::timeInMilliseconds();
                Printer::enableZStepper();
                Printer::unsetAllSteppersDisabled();

                // prepare the direction of the z-axis (we have to move the milling bed up)
                prepareBedUp();

                g_nTempDirectionZ    = -1;
                g_nFindZOriginStatus = 10;

#if DEBUG_FIND_Z_ORIGIN
                Com::printFLN( PSTR( "findZOrigin(): 1 -> 10" ) );
#endif // DEBUG_FIND_Z_ORIGIN
                break;
            }
            case 10:
            {
                // move the heat bed up until we detect the contact pressure
                uStartTime = HAL::timeInMilliseconds();
                while( 1 )
                {
                    nCurrentPressure = readStrainGauge( ACTIVE_STRAIN_GAUGE );

                    if( nCurrentPressure > nMaxPressureContact || nCurrentPressure < nMinPressureContact )
                    {
                        // we have reached the target pressure
                        g_nFindZOriginStatus = 20;

#if DEBUG_FIND_Z_ORIGIN
                        Com::printFLN( PSTR( "findZOrigin(): 10 -> 20" ) );
#endif // DEBUG_FIND_Z_ORIGIN
                        return;
                    }

                    if( Printer::isZMinEndstopHit() )
                    {
                        // this should never happen
                        if( Printer::debugErrors() )
                        {
                            Com::printFLN( PSTR( "findZOrigin(): the z-min endstop has been reached" ) );
                        }
                        g_abortSearch = 1;
                        return;
                    }

                    g_nZOriginPosition[Z_AXIS] += moveZ( SEARCH_Z_ORIGIN_BED_UP_STEPS );

                    uCurrentTime = HAL::timeInMilliseconds();
                    if( (uCurrentTime - uStartTime) > SEARCH_Z_ORIGIN_BREAKOUT_DELAY )
                    {
                        // do not stay within this loop forever
                        return;
                    }

                    if( g_abortSearch )
                    {
                        break;
                    }
                }

                // we should never end up here
                break;
            }
            case 20:
            {
                // move the heat bed down again until we do not detect any contact anymore
                uStartTime = HAL::timeInMilliseconds();
                while( 1 )
                {
                    nCurrentPressure = readStrainGauge( ACTIVE_STRAIN_GAUGE );

                    if( nCurrentPressure > nMinPressureContact && nCurrentPressure < nMaxPressureContact )
                    {
                        // we have reached the target pressure
                        g_nFindZOriginStatus = 30;

#if DEBUG_FIND_Z_ORIGIN
                        Com::printFLN( PSTR( "findZOrigin(): 20 -> 30" ) );
#endif // DEBUG_FIND_Z_ORIGIN
                        return;
                    }

                    if( Printer::isZMaxEndstopHit() )
                    {
                        if( Printer::debugErrors() )
                        {
                            Com::printFLN( PSTR( "findZOrigin(): the z-max endstop has been reached" ) );
                        }
                        g_abortSearch = 1;
                        return;
                    }

                    g_nZOriginPosition[Z_AXIS] += moveZ( SEARCH_Z_ORIGIN_BED_DOWN_STEPS );

                    uCurrentTime = HAL::timeInMilliseconds();
                    if( (uCurrentTime - uStartTime) > SEARCH_Z_ORIGIN_BREAKOUT_DELAY )
                    {
                        // do not stay within this loop forever
                        return;
                    }

                    if( g_abortSearch )
                    {
                        break;
                    }
                }

                // we should never end up here
                break;
            }
            case 30:
            {
                // we have found the z-origin
                setZOrigin();

                GCode::executeFString( Com::tFindZOrigin );
                g_nFindZOriginStatus = 40;

#if DEBUG_FIND_Z_ORIGIN
                Com::printFLN( PSTR( "findZOrigin(): 30 -> 40" ) );
#endif // DEBUG_FIND_Z_ORIGIN
                break;
            }
            case 40:
            {
                if( PrintLine::linesCount )
                {
                    // wait until all moves have been done
                    break;
                }

                Commands::printCurrentPosition();
                g_nFindZOriginStatus = 0;
                UI_STATUS_UPD( UI_TEXT_FIND_Z_ORIGIN_DONE );

#if DEBUG_FIND_Z_ORIGIN
                Com::printFLN( PSTR( "findZOrigin(): 40 -> 0" ) );
#endif // DEBUG_FIND_Z_ORIGIN
                break;
            }
        }
    }

    // we should never end up here
    return;

} // findZOrigin
#endif // FEATURE_FIND_Z_ORIGIN

#if FEATURE_MILLING_MODE
void switchOperatingMode( char newOperatingMode )
{
    if( newOperatingMode != OPERATING_MODE_PRINT && newOperatingMode != OPERATING_MODE_MILL )
    {
        // do not allow not-supported operating modes
        return;
    }

    Printer::operatingMode = newOperatingMode;
    if( Printer::operatingMode == OPERATING_MODE_PRINT )
    {
        setupForPrinting();
    }
    else
    {
        setupForMilling();
    }

    return;

} // switchOperatingMode


void switchActiveWorkPart( char newActiveWorkPart )
{
    if( newActiveWorkPart < 1 || newActiveWorkPart > EEPROM_MAX_WORK_PART_SECTORS )
    {
        // do not allow not-supported z-compensation matrix
        return;
    }

    g_nActiveWorkPart = newActiveWorkPart;
    writeWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, EEPROM_OFFSET_ACTIVE_WORK_PART_Z_MATRIX, g_nActiveWorkPart );

    if( loadCompensationMatrix( (EEPROM_SECTOR_SIZE *9) + (unsigned int)(EEPROM_SECTOR_SIZE * g_nActiveWorkPart) ) )
    {
        // there is no valid z-compensation matrix available
        initCompensationMatrix();
    }
    return;

} // switchActiveWorkPart


void setScanXYStart( void )
{
/*  if( Printer::queuePositionLastSteps[X_AXIS] > g_nScanXMaxPositionSteps ||
        Printer::queuePositionLastSteps[Y_AXIS] > g_nScanYMaxPositionSteps )
    {
        // we can not take over the new x/y start position in case it is bigger than the current x/y end position
        if( Printer::debugErrors() )
        {
            Com::printFLN( PSTR( "setScanXYStart(): the new x/y start position can not be set because it is bigger than the current x/y end position" ) );
            Com::printF( PSTR( "current: x = " ), (float)Printer::queuePositionLastSteps[X_AXIS] / Printer::axisStepsPerMM[X_AXIS] );
            Com::printF( PSTR( ", y = " ), (float)Printer::queuePositionLastSteps[Y_AXIS] / Printer::axisStepsPerMM[Y_AXIS] );
            Com::printFLN( PSTR( " [mm]" ) );
            Com::printF( PSTR( "end: x = " ), (float)g_nScanXMaxPositionSteps / Printer::axisStepsPerMM[X_AXIS] );
            Com::printF( PSTR( ", y = " ), (float)g_nScanYMaxPositionSteps / Printer::axisStepsPerMM[Y_AXIS] );
            Com::printFLN( PSTR( " [mm]" ) );
        }
        BEEP_ABORT_SET_POSITION
        return;
    }
*/
    // round to Integer [mm]
    g_nScanXStartSteps = ((float)Printer::queuePositionLastSteps[X_AXIS] + (float)Printer::directPositionLastSteps[X_AXIS]) / Printer::axisStepsPerMM[X_AXIS];
    g_nScanXStartSteps *= Printer::axisStepsPerMM[X_AXIS];
    g_nScanYStartSteps = ((float)Printer::queuePositionLastSteps[Y_AXIS] + (float)Printer::directPositionLastSteps[Y_AXIS]) / Printer::axisStepsPerMM[Y_AXIS];
    g_nScanYStartSteps *= Printer::axisStepsPerMM[Y_AXIS];

    if( g_nScanXStartSteps > g_nScanXMaxPositionSteps ||
        g_nScanYStartSteps > g_nScanYMaxPositionSteps )
    {
        // the new start position would be bigger than the current end position - we set the end position to the start position in this case
        if( Printer::debugInfo() )
        {
            Com::printFLN( PSTR( "setScanXYStart(): the new x/y start position is bigger than the current x/y end position, the x/y end position will be set to the new x/y start position" ) );
            g_nScanXMaxPositionSteps = g_nScanXStartSteps;
            g_nScanYMaxPositionSteps = g_nScanYStartSteps;
        }
    }
    
    if( Printer::debugInfo() )
    {
        Com::printFLN( PSTR( "setScanXYStart(): the new x/y start position has been set" ) );
        Com::printF( PSTR( "start: x = " ), (float)g_nScanXStartSteps / Printer::axisStepsPerMM[X_AXIS] );
        Com::printF( PSTR( ", y = " ), (float)g_nScanYStartSteps / Printer::axisStepsPerMM[Y_AXIS] );
        Com::printFLN( PSTR( " [mm]" ) );
        Com::printF( PSTR( "end: x = " ), (float)g_nScanXMaxPositionSteps / Printer::axisStepsPerMM[X_AXIS] );
        Com::printF( PSTR( ", y = " ), (float)g_nScanYMaxPositionSteps / Printer::axisStepsPerMM[Y_AXIS] );
        Com::printFLN( PSTR( " [mm]" ) );
    }
    BEEP_ACCEPT_SET_POSITION
    return;

} // setScanXYStart


void setScanXYEnd( void )
{
/*  if( Printer::queuePositionLastSteps[X_AXIS] < g_nScanXStartSteps ||
        Printer::queuePositionLastSteps[Y_AXIS] < g_nScanYStartSteps )
    {
        // we can not take over the new x/y end position in case it is smaller than the current x/y start position
        if( Printer::debugErrors() )
        {
            Com::printFLN( PSTR( "setScanXYEnd(): the new x/y end position can not be set because it is smaller than the current x/y start position" ) );
            Com::printF( PSTR( "start: x = " ), (float)g_nScanXStartSteps / Printer::axisStepsPerMM[X_AXIS] );
            Com::printF( PSTR( ", y = " ), (float)g_nScanYStartSteps / Printer::axisStepsPerMM[Y_AXIS] );
            Com::printFLN( PSTR( " [mm]" ) );
            Com::printF( PSTR( "current: x = " ), (float)Printer::queuePositionLastSteps[X_AXIS] / Printer::axisStepsPerMM[X_AXIS] );
            Com::printF( PSTR( ", y = " ), (float)Printer::queuePositionLastSteps[Y_AXIS] / Printer::axisStepsPerMM[Y_AXIS] );
            Com::printFLN( PSTR( " [mm]" ) );
        }
        BEEP_ABORT_SET_POSITION
        return;
    }
*/
    // round to Integer [mm]
    g_nScanXMaxPositionSteps =  ((float)Printer::queuePositionLastSteps[X_AXIS] + (float)Printer::directPositionLastSteps[X_AXIS]) / Printer::axisStepsPerMM[X_AXIS];
    g_nScanXMaxPositionSteps *= Printer::axisStepsPerMM[X_AXIS];
    g_nScanYMaxPositionSteps =  ((float)Printer::queuePositionLastSteps[Y_AXIS] + (float)Printer::directPositionLastSteps[Y_AXIS]) / Printer::axisStepsPerMM[Y_AXIS];
    g_nScanYMaxPositionSteps *= Printer::axisStepsPerMM[Y_AXIS];

    if( g_nScanXMaxPositionSteps < g_nScanXStartSteps ||
        g_nScanYMaxPositionSteps < g_nScanYStartSteps )
    {
        // the new end position would be smaller than the current start position - we set the start position to the end position in this case
        if( Printer::debugInfo() )
        {
            Com::printFLN( PSTR( "setScanXYEnd(): the new x/y end position is smaller than the current x/y start position, the x/y start position will be set to the new x/y end position" ) );
            g_nScanXStartSteps = g_nScanXMaxPositionSteps;
            g_nScanYStartSteps = g_nScanYMaxPositionSteps;
        }
    }
    
    if( Printer::debugInfo() )
    {
        Com::printFLN( PSTR( "setScanXYEnd(): the new x/y end position has been set" ) );
        Com::printF( PSTR( "start: x = " ), (float)g_nScanXStartSteps / Printer::axisStepsPerMM[X_AXIS] );
        Com::printF( PSTR( ", y = " ), (float)g_nScanYStartSteps / Printer::axisStepsPerMM[Y_AXIS] );
        Com::printFLN( PSTR( " [mm]" ) );
        Com::printF( PSTR( "end: x = " ), (float)g_nScanXMaxPositionSteps / Printer::axisStepsPerMM[X_AXIS] );
        Com::printF( PSTR( ", y = " ), (float)g_nScanYMaxPositionSteps / Printer::axisStepsPerMM[Y_AXIS] );
        Com::printFLN( PSTR( " [mm]" ) );
    }
    BEEP_ACCEPT_SET_POSITION
    return;

} // setScanXYEnd
#endif // FEATURE_MILLING_MODE

#if FEATURE_HEAT_BED_Z_COMPENSATION
void switchActiveHeatBed( char newActiveHeatBed )
{
    if( newActiveHeatBed < 1 || newActiveHeatBed > EEPROM_MAX_HEAT_BED_SECTORS )
    {
        // do not allow not-supported z-compensation matrix
        return;
    }

    g_nActiveHeatBed = newActiveHeatBed;
    writeWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, EEPROM_OFFSET_ACTIVE_HEAT_BED_Z_MATRIX, g_nActiveHeatBed );

    if( loadCompensationMatrix( (unsigned int)(EEPROM_SECTOR_SIZE * g_nActiveHeatBed) ) )
    {
        // there is no valid z-compensation matrix available
        initCompensationMatrix();
    }
    return;

} // switchActiveHeatBed
#endif // FEATURE_HEAT_BED_Z_COMPENSATION

#if FEATURE_RGB_LIGHT_EFFECTS
void setRGBTargetColors( uint8_t R, uint8_t G, uint8_t B )
{
    g_uRGBTargetR = R;
    g_uRGBTargetG = G;
    g_uRGBTargetB = B;

} // setRGBTargetColors


void setRGBLEDs( uint8_t R, uint8_t G, uint8_t B )
{
    if( R > 0 )
    {
        TCCR4A |= (1<<COM4A1);                      // R > 0 PWM start
        OCR4A  =  R*80;
    }                       
    else 
    {                                           
        TCCR4A &= ~(1<<COM4A1);                     // R = 0 PWM stop
    }

    if( G > 0 )
    {
        TCCR4A |= (1<<COM4B1);                      // G > 0 PWM start
        OCR4B  =  G*80;
    }                           
    else
    {
        TCCR4A &= ~(1<<COM4B1);                     // G = 0 PWM stop
    }                           

    if( B > 0 )
    {
        TCCR4A |= (1<<COM4C1);                      // B > 0 PWM start
        OCR4C  =  B*80;
    }                       
    else
    {
        TCCR4A &= ~(1<<COM4C1);                     // B = 0 PWM stop
    }                       

    g_uRGBCurrentR = R;
    g_uRGBCurrentG = G;
    g_uRGBCurrentB = B;

} // setRGBLEDs


void updateRGBLightStatus( void )
{
    char    newStatus = RGB_STATUS_IDLE;
    char    mode;


    if( Printer::RGBButtonBackPressed )
    {
        // toggle the white light
        Printer::RGBLightModeForceWhite = !Printer::RGBLightModeForceWhite;

        if( Printer::RGBLightModeForceWhite )
        {
            setRGBTargetColors( 255, 255, 255 );
        }
        else
        {
            if ( Printer::RGBLightMode == RGB_MODE_OFF )
            {
                setRGBTargetColors( 0, 0, 0 );
            }
            else if ( Printer::RGBLightMode == RGB_MODE_AUTOMATIC )
            {
                Printer::RGBLightStatus = RGB_STATUS_AUTOMATIC;
            }
            else if ( Printer::RGBLightMode == RGB_MODE_MANUAL )
            {
                setRGBTargetColors( g_uRGBManualR, g_uRGBManualG, g_uRGBManualB );
            }
        }

        Printer::RGBButtonBackPressed = 0;
    }

    if( Printer::RGBLightModeForceWhite )
    {
        // there is nothing to do in case we shall display the white light
        return;
    }

    if( Printer::RGBLightStatus == RGB_STATUS_NOT_AUTOMATIC )
    {
        // there is nothing to do in case we shall not change the RGB colors automatically
        return;
    }

#if FEATURE_MILLING_MODE
    mode = Printer::operatingMode;
#else
    mode = OPERATING_MODE_PRINT;
#endif // FEATURE_MILLING_MODE

    if( mode == OPERATING_MODE_PRINT )
    {
        // operating mode print
#if NUM_EXTRUDER >= 1
        if( extruder[0].tempControl.targetTemperatureC > EXTRUDER_MIN_TEMP )
        {
            if( fabs( extruder[0].tempControl.targetTemperatureC - extruder[0].tempControl.currentTemperatureC ) < RGB_LIGHT_TEMP_TOLERANCE )
            {
                // we have reached the target temperature
                newStatus = RGB_STATUS_PRINTING;
            }
            else if( extruder[0].tempControl.targetTemperatureC > extruder[0].tempControl.currentTemperatureC )
            {
                // we are still heating
                newStatus = RGB_STATUS_HEATING;
            }
            else
            {
                // we end up here in case the target temperature is below the current temperature (this happens typically when the target temperature is reduced after the first layer)
            }
        }
#endif // NUM_EXTRUDER >= 1

#if NUM_EXTRUDER == 2
        if( extruder[1].tempControl.targetTemperatureC > EXTRUDER_MIN_TEMP )
        {
            if( fabs( extruder[1].tempControl.targetTemperatureC - extruder[1].tempControl.currentTemperatureC ) < RGB_LIGHT_TEMP_TOLERANCE )
            {
                if( newStatus == RGB_STATUS_IDLE )
                {
                    // we have reached the target temperature
                    newStatus = RGB_STATUS_PRINTING;
                }
            }
            else if( extruder[1].tempControl.targetTemperatureC > extruder[1].tempControl.currentTemperatureC )
            {
                // we are still heating
                newStatus = RGB_STATUS_HEATING;
            }
            else
            {
                // we end up here in case the target temperature is below the current temperature (this happens typically when the target temperature is reduced after the first layer)
            }
        }
#endif // NUM_EXTRUDER == 2

        if( heatedBedController.targetTemperatureC > HEATED_BED_MIN_TEMP )
        {
            if( fabs( heatedBedController.targetTemperatureC - heatedBedController.currentTemperatureC ) < RGB_LIGHT_TEMP_TOLERANCE )
            {
                // we have reached the target temperature
                if( newStatus == RGB_STATUS_IDLE )
                {
                    newStatus = RGB_STATUS_PRINTING;
                }
            }
            else if( heatedBedController.targetTemperatureC > heatedBedController.currentTemperatureC )
            {
                // we are still heating
                newStatus = RGB_STATUS_HEATING;
            }
                    else
            {
                // we end up here in case the target temperature is below the current temperature (this happens typically when the target temperature is reduced after the first layer)
            }
        }

#if NUM_EXTRUDER >= 1
        if( (extruder[0].tempControl.currentTemperatureC - extruder[0].tempControl.targetTemperatureC) > COOLDOWN_THRESHOLD )
        {
            // we shall cool down
            if( newStatus == RGB_STATUS_IDLE )
            {
                newStatus = RGB_STATUS_COOLING;
            }
        }
#endif // NUM_EXTRUDER >= 1

#if NUM_EXTRUDER == 2
        if( (extruder[1].tempControl.currentTemperatureC - extruder[1].tempControl.targetTemperatureC) > COOLDOWN_THRESHOLD )
        {
            // we shall cool down
            if( newStatus == RGB_STATUS_IDLE )
            {
                newStatus = RGB_STATUS_COOLING;
            }
        }
#endif // NUM_EXTRUDER == 2

        if( (heatedBedController.currentTemperatureC - heatedBedController.targetTemperatureC) > COOLDOWN_THRESHOLD )
        {
            // we shall cool down
            if( newStatus == RGB_STATUS_IDLE )
            {
                newStatus = RGB_STATUS_COOLING;
            }
        }
    }
    else
    {
        // operating mode mill
        if( PrintLine::linesCount )
        {
            newStatus = RGB_STATUS_PRINTING;
        }
        else
        {
            newStatus = RGB_STATUS_IDLE;
        }
    }

    if( newStatus != Printer::RGBLightStatus )
    {
        if( newStatus == RGB_STATUS_IDLE && Printer::RGBLightStatus == RGB_STATUS_COLOR_CHANGE )
        {
            // when we are in color change mode already we shall not switch back to idle
        }
        else
        {
            Printer::RGBLightStatus = newStatus;
//          Com::printFLN( PSTR( "new RGB light status: " ), Printer::RGBLightStatus );

            switch( Printer::RGBLightStatus )
            {
                case RGB_STATUS_PRINTING:
                {
                    setRGBTargetColors( g_uRGBPrintingR, g_uRGBPrintingG, g_uRGBPrintingB );
                    break;
                }
                case RGB_STATUS_HEATING:
                {
                    setRGBTargetColors( g_uRGBHeatingR, g_uRGBHeatingG, g_uRGBHeatingB );
                    break;
                }
                case RGB_STATUS_COOLING:
                {
                    setRGBTargetColors( g_uRGBCoolingR, g_uRGBCoolingG, g_uRGBCoolingB );
                    break;
                }
                case RGB_STATUS_IDLE:   // fall through
                default:
                {
                    Printer::RGBLightIdleStart = HAL::timeInMilliseconds();
                    Printer::RGBLightStatus    = RGB_STATUS_IDLE;
                    setRGBTargetColors( g_uRGBIdleR, g_uRGBIdleG, g_uRGBIdleB );
                    break;
                }
            }
        }
    }

    if( Printer::RGBLightStatus == RGB_STATUS_IDLE && Printer::RGBLightIdleStart )
    {
        if( (HAL::timeInMilliseconds() - Printer::RGBLightIdleStart) / 1000 > RGB_LIGHT_COLOR_CHANGE_DELAY )
        {
            Printer::RGBLightStatus    = RGB_STATUS_COLOR_CHANGE;
            Printer::RGBLightIdleStart = 0;

//          Com::printFLN( PSTR( "new RGB light status: " ), Printer::RGBLightStatus );
            setRGBTargetColors( 255, 0, 0 );
        }
    }

    if( Printer::RGBLightStatus == RGB_STATUS_COLOR_CHANGE )
    {
        if( g_uRGBTargetR == g_uRGBCurrentR && 
            g_uRGBTargetG == g_uRGBCurrentG &&
            g_uRGBTargetB == g_uRGBCurrentB )
        {
            if( g_uRGBTargetR == 255 && g_uRGBTargetG == 0 && g_uRGBTargetB == 0 )
            {
                g_uRGBTargetG = 255;
            }
            else if( g_uRGBTargetR == 255 && g_uRGBTargetG == 255 && g_uRGBTargetB == 0 )
            {
                g_uRGBTargetR = 0;
            }
            else if( g_uRGBTargetR == 0 && g_uRGBTargetG == 255 && g_uRGBTargetB == 0 )
            {
                g_uRGBTargetB = 255;
            }
            else if( g_uRGBTargetR == 0 && g_uRGBTargetG == 255 && g_uRGBTargetB == 255 )
            {
                g_uRGBTargetG = 0;
            }
            else if( g_uRGBTargetR == 0 && g_uRGBTargetG == 0 && g_uRGBTargetB == 255 )
            {
                g_uRGBTargetR = 255;
            }
            else if( g_uRGBTargetR == 255 && g_uRGBTargetG == 0 && g_uRGBTargetB == 255 )
            {
                g_uRGBTargetB = 0;
            }
        }
    }

    return;

} // updateRGBLightStatus
#endif // FEATURE_RGB_LIGHT_EFFECTS


void setupForPrinting( void )
{
    Printer::flag0 &= ~PRINTER_FLAG0_TEMPSENSOR_DEFECT;

#if FEATURE_HEAT_BED_Z_COMPENSATION

    // restore the default scan parameters
    restoreDefaultScanParameters();
    
    g_nActiveHeatBed = (char)readWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, EEPROM_OFFSET_ACTIVE_HEAT_BED_Z_MATRIX );

    if( g_nActiveHeatBed < 1 || g_nActiveHeatBed > EEPROM_MAX_HEAT_BED_SECTORS )
    {
        if( Printer::debugErrors() )
        {
            Com::printFLN( PSTR( "setupForPrinting(): invalid active heat bed z matrix detected: " ), (int)g_nActiveHeatBed );
        }

        // continue with the default heat bed z matrix
        g_nActiveHeatBed = 1;
    }

#endif // FEATURE_HEAT_BED_Z_COMPENSATION

#if EEPROM_MODE
    // read the settings from the EEPROM
    Printer::homingFeedrate[X_AXIS] = HAL::eprGetFloat(EPR_X_HOMING_FEEDRATE_PRINT);
    Printer::homingFeedrate[Y_AXIS] = HAL::eprGetFloat(EPR_Y_HOMING_FEEDRATE_PRINT);
    Printer::homingFeedrate[Z_AXIS] = HAL::eprGetFloat(EPR_Z_HOMING_FEEDRATE_PRINT);
#else
    // read the settings from Configuration.h
    Printer::homingFeedrate[X_AXIS] = HOMING_FEEDRATE_X_PRINT;
    Printer::homingFeedrate[Y_AXIS] = HOMING_FEEDRATE_Y_PRINT;
    Printer::homingFeedrate[Z_AXIS] = HOMING_FEEDRATE_Z_PRINT;
#endif // EEPROM_MODE

#if MOTHERBOARD == DEVICE_TYPE_RF2000
    Printer::lengthMM[X_AXIS] = X_MAX_LENGTH_PRINT;
    HAL::eprSetFloat(EPR_X_LENGTH,Printer::lengthMM[X_AXIS]);
    EEPROM::updateChecksum();
#endif // MOTHERBOARD == DEVICE_TYPE_RF2000
    Printer::updateDerivedParameter();

    g_staticZSteps = (Printer::ZOffset * Printer::axisStepsPerMM[Z_AXIS]) / 1000;

    Printer::setMenuMode( MENU_MODE_MILLER, false );
    Printer::setMenuMode( MENU_MODE_PRINTER, true );
    
    g_uStartOfIdle = HAL::timeInMilliseconds();
    return;

} // setupForPrinting


void setupForMilling( void )
{

#if FEATURE_WORK_PART_Z_COMPENSATION

    // we must restore the default work part scan parameters
    restoreDefaultScanParameters();

    g_nActiveWorkPart = (char)readWord24C256( I2C_ADDRESS_EXTERNAL_EEPROM, EEPROM_OFFSET_ACTIVE_WORK_PART_Z_MATRIX );

    if( g_nActiveWorkPart < 1 || g_nActiveWorkPart > EEPROM_MAX_WORK_PART_SECTORS )
    {
        if( Printer::debugErrors() )
        {
            Com::printFLN( PSTR( "setupForMilling(): invalid active work part detected: " ), (int)g_nActiveWorkPart );
        }

        // continue with the default work part
        g_nActiveWorkPart = 1;
    }

#endif // FEATURE_WORK_PART_Z_COMPENSATION

#if EEPROM_MODE
    // read the settings from the EEPROM
    Printer::homingFeedrate[X_AXIS] = HAL::eprGetFloat(EPR_X_HOMING_FEEDRATE_MILL);
    Printer::homingFeedrate[Y_AXIS] = HAL::eprGetFloat(EPR_Y_HOMING_FEEDRATE_MILL);
    Printer::homingFeedrate[Z_AXIS] = HAL::eprGetFloat(EPR_Z_HOMING_FEEDRATE_MILL);
#else
    // read the settings from Configuration.h
    Printer::homingFeedrate[X_AXIS] = HOMING_FEEDRATE_X_MILL;
    Printer::homingFeedrate[Y_AXIS] = HOMING_FEEDRATE_Y_MILL;
    Printer::homingFeedrate[Z_AXIS] = HOMING_FEEDRATE_Z_MILL;
#endif // EEPROM_MODE

    // disable all heaters
    Extruder::setHeatedBedTemperature( 0, false );
    Extruder::setTemperatureForExtruder( 0, 0, false );

#if MOTHERBOARD == DEVICE_TYPE_RF2000
    Printer::lengthMM[X_AXIS] = X_MAX_LENGTH_MILL;
    HAL::eprSetFloat(EPR_X_LENGTH,Printer::lengthMM[X_AXIS]);
    EEPROM::updateChecksum();
#endif // MOTHERBOARD == DEVICE_TYPE_RF2000
    Printer::updateDerivedParameter();

    g_staticZSteps = 0;

    Printer::setMenuMode( MENU_MODE_PRINTER, false );
    Printer::setMenuMode( MENU_MODE_MILLER, true );
    
    g_uStartOfIdle = HAL::timeInMilliseconds();
    return;

} // setupForMilling


void prepareZCompensation( void )
{
    char    mode;

    if( COMPENSATION_MATRIX_SIZE > EEPROM_SECTOR_SIZE )
    {
        if( Printer::debugErrors() )
        {
            Com::printFLN( PSTR( "prepareZCompensation(): the size of the compensation matrix is too big" ) );
        }

        // TODO: show a message at the display in this case
        return;
    }

#if FEATURE_MILLING_MODE
    mode = Printer::operatingMode;
#else
    mode = OPERATING_MODE_PRINT;
#endif // FEATURE_MILLING_MODE

#if FEATURE_HEAT_BED_Z_COMPENSATION
    if( mode == OPERATING_MODE_PRINT )
    {
        // restore the default scan parameters
        restoreDefaultScanParameters();
    
        // restore the last known compensation matrix
        // this operation must be performed after restoring of the default scan parameters because the values from the EEPROM can overwrite some scan parameters
        if( loadCompensationMatrix( (unsigned int)(EEPROM_SECTOR_SIZE * g_nActiveHeatBed ) ) )
        {
            // there is no valid compensation matrix available
            initCompensationMatrix();

            if( Printer::debugErrors() )
            {
                Com::printFLN( PSTR( "prepareZCompensation(): the compensation matrix is not available" ) );
            }
        }
    }
#endif // FEATURE_HEAT_BED_Z_COMPENSATION

#if FEATURE_WORK_PART_Z_COMPENSATION
    if( mode == OPERATING_MODE_MILL )
    {
        // we must restore the default work part scan parameters
        restoreDefaultScanParameters();

        // we must restore the work part z-compensation matrix
        // this operation must be performed after restoring of the default scan parameters because the values from the EEPROM can overwrite some scan parameters
        if( loadCompensationMatrix( (EEPROM_SECTOR_SIZE *9) + (unsigned int)(EEPROM_SECTOR_SIZE * g_nActiveWorkPart) ) )
        {
            // there is no valid compensation matrix available
            initCompensationMatrix();

            if( Printer::debugErrors() )
            {
                Com::printFLN( PSTR( "prepareZCompensation(): the compensation matrix is not available" ) );
            }
        }
    }
#endif // FEATURE_WORK_PART_Z_COMPENSATION

} // prepareZCompensation


void resetZCompensation( void )
{
    if(g_ZOSScanStatus == 0) Com::printFLN( PSTR( "resetZCompensation()" ) ); //nur melden, wenn es ausserhalb dem scan stattfindet.

    InterruptProtectedBlock noInts; //HAL::forbidInterrupts();

    // disable and reset the z-compensation

#if FEATURE_HEAT_BED_Z_COMPENSATION
    Printer::doHeatBedZCompensation = 0;
#endif // FEATURE_HEAT_BED_Z_COMPENSATION

#if FEATURE_WORK_PART_Z_COMPENSATION
    Printer::doWorkPartZCompensation = 0;
    Printer::staticCompensationZ     = 0;
#endif // FEATURE_WORK_PART_Z_COMPENSATION

#if FEATURE_HEAT_BED_Z_COMPENSATION || FEATURE_WORK_PART_Z_COMPENSATION
    Printer::compensatedPositionTargetStepsZ  = 0;
    Printer::compensatedPositionCurrentStepsZ = 0;
    Printer::endZCompensationStep             = 0;
#endif // FEATURE_HEAT_BED_Z_COMPENSATION || FEATURE_WORK_PART_Z_COMPENSATION

    noInts.unprotect(); //HAL::allowInterrupts();
    return;

} // resetZCompensation


unsigned char isSupportedGCommand( unsigned int currentGCode, char neededMode, char outputLog )
{
    char    currentMode = OPERATING_MODE_PRINT;


#if FEATURE_MILLING_MODE
    if( Printer::operatingMode == OPERATING_MODE_MILL )
    {
        currentMode = OPERATING_MODE_MILL;
    }
#endif // FEATURE_MILLING_MODE

    if( currentMode == neededMode )
    {
        return 1;
    }

    if( Printer::debugErrors() && outputLog )
    {
        Com::printF( PSTR( "G" ), (int)currentGCode );
        Com::printFLN( PSTR( ": this command is not supported in this mode" ) );
    }

    // TODO: shall we show an error message at the display here?
    return 0;

} // isSupportedGCommand


unsigned char isSupportedMCommand( unsigned int currentMCode, char neededMode, char outputLog )
{
    char    currentMode = OPERATING_MODE_PRINT;


#if FEATURE_MILLING_MODE
    if( Printer::operatingMode == OPERATING_MODE_MILL )
    {
        currentMode = OPERATING_MODE_MILL;
    }
#endif // FEATURE_MILLING_MODE

    if( currentMode == neededMode )
    {
        return 1;
    }

    if( Printer::debugErrors() && outputLog )
    {
        Com::printF( PSTR( "M" ), (int)currentMCode );
        Com::printFLN( PSTR( ": this command is not supported in this mode" ) );
    }

    // TODO: shall we show an error message at the display here?
    return 0;

} // isSupportedMCommand


unsigned char isMovingAllowed( const char* pszCommand, char outputLog )
{
#if FEATURE_UNLOCK_MOVEMENT
    if( 
#if FEATURE_MILLING_MODE
        Printer::operatingMode == OPERATING_MODE_PRINT && 
#endif // FEATURE_MILLING_MODE
        !Printer::g_unlock_movement ) //!Printer::isHomed()
    {
        // do not allow to move in case the printer just started and has no homing
        // this is a temporary fix to the wall-crashes when the printer resets
        Com::printF( pszCommand );
        Com::printFLN( PSTR( "FEATURE_UNLOCK_MOVEMENT: move forbidden because not (homed/temp set/buttons pressed)" ) );
        return 0;
    }
#endif //FEATURE_UNLOCK_MOVEMENT
    if( Printer::blockAll )
    {
        // do not allow to move in case the movements have been blocked
        return 0;
    }

#if FEATURE_HEAT_BED_Z_COMPENSATION
    if( g_nHeatBedScanStatus || g_ZOSScanStatus )
    {
        // do not allow manual movements while the heat bed scan is in progress
        if( Printer::debugErrors() && outputLog )
        {
            Com::printF( pszCommand );
            Com::printFLN( PSTR( ": this command can not be used while the heat bed scan is in progress" ) );
        }
        return 0;
    }
#endif // FEATURE_HEAT_BED_Z_COMPENSATION
        
#if FEATURE_WORK_PART_Z_COMPENSATION
    if( g_nWorkPartScanStatus )
    {
        // do not allow manual movements while the work part scan is in progress
        if( Printer::debugErrors() && outputLog )
        {
            Com::printF( pszCommand );
            Com::printFLN( PSTR( ": this command can not be used while the work part scan is in progress" ) );
        }
        return 0;
    }
#endif // FEATURE_WORK_PART_Z_COMPENSATION
        
#if FEATURE_FIND_Z_ORIGIN
    if( g_nFindZOriginStatus && g_nFindZOriginStatus != 30 )
    {
        // do not allow manual movements while the z-origin is searched
        if( Printer::debugErrors() && outputLog )
        {
            Com::printF( pszCommand );
            Com::printFLN( PSTR( ": this command can not be used while the z-origin is searched" ) );
        }
        return 0;
    }
#endif // FEATURE_FIND_Z_ORIGIN

#if FEATURE_CONFIGURABLE_Z_ENDSTOPS
    if( Printer::ZEndstopUnknown )
    {
        // in case we do not know which Z-endstop is active at the moment, we do not allow to move until a z-homing has been performed
        if( Printer::debugErrors() && outputLog )
        {
            Com::printF( pszCommand );
            Com::printFLN( PSTR( ": this command can not be used until a z-homing has been performed" ) );
        }
        return 0;
    }
#endif // FEATURE_CONFIGURABLE_Z_ENDSTOPS

    // we allow the manual movements at the moment
    return 1;

} // isMovingAllowed


unsigned char isHomingAllowed( GCode* com, char outputLog )
{
#if FEATURE_HEAT_BED_Z_COMPENSATION
    if( g_nHeatBedScanStatus || g_ZOSScanStatus )
    {
        // do not allow homing while the heat bed scan is in progress
        if( Printer::debugErrors() && outputLog )
        {
            Com::printFLN( PSTR( "G28: homing can not be performed while the heat bed scan is in progress" ) );
        }

        showError( (void*)ui_text_home, (void*)ui_text_operation_denied );
        return 0;
    }
#endif // FEATURE_HEAT_BED_Z_COMPENSATION
        
#if FEATURE_WORK_PART_Z_COMPENSATION
    if( g_nWorkPartScanStatus )
    {
        // do not allow homing while the work part scan is in progress
        if( Printer::debugErrors() && outputLog )
        {
            Com::printFLN( PSTR( "G28: homing can not be performed while the work part scan is in progress" ) );
        }

        showError( (void*)ui_text_home, (void*)ui_text_operation_denied );
        return 0;
    }
#endif // FEATURE_WORK_PART_Z_COMPENSATION
        
#if FEATURE_FIND_Z_ORIGIN
    if( g_nFindZOriginStatus )
    {
        // do not allow homing while the z-origin is searched
        if( Printer::debugErrors() && outputLog )
        {
            Com::printFLN( PSTR( "G28: homing can not be performed while the z-origin is searched" ) );
        }

        showError( (void*)ui_text_home, (void*)ui_text_operation_denied );
        return 0;
    }
#endif // FEATURE_FIND_Z_ORIGIN

    if( !com )
    {
        // there is nothing more which we can check
        return 1;
    }

#if FEATURE_CONFIGURABLE_Z_ENDSTOPS
    if( Printer::ZEndstopUnknown && (com->hasX() || com->hasY() || com->hasNoXYZ()) )
    {
        // in case we do not know which Z-endstop is active at the moment, we do not allow any homing except the z-homing
        if( Printer::debugErrors() && outputLog )
        {
            Com::printFLN( PSTR( "G28: x/y homing can not be performed until a z-homing has been performed" ) );
        }

        // turn off the homing in x- and y-direction
        com->setX( 0 );
        com->setY( 0 );

        if( com->hasZ() || com->hasNoXYZ() )
        {
            com->setZ( 1 );

            if( Printer::debugInfo() )
            {
                Com::printF( PSTR( "isHomingAllowed(): x=" ), com->hasX() );
                Com::printF( PSTR( ", y=" ), com->hasY() );
                Com::printFLN( PSTR( ", z=" ), com->hasZ() );
            }
            return 1;
        }

        showError( (void*)ui_text_home, (void*)ui_text_operation_denied );
        return 0;
    }
#endif // FEATURE_CONFIGURABLE_Z_ENDSTOPS

    // we allow the homing at the moment
    return 1;

} // isHomingAllowed


void showInvalidSyntax( unsigned int currentMCode )
{
    if( Printer::debugErrors() )
    {
        Com::printF( PSTR( "M" ), (int)currentMCode );
        Com::printFLN( PSTR( ": invalid syntax" ) );
    }
    return;

} // showInvalidSyntax


void addUInt32( char* pszString, uint32_t uNumber )
{
    char        szTemp[11];
    char*       pszTemp = &szTemp[10];
    uint32_t    uTemp;
    
    
    *pszTemp = '\0';
    do
    {
        uTemp      =  uNumber;
        uNumber    /= 10;
        *--pszTemp =  '0' + (uTemp - 10 * uNumber);
    }while(uNumber);

    strcpy( pszString, pszTemp );

} // addUInt32


void addFloat( char* pszString, float fNumber, uint8_t uDigits )
{
    float       fRounding = 0.5;
    uint32_t    uNumber;
    float       fRemainder;
    uint8_t     uRemainder;
    uint8_t     i;


    // add at the end of the string
    while( *pszString )
    {
        pszString ++;
    }

    if( isnan( fNumber ) )
    {
        strcpy( pszString, "NAN" );
        return;
    }
    if( isinf( fNumber ) )
    {
        strcpy( pszString, "INF" );
        return;
    }

    // Handle negative numbers
    if( fNumber < 0.0 )
    {
        *pszString = '-';
        pszString ++;
        *pszString = '\0';
        fNumber = -fNumber;
    }

    for( i=0; i<uDigits; i++ )
    {
        fRounding /= 10.0;
    }

    fNumber += fRounding;

    uNumber    = (uint32_t)fNumber;
    fRemainder = fNumber - (float)uNumber;
    addUInt32( pszString, uNumber );

    if( uDigits > 0 )
    {
        while( *pszString )
        {
            pszString ++;
        }

        *pszString = '.';
        pszString ++;

        while( uDigits-- > 0 )
        {
            fRemainder *= 10.0;
            uRemainder =  (uint8_t)fRemainder;
            *pszString =  '0' + uRemainder;
            pszString ++;
            fRemainder -= uRemainder;
        }

        *pszString = '\0';
    }

} // addFloat


#if FEATURE_HEAT_BED_TEMP_COMPENSATION
float getHeatBedTemperatureOffset( float temperatureInCelsius )
{
    const unsigned char     setpointTemperatures[] = BED_SETPOINT_TEMPERATURES;
    const unsigned char     measuredTemperatures[] = BED_MEASURED_TEMPERATURES;
    float                   deltaLow, deltaHigh;
    float                   temp;
    unsigned char                   i;


    if( temperatureInCelsius <= setpointTemperatures[0] )
    {
        // the specified temperature is below our known, measured values
        deltaLow  = 0;
        deltaHigh = measuredTemperatures[0] - setpointTemperatures[0];

        temp =  (deltaHigh - deltaLow) / float(setpointTemperatures[0]);
        temp *= temperatureInCelsius;

#if DEBUG_HEAT_BED_TEMP_COMPENSATION
        if( temperatureInCelsius > 50 )
        {
            Com::printF( PSTR( "getHeatBedTemperatureOffset(): " ), temperatureInCelsius );
            Com::printFLN( PSTR( ", " ), temp );
        }
#endif // DEBUG_HEAT_BED_TEMP_COMPENSATION

        return temp;
    }

    if( temperatureInCelsius >= setpointTemperatures[BED_TEMP_COMPENSATION_INDEX_MAX] )
    {
        // the specified temperature is above our known, measured values
        temp = measuredTemperatures[BED_TEMP_COMPENSATION_INDEX_MAX] - setpointTemperatures[BED_TEMP_COMPENSATION_INDEX_MAX];

#if DEBUG_HEAT_BED_TEMP_COMPENSATION
        Com::printF( PSTR( "getHeatBedTemperatureOffset(): " ), temperatureInCelsius );
        Com::printFLN( PSTR( ", " ), temp );
#endif // DEBUG_HEAT_BED_TEMP_COMPENSATION

        return temp;
    }

    for( i=1; i<=BED_TEMP_COMPENSATION_INDEX_MAX; i++ )
    {
        if( temperatureInCelsius < setpointTemperatures[i] )
        {
            deltaLow  = measuredTemperatures[i-1] - setpointTemperatures[i-1];
            deltaHigh = measuredTemperatures[i]   - setpointTemperatures[i];

            temp =  (deltaHigh - deltaLow) / float(setpointTemperatures[i] - setpointTemperatures[i-1]);
            temp *= (temperatureInCelsius - setpointTemperatures[i-1]);

#if DEBUG_HEAT_BED_TEMP_COMPENSATION
            Com::printF( PSTR( "getHeatBedTemperatureOffset(): " ), temperatureInCelsius );
            Com::printFLN( PSTR( ", " ), temp + deltaLow );
#endif // DEBUG_HEAT_BED_TEMP_COMPENSATION

            return (temp + deltaLow);
        }
    }

    // we should never end up here
    return 0;

} // getHeatBedTemperatureOffset
#endif // FEATURE_HEAT_BED_TEMP_COMPENSATION


#if FEATURE_TYPE_EEPROM
void determineHardwareType( void )
{
    unsigned short  uTemp;


    Printer::wrongType = 0;

    // check the stored header format
    uTemp = readWord24C256( I2C_ADDRESS_TYPE_EEPROM, TYPE_EEPROM_OFFSET_HEADER_FORMAT );

    if( uTemp != TYPE_EEPROM_FORMAT )
    {
        // we could not read the header format or the header format is wrong
        writeWord24C256( I2C_ADDRESS_TYPE_EEPROM, TYPE_EEPROM_OFFSET_HEADER_FORMAT, TYPE_EEPROM_FORMAT );
        writeWord24C256( I2C_ADDRESS_TYPE_EEPROM, TYPE_EEPROM_OFFSET_BOARD_TYPE,    MOTHERBOARD );

        uTemp = readWord24C256( I2C_ADDRESS_TYPE_EEPROM, TYPE_EEPROM_OFFSET_HEADER_FORMAT );

        if( uTemp != TYPE_EEPROM_FORMAT )
        {
            if( MOTHERBOARD == DEVICE_TYPE_RF1000 )
            {
                // the RF1000 does not provide the type EEPROM, thus we should run at an RF1000 board when we end up here
                return;
            }

            // we end up here in case this firmware is for the RF2000, but the current board does not seem to be an RF2000 board
            notifyAboutWrongHardwareType( DEVICE_TYPE_RF1000 );

            Printer::wrongType = 1;
            return;
        }

        if( MOTHERBOARD == DEVICE_TYPE_RF1000 )
        {
            // we end up here in case this firmware is for the RF1000, but the current board seems to be an RF2000 board
            notifyAboutWrongHardwareType( DEVICE_TYPE_RF2000 );

            Printer::wrongType = 1;
            return;
        }

        // when we end up here this firmware is for the RF2000 and the current board seems to be an RF2000 board
        return;
    }

    if( MOTHERBOARD == DEVICE_TYPE_RF1000 )
    {
        // we end up here in case this firmware is for the RF1000, but the current board seems to be an RF2000 board
        notifyAboutWrongHardwareType( DEVICE_TYPE_RF2000 );

        Printer::wrongType = 1;
        return;
    }

    // when we end up here this firmware is for the RF2000 and the current board seems to be an RF2000 board
    if( readWord24C256( I2C_ADDRESS_TYPE_EEPROM, TYPE_EEPROM_OFFSET_BOARD_TYPE ) != MOTHERBOARD )
    {
        writeWord24C256( I2C_ADDRESS_TYPE_EEPROM, TYPE_EEPROM_OFFSET_BOARD_TYPE, MOTHERBOARD );
    }

    return;

} // determineHardwareType


void notifyAboutWrongHardwareType( unsigned char guessedHardwareType )
{
    uint16_t    duration = 1000;
    uint8_t     count    = 4;


    switch( guessedHardwareType )
    {
        case DEVICE_TYPE_RF1000:
        {
            // we try to beep via the beeper pin of the RF1000 hardware
            SET_OUTPUT( BEEPER_PIN_RF1000 );

            for( uint8_t i=0; i<count; i++ )
            {
                WRITE( BEEPER_PIN_RF1000, HIGH );
                HAL::delayMilliseconds( duration );
                WRITE( BEEPER_PIN_RF1000, LOW );
                HAL::delayMilliseconds( duration );
            }
            break;
        }
        case DEVICE_TYPE_RF2000:
        {
            // we try to beep via the beeper pin of the RF2000 hardware
            SET_OUTPUT( BEEPER_PIN_RF2000 );

            for( uint8_t i=0; i<count; i++ )
            {
                WRITE( BEEPER_PIN_RF2000, HIGH );
                HAL::delayMilliseconds( duration );
                WRITE( BEEPER_PIN_RF2000, LOW );
                HAL::delayMilliseconds( duration );
            }
            break;
        }
    }
    return;

} // notifyAboutWrongHardwareType
#endif // FEATURE_TYPE_EEPROM


void showIdle( void )
{
    char    mode = OPERATING_MODE_PRINT;


#if FEATURE_MILLING_MODE
    mode = Printer::operatingMode;
#endif // FEATURE_MILLING_MODE

    if( mode == OPERATING_MODE_PRINT )
    {
        UI_STATUS( UI_TEXT_PRINTER_READY );
    }
    else
    {
        UI_STATUS( UI_TEXT_MILLER_READY );
    }
    return;

} // showIdle


void showError( const void* line2, const void* line3, const void* line4 )
{
    uid.messageLine1 = (const void*)ui_text_error;
    uid.messageLine2 = line2;
    uid.messageLine3 = line3;
    uid.messageLine4 = line4;

    uid.showMessage( true );
    return;

} // showError


void showWarning( const void* line2, const void* line3, const void* line4 )
{
    uid.messageLine1 = (const void*)ui_text_warning;
    uid.messageLine2 = line2;
    uid.messageLine3 = line3;
    uid.messageLine4 = line4;

    uid.showMessage( true );
    return;

} // showWarning


void showInformation( const void* line2, const void* line3, const void* line4 )
{
    uid.messageLine1 = (const void*)ui_text_information;
    uid.messageLine2 = line2;
    uid.messageLine3 = line3;
    uid.messageLine4 = line4;

    uid.showMessage( true );
    return;

} // showInformation

void showMyPage( const void* line1, const void* line2, const void* line3, const void* line4 )
{
    uid.messageLine1 = line1;
    uid.messageLine2 = line2;
    uid.messageLine3 = line3;
    uid.messageLine4 = line4;

    uid.showMessage( true );
    return;

} // showMyPage

void doEmergencyStop( char reason )
{
    showError( (void*)ui_text_emergency_stop );
    Com::printFLN( PSTR( "RequestStop:" ) ); //tell repetierserver to stop.
    Com::printFLN( PSTR( "// action:disconnect" ) ); //tell octoprint to disconnect

    Com::printF( PSTR( "doEmergencyStop(): block all" ) );
    if( reason == STOP_BECAUSE_OF_Z_MIN )
    {
        Com::printFLN( PSTR( " (Z-Min)" ) );
    }
    else if( reason == STOP_BECAUSE_OF_Z_BLOCK )
    {
        Com::printFLN( PSTR( " (Z-Block)" ) );
    }

    // block any further movement
    Printer::stepperDirection[X_AXIS] = 0;
    Printer::stepperDirection[Y_AXIS] = 0;
    Printer::stepperDirection[Z_AXIS] = 0;
    Printer::blockAll                 = 1;

    moveZ( int(Printer::axisStepsPerMM[Z_AXIS] * 5) );
    Printer::stepperDirection[Z_AXIS] = 0;
   
    // we are not going to perform any further operations until the restart of the firmware
    if( sd.sdmode )
    {
        sd.abortPrint();
    }
    Printer::kill( false );
    return;

} // doEmergencyStop
