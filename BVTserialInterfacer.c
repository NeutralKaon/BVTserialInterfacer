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
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#include "cmdline.h"
#include "serial_jjm.h"
#include "convenient_wrapper_functions.h" 

bool verboseFlag = false; 
struct sp_port *port; 

int main(int argc, char **argv){ 
    struct gengetopt_args_info ai; 
    /* ----- Parse arguments ---- */ 

    if (cmdline_parser(argc, argv, &ai) != 0){
        return 1; 
    }

    if(ai.list_devices_given) { 
        list_ports();  
        return 0; /* quit */ 
    }

    /* --- Open device ---*/  

    port = open_and_init_port(ai.device_arg, port); 
    int rV = sp_last_error_code(); 
    if (rV != 0) { 
    #ifndef DEBUG
        fprintf(stderr,"Unable to open and initialise serial port %s. Quitting.\n", ai.device_arg); 
        return(-1); 
    #else 
        fprintf(stderr,"WARNING: Error opening serial port %s, attempting to carry on...\n", ai.device_arg); 
    #endif
    }

    if (ai.verbose_given) { 
        bool *b=&verboseFlag; 
        ++*b; 
        printf("Port %s opened successfully\n", ai.device_arg); 
    }

    /* --- Send relevant commands, reply with data --- */
    //Read temperature 
    if(ai.read_temperature_given) { 
        if(verboseFlag){printf("Reading temperature!\n");}; 
        double temp =  eurotherm902s_get_temperature(port); 
        printf("***TEMP: %f\n", temp); 
    }

    //Check for temperature sensor breaks
    if(ai.check_sensor_break_given || ai.status_all_given ){
        if(verboseFlag){printf("Checking sensor!\n"); } 
        bool result = eurotherm902s_check_sensor_break(port); 
        if (result == SET) { 
            fprintf(stderr,"Warning: PT100/Thermocouple sensor break on device.\n"); 
        } else{
            printf("***SBC : OK\n"); 
        }

    }

    //Check heater 
    if(ai.check_heater_given|| ai.status_all_given){
        if(verboseFlag){ printf("Checking heater!\n"); }       
        int result = bvt3000_check_heater(port); 
        if (result == HEATER_OVERHEATING ) {
            printf("***HCC : FAIL\n"); 
            if(verboseFlag){printf("Disabling heater...\n"); }
            bvt3000_set_heater_state(UNSET,port); 
            fprintf(stderr,"Heater overheating, heater disabled.\n"); 
        } else if(result == HEATER_OK){ 
            printf("***HCC : OK\n"); 
        } else { 
            fprintf(stderr, "Error: heater unexpected result"); 
        }
    }

    //Get heater state 
    if(ai.get_heater_state_given|| ai.status_all_given) {
        checkHeater(port); 
    } 

    // Enable heater 
    if(ai.heater_on_given) { 
        if(verboseFlag){printf("Enabling heater!\n"); }
        bvt3000_set_heater_state(SET, port); 
        checkHeater(port); 
    }

    // Disable heater 
    if(ai.heater_off_given) { 
        if(verboseFlag){printf("Disabling heater!\n"); }
        bvt3000_set_heater_state(UNSET, port); 
        checkHeater(port); 
    }
    
    // Get heater power limit 
    if(ai.get_heater_power_limit_given) { 
        if(verboseFlag){printf("Getting heater power limit!\n"); }
        double result = eurotherm902s_get_heater_power_limit(port); 
        printf("***HPWL: %lf\n", result); 
    }

    // Set heater power limit 
    if(ai.set_heater_power_limit_given) { 
        if(verboseFlag){printf("Setting heater power limit!\n"); }
        setHeaterPowerLimit((double) ai.set_heater_power_limit_arg, port); 
    }

    //Set heater power as a percentage
    if(ai.set_heater_power_given) { 
        double power = (double) ai.set_heater_power_arg; 
        if ((power < 0.0 ) || (power > 100.0) ){ 
            fprintf(stderr,"FATAL: Heater power not in range 0-100 [percent]\n"); 
        } else { 
            if(verboseFlag){printf("Setting heater power to %lf!\n", power); }
            eurotherm902s_set_heater_power(power, port); 
        }
    }

    //Get heater power 
    if ( ai.get_heater_power_given) { 
        if(verboseFlag) { printf("Getting heater power as a percentage!\n"); } 
        double result = eurotherm902s_get_heater_power(port); 
        printf("***HPWR: %lf\n",result); 
    }


    //Enable automatic PID control 
    if(ai.enable_PID_control_given) {
        if(verboseFlag) { printf("Enabling PID control!\n"); } 
        if(! ai.manual_mode_given) {
            eurotherm902s_set_mode(AUTOMATIC_MODE, port);
        } else { 
            fprintf(stderr,"FATAL: Both manual and automatic mode requested\n"); 
        }
    }

    //Enable manual mode (the heater power can be controlled by keys on the device, and also by software)
    if(ai.manual_mode_given) {
        if(verboseFlag) { printf("Enabling manual control!\n"); } 
        if(! ai.enable_PID_control_given) { 
             eurotherm902s_set_mode(MANUAL_MODE, port );
        } else { 
            fprintf(stderr,"FATAL: Both manual and automatic mode requested\n"); 
        }
    }

    //Ask the device about its current mode 
    if(ai.get_mode_given|| ai.status_all_given) { 
        if(verboseFlag) { printf("Getting current PID mode!\n"); } 
        int result = eurotherm902s_get_mode(port) ; 
        if (result == AUTOMATIC_MODE) { 
            printf("***PIDM: AUTO\n"); 
        } else if (result == MANUAL_MODE) { 
            printf("***PIDM: MANUAL\n");
        }
    }

    //Get gas flow rate 
    if(ai.get_gas_flow_rate_given) {
        if(verboseFlag){printf("Getting gas flow rate!\n"); }

        unsigned int gfr = bvt3000_get_flow_rate(port); 
        assert (gfr <= 15) ; // For the 4-valve block gas flow device at any rate -- you might need to change this. 
        double result = translate_flow_rate(gfr); 
        printf("***GASR: %lf\n", result); 
    } 

    //Set gas flow rate 
    if(ai.set_gas_flow_rate_given){
        if(verboseFlag) { printf("Requested change to gas flow rate to %f l/hr!\n", 
                ai.set_gas_flow_rate_arg); }

        if (verboseFlag) {printf("Checking heater status...\n");}
        bool result = bvt3000_get_heater_state(port); 

        if (result == SET) {
            if (verboseFlag) {printf("Setting flow rate...\n");}
            set_flow_rate((double) ai.set_gas_flow_rate_arg, port); 
            if (verboseFlag) {printf("Checking flow rate...\n");}
            unsigned int gfr = bvt3000_get_flow_rate(port); 
            if (verboseFlag) {printf("Actual flow rate %u...\n", gfr);}
        } else if (result == UNSET) {
            fprintf(stderr,"FATAL: Cannot change gas flow rate with heater off\n"); 
        }

    }

    //Get temperature setpoint 

    if(ai.get_temperature_setpoint_given) { 
        if(verboseFlag) { printf("Getting temperature setpoint!\n"); } 

        double tpsp =  eurotherm902s_get_setpoint( SP1, port );
        printf("***TSP : %lf\n", tpsp); 
    }


    //Set temperature setpoint 
    
    if(ai.set_temperature_setpoint_given) { 
        if(verboseFlag)  {printf("Setting temperature setpoint SP1 to %f!\n", ai.set_temperature_setpoint_arg); } 
        double temp = (double) ai.set_temperature_setpoint_arg; 
        eurotherm902s_set_setpoint(SP1, temp, port); 
    }

    //Get Eurotherm temperature box status 
    if(ai.get_eurotherm_status_given|| ai.status_all_given) { 
        if(verboseFlag) { printf("Getting Eurotherm status!\n"); } 
        bool result = eurotherm902s_get_alarm_state( port );
        if(result == SET) { 
            fprintf(stderr,"***EALM: ON\n"); 
            if(verboseFlag) { printf("Eurotherm is alarming!\n"); } 
        } else if (result == UNSET) { 
            printf("***EALM: OFF\n"); 
        }
    }


    //Lock or unlock keypad 
    if(ai.lock_keypad_given) {
        if(verboseFlag) { printf("Keyboard lock/unlock state change requested!\n"); }
        if (ai.lock_keypad_arg == 1) {
            if(verboseFlag){printf("Unlocking requested....\n"); }
            eurotherm902s_lock_keyboard((bool) ai.lock_keypad_arg, port); 
        } else {
            if(verboseFlag){printf("Locking requested....\n"); }
            eurotherm902s_lock_keyboard((bool) ai.lock_keypad_arg, port); 
        }
    }


    //Get PID -- P
    if(ai.get_proportional_band_given) { 
            if (verboseFlag) {printf("Getting proportional band...\n");}
            double result = eurotherm902s_get_proportional_band(port); 
            printf("***PPID: %lf\n", result); 
    }


    //Get PID -- I 
    if(ai.get_integral_time_given) { 
            if (verboseFlag) {printf("Getting integral time...\n");}
            double result = eurotherm902s_get_integral_time(port); 
            printf("***IPID: %lf\n", result); 
    }

    //Get PID -- D 
    if(ai.get_differential_time_given) { 
            if (verboseFlag) {printf("Getting derivative time...\n");}
            double result = eurotherm902s_get_derivative_time(port); 
            printf("***DPID: %lf\n", result); 
    }


    //Set PID -- P
    if(ai.set_proportional_band_given) { 
            float setValue = ai.set_proportional_band_arg; 
            if (verboseFlag) {printf("Setting proportional band to %f...\n", setValue);}
            eurotherm902s_set_proportional_band(setValue, port); 
    }


    //Set PID -- I 
    if(ai.set_integral_time_given) { 
            float setValue = ai.set_integral_time_arg; 
            if (verboseFlag) {printf("Setting integral time to %f...\n", setValue);}
            eurotherm902s_set_integral_time(setValue, port); 
    }

    //Get PID -- D 
    if(ai.set_differential_time_given) { 
            float setValue = ai.set_differential_time_arg; 
            if (verboseFlag) {printf("Setting derivative time to %f...\n", setValue);}
            eurotherm902s_set_derivative_time(setValue, port); 
    }

    //LN2 methods  -- get Ln2 heater state 
    if(ai.get_ln2_heater_state_given) { 
        if(verboseFlag) {printf("Getting LN2 heater state!\n");}
        bool result = bvt3000_get_ln2_heater_state(port);
        if (result == SET){
            printf("***N2HE: ON\n"); 
        } else if (result == UNSET) {
            printf("***N2HE: OFF\n"); 
        }
    }

    //Set heater state 
    if(ai.set_ln2_heater_state_given) { 
        if(verboseFlag) {printf("Changing state of ln2 LN2 heater to %u!\n", (bool)ai.set_ln2_heater_state_arg); } 
        bvt3000_set_ln2_heater_state((bool)ai.set_ln2_heater_state_arg, port); 
    }

    //Get LN2 heater power
    if(ai.get_ln2_heater_power_given) { 
        if(verboseFlag) {printf("Getting LN2 heater power!\n");}
        double result = bvt3000_get_ln2_heater_power(port); 
        printf("***N2HP: %lf\n", result); 
    }
    
    //Set LN2  heater power 
    if(ai.set_ln2_heater_power_given) { 
        float gvnPwr = ai.set_ln2_heater_power_arg; 
        if( (gvnPwr < 0 ) || (gvnPwr > 100)) { 
            fprintf(stderr,"FATAL: Supplied heater power %f out of range [0, 100]\n", gvnPwr); 
        } else { 
            if(verboseFlag) { printf("Setting LN2 heater power to %f!\n", gvnPwr); } 
            bvt3000_set_ln2_heater_power(gvnPwr, port); 
        }
    }

    //Check LN2 tank
    if(ai.check_ln2_heater_given) { 
        if(verboseFlag) { printf("Checking LN2 tank!\n"); } 
        int result = bvt3000_check_ln2_heater(port); 
        if(result == LN2_OK) { 
            printf("***N2TK: OK\n"); 
        } else if (result == LN2_NEEDS_REFILL ) { 
            printf("***N2TK: FILL_ME\n"); 
        } else if (result == LN2_TANK_EMPTY) { 
            printf("***N2TK: EMPTY\n"); 
        }
    }



    /*------------------------------------------------------------------------*/
    /* --- Close device --- */	
    enum sp_return anyErrors = sp_close(port); 

    if (ai.verbose_given && (anyErrors == SP_OK) ) { 
        printf("Port %s closed successfully\n", ai.device_arg); 
    } else { 
        fprintf(stderr,"Error closing port %s, return code %d", ai.device_arg, anyErrors); 
    }
    return 0; 
}

