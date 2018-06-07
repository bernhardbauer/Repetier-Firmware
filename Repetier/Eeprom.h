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


#ifndef EEPROM_H
#define EEPROM_H

// Id to distinguish version changes
#define EEPROM_PROTOCOL_VERSION         8

/** Where to start with our datablock in memory. Can be moved if you
have problems with other modules using the eeprom */


#define EPR_MAGIC_BYTE                  0
#define EPR_ACCELERATION_TYPE           1
#define EPR_XAXIS_STEPS_PER_MM          3
#define EPR_YAXIS_STEPS_PER_MM          7
#define EPR_ZAXIS_STEPS_PER_MM          11
#define EPR_X_MAX_FEEDRATE              15
#define EPR_Y_MAX_FEEDRATE              19
#define EPR_Z_MAX_FEEDRATE              23
#define EPR_X_HOMING_FEEDRATE_PRINT     27
#define EPR_Y_HOMING_FEEDRATE_PRINT     31
#define EPR_Z_HOMING_FEEDRATE_PRINT     35
#define EPR_MAX_JERK                    39
#define EPR_MAX_ZJERK                   47
#define EPR_X_MAX_ACCEL                 51
#define EPR_Y_MAX_ACCEL                 55
#define EPR_Z_MAX_ACCEL                 59
#define EPR_X_MAX_TRAVEL_ACCEL          63
#define EPR_Y_MAX_TRAVEL_ACCEL          67
#define EPR_Z_MAX_TRAVEL_ACCEL          71
#define EPR_BAUDRATE                    75
#define EPR_MAX_INACTIVE_TIME           79
#define EPR_STEPPER_INACTIVE_TIME       83
#define EPR_EXTRUDER_SPEED              95
#define EPR_INTEGRITY_BYTE              104   // Here the xored sum over eeprom is stored
#define EPR_VERSION                     105   // Version id for updates in EEPROM storage
//#define EPR_BED_HEAT_MANAGER            106
#define EPR_BED_DRIVE_MAX               107
#define EPR_BED_PID_PGAIN               108
#define EPR_BED_PID_IGAIN               112
#define EPR_BED_PID_DGAIN               116
#define EPR_BED_PID_MAX                 120
#define EPR_BED_DRIVE_MIN               124
#define EPR_PRINTING_TIME               125  // Time in seconds printing
#define EPR_PRINTING_DISTANCE           129  // Filament length printed
#define EPR_X_HOME_OFFSET               133
#define EPR_Y_HOME_OFFSET               137
//#define EPR_Z_HOME_OFFSET               141 //z home pos removed by nibbels
#define EPR_X_LENGTH                    145
#define EPR_Y_LENGTH                    149
#define EPR_Z_LENGTH                    153
#define EPR_BACKLASH_X                  157
#define EPR_BACKLASH_Y                  161
#define EPR_BACKLASH_Z                  165

#if FEATURE_SERVICE_INTERVAL
#define EPR_PRINTING_TIME_SERVICE       169  // Time in seconds printing since last service intervall
#define EPR_PRINTING_DISTANCE_SERVICE   173  // Filament length printed since last service intervall
#define EPR_MILLING_TIME_SERVICE        177  // Time in seconds milling since last service intervall
#endif // FEATURE_SERVICE_INTERVAL

#define EPR_MILLING_TIME                181  // Time in seconds milling

#define EPR_RF_BEEPER_MODE              1000
#define EPR_RF_CASE_LIGHT_MODE          1001
#define EPR_RF_230V_OUTPUT_MODE         1002
#define EPR_RF_OPERATING_MODE           1003
#define EPR_X_HOMING_FEEDRATE_MILL      1004
#define EPR_Y_HOMING_FEEDRATE_MILL      1008
#define EPR_Z_HOMING_FEEDRATE_MILL      1012
#define EPR_RF_Z_ENDSTOP_TYPE           1016
#define EPR_RF_HOTEND_TYPE              1017 //depreached 30 07 2017 Nibbels : Die Funktion war nur für RF1000, nur Single, in der Falschen Klasse, wenn nötig, dann in Class Extruder einbauen. Hat beim RF2000 keiner vermisst: Ersatz ist EPR_EXTRUDER_SENSOR_TYPE
#define EPR_RF_MILLER_TYPE              1018
#define EPR_RF_RGB_LIGHT_MODE           1019
#define EPR_RF_FET1_MODE                1020
#define EPR_RF_FET2_MODE                1021
#define EPR_RF_FET3_MODE                1022
#define EPR_RF_RGB_PRINTING_R           1023
#define EPR_RF_RGB_PRINTING_G           1024
#define EPR_RF_RGB_PRINTING_B           1025
#define EPR_RF_RGB_HEATING_R            1026
#define EPR_RF_RGB_HEATING_G            1027
#define EPR_RF_RGB_HEATING_B            1028
#define EPR_RF_RGB_COOLING_R            1029
#define EPR_RF_RGB_COOLING_G            1030
#define EPR_RF_RGB_COOLING_B            1031
#define EPR_RF_RGB_IDLE_R               1032
#define EPR_RF_RGB_IDLE_G               1033
#define EPR_RF_RGB_IDLE_B               1034
#define EPR_RF_RGB_MANUAL_R             1035
#define EPR_RF_RGB_MANUAL_G             1036
#define EPR_RF_RGB_MANUAL_B             1037
#define EPR_RF_Z_OFFSET                 1038
#define EPR_RF_MOVE_MODE_X              1042
#define EPR_RF_MOVE_MODE_Y              1043
#define EPR_RF_MOVE_MODE_Z              1044
#define EPR_RF_Z_MODE                   1045 //das ist byte, 1046 wäre frei.

//vorsicht bei mehrbytigen Variablen, dann freilassen.

#define EPR_RF_MOD_Z_STEP_SIZE            1900 //+1901+1902+1903 g_nManualSteps[Z_AXIS] [4byte unsigned long]

#define EPR_RF_MOD_ZOS_SCAN_POINT_X       1904 //g_ZOSTestPoint[0] [1byte unsigned char]
#define EPR_RF_MOD_ZOS_SCAN_POINT_Y       1905 //g_ZOSTestPoint[1] [1byte unsigned char]

#define EPR_RF_MOD_SENSEOFFSET_OFFSET_MAX 1906 //+1907 g_nSensiblePressureOffsetMax [2byte short]
#define EPR_RF_MOD_SENSEOFFSET_DIGITS     1908 //+1909 g_nSensiblePressureDigits [2byte short]

#define EPR_RF_EMERGENCYPAUSEDIGITSMIN    1910 //+1911+1912+1913 g_nEmergencyPauseDigitsMin [4byte long]
#define EPR_RF_EMERGENCYPAUSEDIGITSMAX    1914 //+1915+1916+1917 g_nEmergencyPauseDigitsMax [4byte long]

#define EPR_RF_EMERGENCYZSTOPDIGITSMIN    1918 //+1919 g_nZEmergencyStopAllMin [2byte short]
#define EPR_RF_EMERGENCYZSTOPDIGITSMAX    1920 //+1921 g_nZEmergencyStopAllMax [2byte short]

#define EPR_RF_HEATED_BED_SENSOR_TYPE     1922 //[1byte]

#define EPR_RF_MOTOR_CURRENT              1923 //1924+1925+1926+1927 [5byte für 5 Motorstrombytes]

#define EPR_RF_FREQ_DBL                   1928 //1929 2byte für uint16 -> frequenz double grenze
#define EPR_RF_FAN_MODE                   1930 //[1byte]
#define EPR_RF_FAN_SPEED                  1931 //[1byte]

#define EPR_RF_MILL_ACCELERATION          1932 //+1933 u short
#define EPR_RF_CAL_STANDARD               1934 //+1935 u short
#define EPR_RF_CAL_ADJUST                 1936 //[1byte]
#define EPR_RF_ZERO_DIGIT_STATE           1937 //[1byte]
#define EPR_RF_DIGIT_CMP_STATE            1938 //[1byte]

#define EPR_X_LENGTH_MILLING              1939 //[+1940+1941+1942 4byte float x-axis length for milling / switching]

#define EPR_RF_MICRO_STEPS_X              1943 //[1byte]
#define EPR_RF_MICRO_STEPS_Y              1944 //[1byte]
#define EPR_RF_MICRO_STEPS_Z              1945 //[1byte]
#define EPR_RF_MICRO_STEPS_E0             1946 //[1byte]
#define EPR_RF_MICRO_STEPS_E1             1947 //[1byte]
#define EPR_RF_MICRO_STEPS_USED           1948 //[1byte]

#define EPR_ZSCAN_START_MM                1949 //[+1950+1951+1952 4byte float]

#define EPR_RF_MOD_SENSEOFFSET_AUTOSTART  1953 //[1byte]

#define EPR_RF_MOD_WOBBLE_FIX_PHASEXY     1954 //[1byte]
#define EPR_RF_MOD_WOBBLE_FIX_AMPX        1955 //[+1956 2byte short]
#define EPR_RF_MOD_WOBBLE_FIX_AMPY1       1957 //[+1958 2byte short]
#define EPR_RF_MOD_WOBBLE_FIX_AMPY2       1959 //[+1960 2byte short]



//Nibbels: Computechecksum geht bis 2047

#define EEPROM_EXTRUDER_OFFSET          200

// bytes per extruder needed, leave some space for future development
#define EEPROM_EXTRUDER_LENGTH          100

// Extruder positions relative to extruder start
#define EPR_EXTRUDER_STEPS_PER_MM       0
#define EPR_EXTRUDER_MAX_FEEDRATE       4

// Feedrate from halted extruder in mm/s
#define EPR_EXTRUDER_MAX_START_FEEDRATE 8

// Acceleration in mm/s^2
#define EPR_EXTRUDER_MAX_ACCELERATION   12
//#define EPR_EXTRUDER_HEAT_MANAGER       16
#define EPR_EXTRUDER_DRIVE_MAX          17
#define EPR_EXTRUDER_PID_PGAIN          18
#define EPR_EXTRUDER_PID_IGAIN          22
#define EPR_EXTRUDER_PID_DGAIN          26
#define EPR_EXTRUDER_PID_MAX            30
#define EPR_EXTRUDER_X_OFFSET           31
#define EPR_EXTRUDER_Y_OFFSET           35
#define EPR_EXTRUDER_WATCH_PERIOD       39
#define EPR_EXTRUDER_ADVANCE_K          41
#define EPR_EXTRUDER_DRIVE_MIN          45
#define EPR_EXTRUDER_ADVANCE_L          46
#define EPR_EXTRUDER_WAIT_RETRACT_TEMP  50
#define EPR_EXTRUDER_WAIT_RETRACT_UNITS 52
#define EPR_EXTRUDER_COOLER_SPEED       54 //byte
#define EPR_EXTRUDER_SENSOR_TYPE        94 //uint8 
#define EPR_EXTRUDER_Z_OFFSET           95 //int32_t -> +96+97+98

class EEPROM
{
#if EEPROM_MODE!=0
    static uint8_t computeChecksum();
    static void writeExtruderPrefix(uint pos);
    static void writeFloat(uint pos,PGM_P text,uint8_t digits=3);
    static void writeLong(uint pos,PGM_P text);
    static void writeInt(uint pos,PGM_P text);
    static void writeByte(uint pos,PGM_P text);
#endif // EEPROM_MODE!=0

public:
    static void init();
    static void initBaudrate();
    static void storeDataIntoEEPROM(uint8_t corrupted=0);
    static void updateChecksum();
    static void readDataFromEEPROM();
    static void restoreEEPROMSettingsFromConfiguration();
    static void initializeAllOperatingModes();
    static void clearEEPROM();
    static void writeSettings();
    static void update(GCode *com);
    static void updatePrinterUsage();
    static int getExtruderOffset(uint8_t extruder=0);

}; // EEPROM


#endif // EEPROM_H
