#include "convenient_wrapper_functions.h" 
extern bool verboseFlag; 

static double flow_rates[ ] = {    0.0,    /* in l/h */
                                 135.0,
                                 270.0,
                                 400.0,
                                 535.0,
                                 670.0,
                                 800.0,
                                 935.0,
                                1070.0,
                                1200.0,
                                1335.0,
                                1470.0,
                                1600.0,
                                1735.0,
                                1870.0,
                                2000.0 };

double translate_flow_rate(unsigned int gfr){
    return(flow_rates[gfr]); 
}

int set_flow_rate( double flow_rate, struct sp_port* port_choice ) {
    int fr_index=0;  
    int i = 0; 

    if(verboseFlag) {
        printf("Requested to change flow rate to %lf\n", flow_rate); 
    } 

    if ( flow_rate < 0 ) { 
        printf("FATAL: flow rate negative!\n"); 
        return(-1) ;
    }
    
    if (flow_rate > 2000.0) { 
        printf("FATAL: flow rate must be below 2000 l/hr\n"); 
        return(-1); 
    }
    for ( i = 1; i < 16; i++ )
        if (    flow_rate >= flow_rates[ i - 1 ]
             && flow_rate <= flow_rates[ i ] )
        {
            if ( flow_rate < 0.5 * ( flow_rates[ i - 1 ] + flow_rates[ i ] ) )
                fr_index = i - 1;
            else
                fr_index = i;
            break;
        }

    if (    flow_rate != 0.0
         && fabs( ( flow_rate - flow_rates[ fr_index ] ) / flow_rate ) > 0.01 )
        printf("WARNING: Flow rate had to be adjusted from %.1f l/h to "
               "%.1f l/h.\n", flow_rate, flow_rates[ fr_index ] );

    bvt3000_set_flow_rate( fr_index , port_choice);
    return 0; 
}

void checkHeater(struct sp_port* port_choice) 
{
        if (verboseFlag) { printf("Checking heater state!\n");}
        bool result = bvt3000_get_heater_state(port_choice); 
        if (result == SET) {
            if (verboseFlag){printf("Heater is ON\n"); }
            printf("***HSC : ON\n"); 
        } else if (result == UNSET) { 
            printf("***HSC : OFF\n"); 
            if (verboseFlag){printf("Heater is OFF\n"); }
        }
}

void setHeaterPowerLimit(float limit, struct sp_port* port_choice){
    if (limit < 0.0 || limit > 100.0) { 
        fprintf(stderr,"FATAL: Invalid power limit, %f not in [0, 100]\n", limit); 
    } else if (limit >= 0.0 && limit <= 100.0) { 
        bool result = bvt3000_get_heater_state(port_choice); 
        if (result == UNSET) { 
            fprintf(stderr,"WARNING: Heater not currently enabled; limit takes effect in the future.\n"); 
        }
        eurotherm902s_set_heater_power_limit( limit,  port_choice );

    }
}
