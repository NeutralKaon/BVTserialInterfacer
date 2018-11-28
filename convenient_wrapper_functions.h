#include <stdio.h>
#include "serial_jjm.h"
#include <math.h>
int set_flow_rate( double flow_rate, struct sp_port* port_choice ) ;
void checkHeater(struct sp_port* port) ;
void setHeaterPowerLimit(float limit, struct sp_port* port_choice);
double translate_flow_rate(unsigned int gfr); 
