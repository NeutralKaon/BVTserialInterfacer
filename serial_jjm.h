/* A silly command line wrapper to the Bruker BVT3000 NMR Variable Temperature 
 * Device -- written by Jack J Miller, University of Oxford 2018, but heavily
 * "inspired" by the sterling work of fsc2, an open-source spectrometer library
 * predominantly aimed at EPR and UV-VIS instruments. 
 *
 * Huge thanks to Jens and the open source movement for letting us use the 
 * overly expensive hardware we have already paid for...
 *
 * As an aside, this might be useful as an implementation of the Bi-Synch 
 * protocol for talking to Eurotherm devices (or equivalent others)
 *
 *  ------
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
*/ 
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h> // for sleep function
#include <libserialport.h> // cross platform serial port lib
#include <assert.h>
#include <string.h>
#include <time.h> 



//guards
#pragma once
#if ! defined BVT3000_HEADER
#define BVT3000_HEADER

// Device variables 
#define BAUD_RATE 9600 
#define NUM_DATA_BITS 7
#define NUM_STOP_BITS 1 
#define GROUP_ID 0
#define DEVICE_ID 0 

//Convenience declarations 

#define STX   '\x02'
#define ETX   '\x03'
#define EOT   '\x04'
#define ENQ   '\x05'
#define ACK   '\x06'
#define NAK   '\x15'

#define SP1   0
#define SP2   1

#define AUTOMATIC_MODE  3
#define MANUAL_MODE     0

#define NORMAL_OPERATION    0
#define CONFIGURATION_MODE  2

#define TUNE_OFF            0
#define ADAPTIVE_TUNE       1
#define SELF_TUNE           2

#define HEATER_OK           0
#define HEATER_OVERHEATING  1

#define LN2_OK              0
#define LN2_NEEDS_REFILL    1
#define LN2_TANK_EMPTY      2


/* Defines for the BVT3000 interface status */

#define BVT3000_HEATER_ON                ( 1 <<  0 )
#define BVT3000_EVAPORATOR_CONNECTED     ( 1 <<  2 )
#define BVT3000_MISSING_GAS_FLOW         ( 1 <<  3 )
#define BVT3000_HEATER_OVERHEATING       ( 1 <<  4 )
#define BVT3000_EXCHANGER_CONNECTED      ( 1 <<  5 )
#define BVT3000_LN2_REFILL               ( 1 <<  6 )
#define BVT3000_LN2_EMPTY                ( 1 <<  7 )
#define BVT3000_LN2_HEATER_ON            ( 1 <<  8 )
#define BVT3000_BVBT3500_PRESENT         ( 1 << 10 )

/* Defines for bits in the Eurotherm 902S status word (SW) */

#define DATA_FORMAT_FLAG        0x0001   /* off for free, on for fixed format */
#define SENSOR_BREAK_FLAG       0x0002   /* set on sensor break */
#define KEYLOCK_FLAG            0x0004   /* set if keyboard is disabled */
#define ALARM2_STATE_FLAG       0x0100   /* set if ALARM2 is on */
#define ALARM1_STATE_FLAG       0x0400   /* set if ALARM1 is on */
#define ALARMS_STATE_FLAG       0x1000   /* ALARM1 or ALARM2 is on */
#define ACTIVE_SETPOINT_FLAG    0x2000   /* off for SP1, set for SP2 */
#define REMOTE_ACTIVE_FLAG      0x4000   /* set when in remote state */
#define MANUAL_MODE_FLAG        0x8000   /* set when manual, off in auto mode */


/* Defines for bits in the Eurotherm 902S extension status word (XS) */

#define SELF_TUNE_FLAG          0x01     /* set if in self tune mode */
#define ADAPTIVE_TUNE_FLAG      0x02     /* set if adaptuve tune is on */
#define ENABLE_BROADCAST_FLAG   0x04     /* broadcast enabled when on */
#define PID_CONTROL_FLAG        0x10     /* ? */
#define ACTIVE_PID_FLAG         0x20     /* off: PID1 active, on: PID2 active */


#define MIN_SETPOINT    73.0   /* lowest allowed value for setpoint, 73 K */
#define MAX_SETPOINT  1273.0   /* highest allowed value for setpoint, 1273 K */

#define MAX_PROPORTIONAL_BAND   999.9    /* according to experiments */
#define MAX_INTEGRAL_TIME      9999.0    /* according to experiments */
#define MAX_DERIVATIVE_TIME     999.9    /* according to experiments */
#define MAX_AT_TRIGGER_LEVEL    231.7    /* according to experiments */


#define TEST_STATE                 MANUAL_MODE
#define TEST_TUNE_STATE            ADAPTIVE_TUNE
#define TEST_TEMPERATURE          123.4
#define TEST_SETPOINT             123.4
#define TEST_HEATER_POWER_LIMIT   100.0
#define TEST_HEATER_POWER          50.0
#define TEST_FLOW_RATE           1600.0
#define TEST_PROPORTIONAL_BAND     36.0
#define TEST_INTEGRAL_TIME         17.0
#define TEST_DERIVATIVE_TIME        2.8
#define TEST_CUTBACK_LOW           67.9
#define TEST_CUTBACK_HIGH          56.2
#define TEST_AT_TRIGGER_LEVEL       4.6
#define TEST_DISPLAY_MIN            73.0
#define TEST_DISPLAY_MAX          1273.0
#define TEST_LN2_HEATER_POWER       35.0


/* Maximum time to wait for a reply (in ms) */

#define SERIAL_WAIT  125
#define ACK_WAIT     300

#define FAIL    false
#define OK      true
#define FALSE   false
#define SET     true
#define UNSET   false

#endif
/* End header*/ 


extern bool verboseFlag; 
struct sp_port *port;

//For debugging: 
void bvt3000_comm_fail_debug( char const * caller_name ); 
char* bvt3000_query_debug(const char * cmd, struct sp_port* port_choice , char const * caller_name) ; 
void list_ports(); 



// Low-level communication functions  
int handled_usleep( unsigned long us_dur, bool quit_on_signal);
struct sp_port* open_and_init_port(char* desired_port, struct sp_port *port_choice) ;
void bvt3000_send_command( const char * cmd , struct sp_port* port_choice) ;
size_t bvt3000_add_bcc( unsigned char * data ) ;
char* bvt3000_query(const char * cmd, struct sp_port* port_choice); 
bool bvt3000_check_ack(struct sp_port* port_choice); 
void bvt3000_comm_fail(); 
bool bvt3000_check_bcc(unsigned char* data, unsigned char bcc); 
unsigned int bvt3000_get_interface_status( struct sp_port* port_choice);
unsigned int eurotherm902s_get_sw( struct sp_port* port_choice );
void eurotherm902s_set_os( unsigned int os, struct sp_port* port_choice );
unsigned int eurotherm902s_get_os( struct sp_port* port_choice );
void eurotherm902s_set_xs( unsigned int xs,  struct sp_port* port_choice );
unsigned int eurotherm902s_get_xs( struct sp_port* port_choice );
void eurotherm902s_set_sw( unsigned int sw, struct sp_port* port_choice );

//Flow 
unsigned int bvt3000_get_flow_rate( struct sp_port* port_choice );
void bvt3000_set_flow_rate( unsigned int flow_rate, struct sp_port* port_choice );
unsigned char bvt300_get_port( int port , struct sp_port* port_choice );

//Heater functions
void eurotherm902s_set_heater_power_limit( double power, struct sp_port* port_choice );
double eurotherm902s_get_heater_power_limit( struct sp_port* port_choice );
void eurotherm902s_set_heater_power( double power, struct sp_port* port_choice );
double eurotherm902s_get_heater_power( struct sp_port* port_choice );
int bvt3000_check_heater( struct sp_port* port_choice);
bool bvt3000_get_heater_state( struct sp_port* port_choice);
void bvt3000_set_heater_state( bool state, struct sp_port* port_choice);

//PID controller functions 
void eurotherm902s_set_operation_mode( int op_mode, struct sp_port* port_choice );
bool eurotherm902s_check_sensor_break( struct sp_port* port_choice  );
int eurotherm902s_get_operation_mode( struct sp_port* port_choice );
void eurotherm902s_set_mode( int mode, struct sp_port* port_choice );
int eurotherm902s_get_mode( struct sp_port* port_choice );

double eurotherm902s_get_temperature( struct sp_port* port_choice  );
void eurotherm902s_set_active_setpoint( int sp, struct sp_port* port_choice );
int eurotherm902s_get_active_setpoint( struct sp_port* port_choice );
void eurotherm902s_set_setpoint( int    sp,
                            double temp,
                            struct sp_port* port_choice );

double eurotherm902s_get_setpoint( int sp, struct sp_port* port_choice );
double eurotherm902s_get_working_setpoint( struct sp_port* port_choice );
bool eurotherm902s_get_alarm_state( struct sp_port* port_choice );
void eurotherm902s_set_self_tune_state( bool on_off , struct sp_port* port_choice );
bool eurotherm902s_get_self_tune_state( struct sp_port* port_choice );
void eurotherm902s_set_adaptive_tune_state( bool on_off, struct sp_port* port_choice );
bool eurotherm902s_get_adaptive_tune_state( struct sp_port* port_choice );
void eurotherm902s_set_adaptive_tune_trigger( double tr, struct sp_port* port_choice );
double eurotherm902s_get_adaptive_tune_trigger( struct sp_port* port_choice );
double eurotherm902s_get_min_setpoint( int sp, struct sp_port* port_choice );
double eurotherm902s_get_max_setpoint( int sp, struct sp_port* port_choice );

// PID controls themselves 
void eurotherm902s_set_proportional_band( double pb,  struct sp_port* port_choice );
double eurotherm902s_get_proportional_band( struct sp_port* port_choice );
void eurotherm902s_set_integral_time( double it,  struct sp_port* port_choice );
double eurotherm902s_get_integral_time( struct sp_port* port_choice );
void eurotherm902s_set_derivative_time( double dt , struct sp_port* port_choice );
double eurotherm902s_get_derivative_time(  struct sp_port* port_choice );
void eurotherm902s_set_cutback_high( double cb,  struct sp_port* port_choice  );
double eurotherm902s_get_cutback_high(  struct sp_port* port_choice );
void eurotherm902s_set_cutback_low( double cb,  struct sp_port* port_choice  );
double eurotherm902s_get_cutback_low(  struct sp_port* port_choice );


//Display variables
double eurotherm902s_get_display_maximum( struct sp_port* port_choice);
double eurotherm902s_get_display_minimum( struct sp_port* port_choice);
void eurotherm902s_lock_keyboard( bool lock,struct sp_port* port_choice );

//LN2 functions 
void bvt3000_set_ln2_heater_state( bool state, struct sp_port* port_choice );
bool bvt3000_get_ln2_heater_state( struct sp_port* port_choice);
void bvt3000_set_ln2_heater_power( double p, struct sp_port* port_choice );
double bvt3000_get_ln2_heater_power( struct sp_port* port_choice ); 
int bvt3000_check_ln2_heater( struct sp_port* port_choice);

//Helper function
void printArray(char* buf) ;

